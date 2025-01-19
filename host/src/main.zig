const std = @import("std");
const fs = std.fs;
const Protocol = @import("protocol.zig").Protocol;
const PacketHeader = @import("protocol.zig").PacketHeader;

const c = @cImport({
    @cInclude("png.h");
    @cInclude("fcntl.h");
    @cInclude("unistd.h");
    @cInclude("errno.h");
    @cInclude("string.h");
});

const SerialLogger = struct {
    file: std.fs.File,
    mutex: std.Thread.Mutex,

    pub fn init(path: []const u8) !SerialLogger {
        const file = try std.fs.cwd().createFile(path, .{ .truncate = false });
        return SerialLogger{
            .file = file,
            .mutex = std.Thread.Mutex{},
        };
    }

    pub fn deinit(self: *SerialLogger) void {
        self.file.close();
    }

    pub fn logSent(self: *SerialLogger, data: []const u8) !void {
        const writer = self.file.writer();
        self.mutex.lock();
        defer self.mutex.unlock();

        const timestamp = std.time.timestamp();
        try writer.print("[{d}] TX: ", .{timestamp});
        for (data) |byte| {
            try writer.print("{x:0>2} ", .{byte});
        }
        try writer.writeAll("\n");
    }

    pub fn logReceived(self: *SerialLogger, data: []const u8) !void {
        const writer = self.file.writer();
        self.mutex.lock();
        defer self.mutex.unlock();

        const timestamp = std.time.timestamp();
        try writer.print("[{d}] RX: ", .{timestamp});

        // Always print ASCII representation, replacing non-printable chars with dots
        try writer.print("\"", .{});
        for (data) |byte| {
            if (byte >= 32 and byte <= 126) {
                try writer.writeByte(byte);
            } else if (byte == '\n' or byte == '\r') {
                try writer.print("\\{c}", .{byte});
            } else {
                try writer.writeByte('.');
            }
        }
        try writer.print("\"", .{});

        // Also print hex values for reference
        try writer.print(" (hex:", .{});
        for (data) |byte| {
            try writer.print(" {x:0>2}", .{byte});
        }
        try writer.print(")", .{});
        try writer.writeAll("\n");
    }
};

// Protocol helper functions
fn writePacket(serial: std.fs.File, header: PacketHeader, payload: ?[]const u8, logger: *SerialLogger) !void {
    // Write header
    var header_bytes: [@sizeOf(PacketHeader)]u8 = undefined;
    @memcpy(&header_bytes, std.mem.asBytes(&header));
    try serial.writeAll(&header_bytes);
    try logger.logSent(&header_bytes);

    // Write payload if present
    if (payload) |data| {
        try serial.writeAll(data);
        try logger.logSent(data);
    }
}

fn readPacket(serial: std.fs.File, buffer: []u8, logger: *SerialLogger) !struct { header: PacketHeader, payload: []u8 } {
    // Read header
    var header_bytes: [@sizeOf(PacketHeader)]u8 = undefined;
    const header_read = try serial.read(&header_bytes);
    if (header_read != @sizeOf(PacketHeader)) return error.InvalidHeader;
    try logger.logReceived(header_bytes[0..header_read]);

    const header = @as(*const PacketHeader, @ptrCast(@alignCast(&header_bytes))).*;

    // Read payload if present
    var payload: []u8 = buffer[0..0];
    if (header.length > 0) {
        if (header.length > buffer.len) return error.BufferTooSmall;
        const payload_read = try serial.read(buffer[0..header.length]);
        if (payload_read != header.length) return error.InvalidPayload;
        payload = buffer[0..payload_read];
        try logger.logReceived(payload);

        // Verify checksum
        const calc_crc = Protocol.calculateCrc32(payload);
        if (calc_crc != header.checksum) return error.InvalidChecksum;
    }

    return .{ .header = header, .payload = payload };
}

fn getRetryDelay(attempt: u8) u64 {
    const delay = Protocol.MIN_RETRY_DELAY_MS * std.math.pow(u64, 2, attempt);
    return @min(delay, Protocol.MAX_RETRY_DELAY_MS);
}

