const std = @import("std");
const fs = std.fs;

const c = @cImport({
    @cInclude("png.h");
});

const ImageSize = struct {
    const width: usize = 240;
    const height: usize = 240;
    const pixels: usize = width * height;
    const bytes_per_pixel: usize = 2; // RGB565
    const total_bytes: usize = pixels * bytes_per_pixel;
};

const ReadFn = struct {
    stream: *std.fs.File.Reader,
    pub fn read(png_ptr: *c.png_struct, data: [*c]u8, size: usize) callconv(.C) void {
        const self = @as(*ReadFn, @ptrCast(c.png_get_progressive_ptr(png_ptr)));
        _ = self.stream.readAll(data[0..size]) catch {};
    }
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
    // Validate input dimensions
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

        // Safely write both bytes
        rgb565_data[i] = @truncate(rgb565 & 0xFF);
        rgb565_data[i + 1] = @truncate((rgb565 >> 8) & 0xFF);
        i += 2;
    }

    return rgb565_data;
}

// Converts RGB888 to RGB565
fn rgb888_to_rgb565(r: u8, g: u8, b: u8) u16 {
    return (@as(u16, r >> 3) << 11) | (@as(u16, g >> 2) << 5) | @as(u16, b >> 3);
}

// Handles serial communication to send image data
fn sendImageData(serial: std.fs.File, image_data: []const u8) !void {
    const stdout = std.io.getStdOut().writer();

    try serial.writeAll(&[_]u8{ImageProtocol.START_MARKER});
    var response_buf: [1]u8 = undefined;
    if (try serial.read(&response_buf) != ImageProtocol.ACK) return error.ProtocolError;

    try serial.writeAll(&[_]u8{ImageProtocol.IMAGE_COMMAND});
    if (try serial.read(&response_buf) != ImageProtocol.ACK) return error.ProtocolError;

    var size_bytes: [4]u8 = undefined;
    std.mem.writeInt(u32, &size_bytes, @intCast(image_data.len), .big);
    try serial.writeAll(&size_bytes);

    if (try serial.read(&response_buf) != ImageProtocol.ACK) return error.ProtocolError;

    var total_sent: usize = 0;
    while (total_sent < image_data.len) {
        const chunk = image_data[total_sent..@min(total_sent + ImageProtocol.CHUNK_SIZE, image_data.len)];
        try serial.writeAll(chunk);
        if (try serial.read(&response_buf) != ImageProtocol.ACK) return error.ProtocolError;

        total_sent += chunk.len;
        const progress = @as(f32, @floatFromInt(total_sent)) / @as(f32, @floatFromInt(image_data.len)) * 100.0;
        try stdout.print("\rProgress: {d:.1}%", .{progress});
    }

    try serial.writeAll(&[_]u8{ImageProtocol.END_MARKER});
    if (try serial.read(&response_buf) != ImageProtocol.ACK) return error.ProtocolError;

    try stdout.writeAll("\nImage sent successfully!\n");
}

pub fn main() !void {
    const stdout = std.io.getStdOut().writer();
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    // Make the serial port connection optional
    const serial = blk: {
        const file = fs.openFileAbsolute("/dev/ttyACM0", .{ .mode = .read_write }) catch |err| {
            try stdout.print("Warning: Could not open serial port: {}\n", .{err});
            break :blk null;
        };
        break :blk file;
    };
    defer if (serial) |s| s.close();

    while (true) {
        try stdout.writeAll(
            \\Select:
            \\1. Checkerboard
            \\2. Stripes
            \\3. Gradient
            \\4. Image
            \\5. Exit
            \\Choice: 
        );

        var buf: [256]u8 = undefined;
        if (try std.io.getStdIn().reader().readUntilDelimiterOrEof(&buf, '\n')) |user_input| {
            const choice = std.mem.trim(u8, user_input, " \t\r\n");
            if (choice.len == 0) continue;

            // Add a check to ensure serial is connected before writing
            if (serial) |s| {
                switch (choice[0]) {
                    '1'...'3' => try s.writeAll(&[_]u8{choice[0]}),
                    '4' => {
                        try stdout.writeAll("Enter image path: ");
                        if (try std.io.getStdIn().reader().readUntilDelimiterOrEof(&buf, '\n')) |path| {
                            const trimmed_path = std.mem.trim(u8, path, " \t\r\n");

                            const rgb888_data = decodePNG(allocator, trimmed_path) catch |err| {
                                switch (err) {
                                    error.InvalidImageDimensions => try stdout.print("Error: Image must be {}x{} pixels\n", .{ ImageSize.width, ImageSize.height }),
                                    error.UnexpectedImageFormat => try stdout.print("Error: Image must be in RGB format\n", .{}),
                                    error.FileNotFound => try stdout.print("Error: File not found: {s}\n", .{trimmed_path}),
                                    else => try stdout.print("Error decoding PNG: {}\n", .{err}),
                                }
                                continue;
                            };
                            defer allocator.free(rgb888_data);

                            const rgb565_data = convertToRGB565(allocator, rgb888_data, ImageSize.width, ImageSize.height) catch |err| {
                                try stdout.print("Error converting to RGB565: {}\n", .{err});
                                continue;
                            };
                            defer allocator.free(rgb565_data);

                            sendImageData(s, rgb565_data) catch |err| {
                                try stdout.print("Error sending image: {}\n", .{err});
                                continue;
                            };
                        }
                    },
                    '5' => std.process.exit(0),
                    else => try stdout.writeAll("Invalid choice. Try again.\n"),
                }
            } else {
                try stdout.writeAll("Serial port not connected. Cannot send commands.\n");
            }
        }
    }
}

const ImageProtocol = struct {
    const START_MARKER: u8 = 'S';
    const END_MARKER: u8 = 'E';
    const IMAGE_COMMAND: u8 = 'I';
    const ACK: u8 = 'A';
    const CHUNK_SIZE: usize = 512;
};
