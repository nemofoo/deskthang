const std = @import("std");
const Serial = @import("serial.zig").Serial;
const Logger = @import("logger.zig").Logger;
const StateMachine = @import("state.zig").StateMachine;
const State = @import("state.zig").State;
const Packet = @import("packet.zig").Packet;
const PacketType = @import("packet.zig").PacketType;
const PacketHeader = @import("packet.zig").PacketHeader;
const constants = @import("constants.zig");
const commands = @import("command");
const image = commands.image;

pub const TransferError = error{
    SyncFailed,
    TransferFailed,
    InvalidState,
    InvalidResponse,
    InvalidPacket,
    InvalidPattern,
    Timeout,
    NotImplemented,
};

pub const Transfer = struct {
    serial: *Serial,
    logger: *Logger,
    state: *StateMachine,
    buffer: [constants.MAX_PACKET_SIZE]u8,

    const Self = @This();

    pub fn init(serial: *Serial, logger: *Logger, state: *StateMachine) Self {
        return Self{
            .serial = serial,
            .logger = logger,
            .state = state,
            .buffer = undefined,
        };
    }

    /// Synchronize with the device
    pub fn sync(self: *Self) !void {
        const stdout = std.io.getStdOut().writer();
        try stdout.print("\nInitiating protocol sync...\n", .{});

        // Clear any pending data
        try self.serial.clearInput();

        // Enter sync state
        try self.state.transition(.syncing);

        // Send sync packet with version
        const sync_packet = try Packet.init(
            .SYNC,
            self.state.nextSequence(),
            &[_]u8{constants.VERSION},
        );

        var retry: u8 = 0;
        while (retry < constants.MAX_RETRIES) : (retry += 1) {
            if (retry > 0) {
                const delay = self.state.getRetryDelay();
                try stdout.print("\rRetry {}/{}: Waiting {}ms...", .{ retry + 1, constants.MAX_RETRIES, delay });
                std.time.sleep(delay * std.time.ns_per_ms);
            }

            // Send sync packet
            try self.sendPacket(sync_packet);

            // Wait for sync acknowledgment
            const response = self.receivePacket() catch |err| {
                try stdout.print("Sync error: {}\n", .{err});
                try self.state.incrementRetry();
                continue;
            };

            if (response.header.packet_type == .SYNC_ACK) {
                try self.state.transition(.ready);
                try stdout.print("\nSync established (Protocol v{})\n", .{constants.VERSION});
                self.state.resetRetry();
                return;
            }
        }

        try self.state.transition(.error_state);
        return error.SyncFailed;
    }

    /// Send a command to the device
    pub fn sendCommand(self: *Self, command: constants.Command) !void {
        if (self.state.current_state != .ready) {
            try self.sync();
        }

        try self.state.transition(.sending_command);

        const cmd_packet = try Packet.init(
            .CMD,
            self.state.nextSequence(),
            &[_]u8{@intFromEnum(command)},
        );

        try self.sendPacket(cmd_packet);

        const response = try self.receivePacket();
        if (response.header.packet_type != .ACK) {
            return error.InvalidResponse;
        }

        try self.state.transition(.ready);
    }

    /// Send data in chunks
    pub fn sendData(self: *Self, data: []const u8) !void {
        if (self.state.current_state != .ready) {
            return error.InvalidState;
        }

        const stdout = std.io.getStdOut().writer();
        var total_sent: usize = 0;

        while (total_sent < data.len) {
            const chunk_size = @min(constants.CHUNK_SIZE, data.len - total_sent);
            const chunk = data[total_sent .. total_sent + chunk_size];

            const data_packet = try Packet.init(
                .DATA,
                self.state.nextSequence(),
                chunk,
            );

            var retry: u8 = 0;
            while (retry < constants.MAX_RETRIES) : (retry += 1) {
                if (retry > 0) {
                    const delay = self.state.getRetryDelay();
                    try stdout.print("\rRetry {}/{}: Waiting {}ms...", .{ retry + 1, constants.MAX_RETRIES, delay });
                    std.time.sleep(delay * std.time.ns_per_ms);
                }

                try self.sendPacket(data_packet);

                const response = self.receivePacket() catch |err| {
                    try stdout.print("Transfer error: {}\n", .{err});
                    try self.state.incrementRetry();
                    continue;
                };

                switch (response.header.packet_type) {
                    .ACK => break,
                    .NACK => {
                        if (response.payload) |payload| {
                            try stdout.print("\nNACK received: {s}\n", .{payload});
                        }
                        try self.state.incrementRetry();
                        continue;
                    },
                    else => {
                        try self.state.incrementRetry();
                        continue;
                    },
                }
            }

            if (retry >= constants.MAX_RETRIES) {
                try self.state.transition(.error_state);
                return error.TransferFailed;
            }

            total_sent += chunk_size;
            self.state.resetRetry();

            const progress = @as(f32, @floatFromInt(total_sent)) / @as(f32, @floatFromInt(data.len)) * 100.0;
            try stdout.print("\rProgress: {d:.1}% ({}/{} bytes)", .{ progress, total_sent, data.len });
        }

        try stdout.print("\nTransfer complete!\n", .{});
    }

    /// Send a packet to the device
    fn sendPacket(self: *Self, packet: Packet) !void {
        const header_size = @sizeOf(PacketHeader);
        const header_bytes = std.mem.asBytes(&packet.header);
        _ = try self.serial.write(header_bytes[0..header_size]);

        if (packet.payload) |payload| {
            _ = try self.serial.write(payload);
        }
    }

    /// Receive a packet from the device
    fn receivePacket(self: *Self) !Packet {
        const header_size = @sizeOf(PacketHeader);
        const header_bytes = self.buffer[0..header_size];

        // Read header
        var total_read: usize = 0;
        while (total_read < header_size) {
            const bytes_read = try self.serial.read(header_bytes[total_read..header_size]);
            if (bytes_read == 0) {
                return error.Timeout;
            }
            total_read += bytes_read;
        }

        // Convert bytes to header struct
        const header = std.mem.bytesToValue(PacketHeader, header_bytes[0..@sizeOf(PacketHeader)]);

        // Validate payload length
        if (header.length > constants.MAX_PACKET_SIZE - header_size) {
            return error.InvalidPacket;
        }

        // Read payload if present
        var payload: ?[]const u8 = null;
        if (header.length > 0) {
            const payload_bytes = self.buffer[header_size .. header_size + header.length];
            total_read = 0;
            while (total_read < header.length) {
                const bytes_read = try self.serial.read(payload_bytes[total_read..header.length]);
                if (bytes_read == 0) {
                    return error.Timeout;
                }
                total_read += bytes_read;
            }
            payload = payload_bytes;
        }

        const packet = Packet{
            .header = header,
            .payload = payload,
        };

        // Verify CRC32
        const calculated_crc = try packet.calculateCRC32();
        if (calculated_crc != header.crc32) {
            return error.InvalidPacket;
        }

        return packet;
    }

    /// Send a test pattern to the device
    pub fn sendTestPattern(self: *Self, pattern: u8) !void {
        const cmd = switch (pattern) {
            1 => constants.Command.checkerboard,
            2 => constants.Command.stripes,
            3 => constants.Command.gradient,
            else => return error.InvalidPattern,
        };
        try self.sendCommand(cmd);
    }

    /// Send an image to the device
    pub fn sendImage(self: *Self, image_path: []const u8) !void {
        const stdout = std.io.getStdOut().writer();
        try stdout.print("Loading image from {s}...\n", .{image_path});

        // Start image transfer
        try self.sendCommand(constants.Command.image);

        // Load and validate PNG
        var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
        defer arena.deinit();
        const allocator = arena.allocator();

        // Decode PNG to RGB888
        const rgb888_data = try image.decodePNG(allocator, image_path);
        defer allocator.free(rgb888_data);

        // Convert to RGB565
        const rgb565_data = try image.convertToRGB565(allocator, rgb888_data, image.ImageSize.width, image.ImageSize.height);
        defer allocator.free(rgb565_data);

        // Send image data
        try self.sendData(rgb565_data);

        // End transfer
        try self.sendCommand(constants.Command.end);
        try stdout.print("Image transfer complete!\n", .{});
    }
};