fn syncSerial(serial: std.fs.File, logger: *SerialLogger) !bool {
    const stdout = std.io.getStdOut().writer();
    var buffer: [Protocol.MAX_PACKET_SIZE]u8 = undefined;

    try stdout.print("\nInitiating protocol sync...\n", .{});

    // Clear any pending data
    while (true) {
        const bytes = serial.read(&buffer) catch |err| switch (err) {
            error.WouldBlock => break,
            else => return err,
        };
        if (bytes == 0) break;
        try logger.logReceived(buffer[0..bytes]);
    }

    // Send sync packet with version info
    const sync_payload = [_]u8{Protocol.VERSION};
    const sync_header = PacketHeader.init(Protocol.PacketType.sync, 0, @intCast(sync_payload.len), Protocol.calculateCrc32(&sync_payload));

    var retry: u8 = 0;
    while (retry < Protocol.MAX_RETRIES) : (retry += 1) {
        if (retry > 0) {
            const delay = getRetryDelay(retry);
            try stdout.print("\rRetry {}/{}: Waiting {}ms...", .{ retry + 1, Protocol.MAX_RETRIES, delay });
            std.time.sleep(delay * std.time.ns_per_ms);
        }

        // Send sync packet
        try writePacket(serial, sync_header, &sync_payload, logger);

        // Wait for sync acknowledgment
        const response = readPacket(serial, &buffer, logger) catch |err| {
            try stdout.print("Sync error: {}\n", .{err});
            continue;
        };

        if (response.header.type == Protocol.PacketType.sync_ack) {
            if (response.payload.len > 0 and response.payload[0] == Protocol.VERSION) {
                try stdout.print("\nSync established (Protocol v{})\n", .{Protocol.VERSION});
                return true;
            }
            try stdout.print("\nWarning: Protocol version mismatch (got v{}, expected v{})\n", .{ response.payload[0], Protocol.VERSION });
            return false;
        }
    }

    try stdout.print("\nSync failed after {} attempts\n", .{Protocol.MAX_RETRIES});
    return false;
}

fn sendImageData(serial: std.fs.File, image_data: []const u8, logger: *SerialLogger) !void {
    const stdout = std.io.getStdOut().writer();
    var buffer: [Protocol.MAX_PACKET_SIZE]u8 = undefined;

    // Try protocol sync first
    const new_protocol = try syncSerial(serial, logger);

    if (new_protocol) {
        try stdout.print("Using protocol v{} for transfer\n", .{Protocol.VERSION});

        // Send image command
        const cmd_header = PacketHeader.init(Protocol.PacketType.cmd, 0, 1, Protocol.calculateCrc32(&[_]u8{@intFromEnum(Protocol.Command.image)}));
        try writePacket(serial, cmd_header, &[_]u8{@intFromEnum(Protocol.Command.image)}, logger);

        // Wait for acknowledgment
        _ = try readPacket(serial, &buffer, logger);

        // Send data in chunks
        var sequence: u8 = 0;
        var total_sent: usize = 0;

        while (total_sent < image_data.len) {
            const chunk_size = @min(Protocol.CHUNK_SIZE, image_data.len - total_sent);
            const chunk = image_data[total_sent .. total_sent + chunk_size];

            const data_header = PacketHeader.init(Protocol.PacketType.data, sequence, @intCast(chunk_size), Protocol.calculateCrc32(chunk));

            var retry: u8 = 0;
            while (retry < Protocol.MAX_RETRIES) : (retry += 1) {
                if (retry > 0) {
                    const delay = getRetryDelay(retry);
                    try stdout.print("\rRetry {}/{}: Waiting {}ms...", .{ retry + 1, Protocol.MAX_RETRIES, delay });
                    std.time.sleep(delay * std.time.ns_per_ms);
                }

                try writePacket(serial, data_header, chunk, logger);

                const response = readPacket(serial, &buffer, logger) catch |err| {
                    try stdout.print("Transfer error: {}\n", .{err});
                    continue;
                };

                switch (response.header.type) {
                    .ack => break,
                    .nack => {
                        if (response.payload.len > 0) {
                            try stdout.print("\nNACK received: {s}\n", .{response.payload});
                        }
                        continue;
                    },
                    else => continue,
                }
            }

            if (retry >= Protocol.MAX_RETRIES) {
                return error.TransferFailed;
            }

            total_sent += chunk_size;
            sequence +%= 1;

            const progress = @as(f32, @floatFromInt(total_sent)) / @as(f32, @floatFromInt(image_data.len)) * 100.0;
            try stdout.print("\rProgress: {d:.1}% ({}/{} bytes)", .{ progress, total_sent, image_data.len });
        }

        // Send end marker
        const end_header = PacketHeader.init(Protocol.PacketType.cmd, sequence, 1, Protocol.calculateCrc32(&[_]u8{@intFromEnum(Protocol.Command.end)}));
        try writePacket(serial, end_header, &[_]u8{@intFromEnum(Protocol.Command.end)}, logger);

        _ = try readPacket(serial, &buffer, logger);

        try stdout.print("\nTransfer complete!\n", .{});
    } else {
        // Fall back to legacy protocol
        try stdout.print("Using legacy protocol\n", .{});

        // Send image command
        const cmd = [_]u8{@intFromEnum(Protocol.Command.image)};
        try serial.writeAll(&cmd);
        try logger.logSent(&cmd);

        // Wait for ACK
        var response_buf: [1]u8 = undefined;
        const bytes_read = try serial.read(&response_buf);
        if (bytes_read == 0 or response_buf[0] != 'A') {
            return error.NoAckReceived;
        }
        try logger.logReceived(response_buf[0..bytes_read]);

        // Send image size
        const size_bytes = writeIntBigEndian(@intCast(image_data.len));
        try serial.writeAll(&size_bytes);
        try logger.logSent(&size_bytes);

        // Wait for ACK
        const size_ack = try serial.read(&response_buf);
        if (size_ack == 0 or response_buf[0] != 'A') {
            return error.NoAckReceived;
        }
        try logger.logReceived(response_buf[0..size_ack]);

        // Send data in chunks
        var total_sent: usize = 0;
        var chunk_counter: usize = 0;

        while (total_sent < image_data.len) {
            const chunk_size = @min(Protocol.CHUNK_SIZE, image_data.len - total_sent);
            const chunk = image_data[total_sent .. total_sent + chunk_size];
            chunk_counter += 1;

            // Send chunk
            try serial.writeAll(chunk);
            try logger.logSent(chunk);

            // Wait for ACK with retry
            var retry: u8 = 0;
            while (retry < Protocol.MAX_RETRIES) : (retry += 1) {
                if (retry > 0) {
                    const delay = getRetryDelay(retry);
                    try stdout.print("\rRetry {}/{}: Waiting {}ms...", .{ retry + 1, Protocol.MAX_RETRIES, delay });
                    std.time.sleep(delay * std.time.ns_per_ms);
                }

                const chunk_ack = try serial.read(&response_buf);
                if (chunk_ack > 0 and response_buf[0] == 'A') {
                    try logger.logReceived(response_buf[0..chunk_ack]);
                    break;
                }
            }

            if (retry >= Protocol.MAX_RETRIES) {
                return error.TransferFailed;
            }

            total_sent += chunk_size;
            const progress = @as(f32, @floatFromInt(total_sent)) / @as(f32, @floatFromInt(image_data.len)) * 100.0;
            try stdout.print("\rProgress: {d:.1}% (Chunk {}/{}, {} bytes)", .{ progress, chunk_counter, (image_data.len + Protocol.CHUNK_SIZE - 1) / Protocol.CHUNK_SIZE, total_sent });
        }
        try stdout.writeByte('\n');

        // Send end marker
        const end_marker = [_]u8{@intFromEnum(Protocol.Command.end)};
        try serial.writeAll(&end_marker);
        try logger.logSent(&end_marker);

        // Wait for final ACK
        const end_ack = try serial.read(&response_buf);
        if (end_ack == 0 or response_buf[0] != 'A') {
            return error.NoAckReceived;
        }
        try logger.logReceived(response_buf[0..end_ack]);

        try stdout.print("\nTransfer complete!\n", .{});
    }
}

fn writeIntBigEndian(value: u32) [4]u8 {
    return [4]u8{
        @truncate((value >> 24) & 0xFF),
        @truncate((value >> 16) & 0xFF),
        @truncate((value >> 8) & 0xFF),
        @truncate(value & 0xFF),
    };
}

const ImageSize = struct {
    const width: usize = 240;
    const height: usize = 240;
    const pixels: usize = width * height;
    const bytes_per_pixel: usize = 2; // RGB565
    const total_bytes: usize = pixels * bytes_per_pixel;
};

const ReadContext = struct {
    file: std.fs.File,
};

pub fn decodePNG(allocator: std.mem.Allocator, file_path: []const u8) ![]u8 {
    const file = try std.fs.cwd().openFile(file_path, .{});
    defer file.close();

    var png_ptr = c.png_create_read_struct(c.PNG_LIBPNG_VER_STRING, null, null, null);
    if (png_ptr == null) return error.AllocationFailure;
    defer c.png_destroy_read_struct(&png_ptr, null, null);

    const info_ptr = c.png_create_info_struct(png_ptr);
    if (info_ptr == null) return error.AllocationFailure;

    const jmp_buf_ptr = @constCast(&c.png_jmpbuf(png_ptr)[0]);
    if (c.setjmp(jmp_buf_ptr) != 0) {
        return error.DecodingError;
    }

    var context = ReadContext{ .file = file };
    const read_func = struct {
        fn callback(png_ptr_arg: ?*c.png_struct, outBytes: [*c]u8, byte_count: usize) callconv(.C) void {
            if (png_ptr_arg) |outer_png_ptr| {
                const file_ctx = @as(*ReadContext, @ptrCast(@alignCast(c.png_get_io_ptr(outer_png_ptr))));
                _ = file_ctx.file.read(outBytes[0..byte_count]) catch {};
            }
        }
    }.callback;

    c.png_set_read_fn(png_ptr, &context, read_func);
    c.png_read_info(png_ptr, info_ptr);

    const width = c.png_get_image_width(png_ptr, info_ptr);
    const height = c.png_get_image_height(png_ptr, info_ptr);

    // Validate image dimensions
    if (width != ImageSize.width or height != ImageSize.height) {
        return error.InvalidImageDimensions;
    }

    const color_type = c.png_get_color_type(png_ptr, info_ptr);
    const bit_depth = c.png_get_bit_depth(png_ptr, info_ptr);

    // Configure transformations for RGB output
    if (bit_depth == 16)
        c.png_set_strip_16(png_ptr);
    if (color_type == c.PNG_COLOR_TYPE_PALETTE)
        c.png_set_palette_to_rgb(png_ptr);
    if (color_type == c.PNG_COLOR_TYPE_GRAY and bit_depth < 8)
        c.png_set_expand_gray_1_2_4_to_8(png_ptr);
    if (c.png_get_valid(png_ptr, info_ptr, c.PNG_INFO_tRNS) != 0)
        c.png_set_tRNS_to_alpha(png_ptr);
    if (color_type == c.PNG_COLOR_TYPE_GRAY or color_type == c.PNG_COLOR_TYPE_GRAY_ALPHA)
        c.png_set_gray_to_rgb(png_ptr);
    if (color_type == c.PNG_COLOR_TYPE_RGB_ALPHA or color_type == c.PNG_COLOR_TYPE_GRAY_ALPHA)
        c.png_set_strip_alpha(png_ptr);

    c.png_read_update_info(png_ptr, info_ptr);

    const rowbytes = c.png_get_rowbytes(png_ptr, info_ptr);
    const total_size = height * rowbytes;

    // Verify expected size
    if (total_size != ImageSize.width * ImageSize.height * 3) {
        return error.UnexpectedImageFormat;
    }

    const raw_data = try allocator.alloc(u8, total_size);
    errdefer allocator.free(raw_data);

    var row_pointers = try allocator.alloc([*c]u8, height);
    defer allocator.free(row_pointers);

    for (0..height) |i| {
        row_pointers[i] = raw_data.ptr + i * rowbytes;
    }

    c.png_read_image(png_ptr, row_pointers.ptr);

    return raw_data;
}

fn convertToRGB565(allocator: std.mem.Allocator, rgb888: []const u8, width: usize, height: usize) ![]u8 {
    if (rgb888.len != width * height * 3) {
        return error.InvalidInputSize;
    }

    const total_bytes = width * height * ImageSize.bytes_per_pixel;
    var rgb565_data = try allocator.alloc(u8, total_bytes);
    errdefer allocator.free(rgb565_data);

    var idx: usize = 0;
    var i: usize = 0;
    while (idx < rgb888.len) : (idx += 3) {
        const r = rgb888[idx];
        const g = rgb888[idx + 1];
        const b = rgb888[idx + 2];
        const rgb565 = rgb888_to_rgb565(r, g, b);

        rgb565_data[i] = @truncate(rgb565 & 0xFF);
        rgb565_data[i + 1] = @truncate((rgb565 >> 8) & 0xFF);
        i += 2;
    }

    return rgb565_data;
}

fn rgb888_to_rgb565(r: u8, g: u8, b: u8) u16 {
    return (@as(u16, r >> 3) << 11) | (@as(u16, g >> 2) << 5) | @as(u16, b >> 3);
}

pub fn main() !void {
    const stdout = std.io.getStdOut().writer();
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // Initialize serial logger
    var logger = try SerialLogger.init("serial.log");
    defer logger.deinit();

    try stdout.print("Opening serial port...\n", .{});
    const serial = blk: {
        const file = fs.openFileAbsolute("/dev/ttyACM0", .{ .mode = .read_write }) catch |err| {
            try stdout.print("Error: Could not open serial port: {}\n", .{err});
            try stdout.print("Troubleshooting tips:\n", .{});
            try stdout.print("1. Make sure the device is connected\n", .{});
            try stdout.print("2. Check if the device shows up in 'lsusb'\n", .{});
            try stdout.print("3. Verify you have permission to access /dev/ttyACM0\n", .{});
            try stdout.print("4. Try unplugging and replugging the device\n", .{});
            break :blk null;
        };

        try stdout.print("Configuring serial port...\n", .{});

        // Configure serial port for non-blocking I/O
        const flags = c.fcntl(@as(c_int, @intCast(file.handle)), c.F_GETFL, @as(c_int, 0));
        if (flags < 0) {
            try stdout.print("Error getting file flags: {s}\n", .{"Permission Denied"});
            file.close();
            break :blk null;
        }

        const result = c.fcntl(@as(c_int, @intCast(file.handle)), c.F_SETFL, @as(c_int, flags | c.O_NONBLOCK));
        if (result < 0) {
            try stdout.print("Error setting non-blocking mode: {s}\n", .{"Permission Denied"});
            file.close();
            break :blk null;
        }

        try stdout.print("Serial port ready\n", .{});
        break :blk file;
    };
    defer if (serial) |s| s.close();

    // Give device time to initialize
    try stdout.print("Waiting for device to initialize...\n", .{});
    std.time.sleep(2000 * std.time.ns_per_ms);

    // Show initial help
    try stdout.print("Commands:\n", .{});
    try stdout.print("1-3 : Show test patterns\n", .{});
    try stdout.print("4   : Upload and display image (240x240 RGB565)\n", .{});
    try stdout.print("5   : Exit program\n", .{});
    try stdout.print("{c}   : Show this help message\n\n", .{@intFromEnum(Protocol.Command.help)});

    while (true) {
        try stdout.writeAll("Choice: ");

        var buf: [256]u8 = undefined;
        if (try std.io.getStdIn().reader().readUntilDelimiterOrEof(&buf, '\n')) |user_input| {
            const choice = std.mem.trim(u8, user_input, " \t\r\n");
            if (choice.len == 0) continue;

            if (serial) |s| {
                switch (choice[0]) {
                    @intFromEnum(Protocol.Command.help) => {
                        try stdout.print("\nAvailable Commands:\n", .{});
                        try stdout.print("------------------\n", .{});
                        try stdout.print("1-3 : Show test patterns\n", .{});
                        try stdout.print("4   : Upload and display image (240x240 RGB565)\n", .{});
                        try stdout.print("5   : Exit program\n", .{});
                        try stdout.print("{c}   : Show this help message\n\n", .{@intFromEnum(Protocol.Command.help)});
                    },
                    '1'...'3' => {
                        var buffer: [Protocol.MAX_PACKET_SIZE]u8 = undefined;
                        const new_protocol = try syncSerial(s, &logger);

                        if (new_protocol) {
                            const cmd_header = PacketHeader.init(Protocol.PacketType.cmd, 0, 1, Protocol.calculateCrc32(&[_]u8{choice[0]}));
                            try writePacket(s, cmd_header, &[_]u8{choice[0]}, &logger);
                            _ = try readPacket(s, &buffer, &logger);
                        } else {
                            try s.writeAll(&[_]u8{choice[0]});
                            try logger.logSent(&[_]u8{choice[0]});
                            var response_buf: [1]u8 = undefined;
                            _ = try s.read(&response_buf);
                        }

                        std.time.sleep(200 * std.time.ns_per_ms);
                    },
                    '4' => {
                        try stdout.print("Loading image from host/240.png...\n", .{});
                        const rgb888_data = decodePNG(allocator, "host/240.png") catch |err| {
                            switch (err) {
                                error.InvalidImageDimensions => {
                                    try stdout.print("Error: Invalid image dimensions\n", .{});
                                    try stdout.print("Required: 240x240 pixels\n", .{});
                                },
                                error.UnexpectedImageFormat => {
                                    try stdout.print("Error: Unexpected image format\n", .{});
                                    try stdout.print("Required: RGB format\n", .{});
                                },
                                error.FileNotFound => {
                                    try stdout.print("Error: File not found: host/240.png\n", .{});
                                },
                                else => {
                                    try stdout.print("Error decoding PNG: {}\n", .{err});
                                },
                            }
                            continue;
                        };
                        defer allocator.free(rgb888_data);

                        try stdout.print("Converting to RGB565 format...\n", .{});
                        const rgb565_data = try convertToRGB565(allocator, rgb888_data, 240, 240);
                        defer allocator.free(rgb565_data);

                        try sendImageData(s, rgb565_data, &logger);
                    },
                    '5' => std.process.exit(0),
                    else => {
                        try stdout.print("Invalid choice: {c}\n", .{choice[0]});
                        try stdout.print("Send {c} for help\n", .{@intFromEnum(Protocol.Command.help)});
                    },
                }
            } else {
                try stdout.writeAll("Serial port not connected. Cannot send commands.\n");
            }
        }
    }
}
