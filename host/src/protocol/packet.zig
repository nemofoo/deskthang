const std = @import("std");

pub const PacketType = enum(u8) {
    SYNC = 0x1B,
    SYNC_ACK = 0x1C,
    CMD = 0x1D,
    DATA = 0x1E,
    ACK = 0x1F,
    NACK = 0x20,
};

pub const PacketHeader = packed struct {
    marker: u8 = 0xAA, // Start marker
    packet_type: PacketType,
    sequence: u8,
    length: u16,
    crc32: u32,
};

pub const Packet = struct {
    header: PacketHeader,
    payload: ?[]const u8,

    const Self = @This();

    pub fn init(packet_type: PacketType, sequence: u8, payload: ?[]const u8) !Self {
        const payload_len: u16 = if (payload) |p| @intCast(p.len) else 0;

        var packet = Self{
            .header = PacketHeader{
                .packet_type = packet_type,
                .sequence = sequence,
                .length = payload_len,
                .crc32 = 0, // Will be calculated later
            },
            .payload = payload,
        };

        // Calculate CRC32 over header (excluding crc field) and payload
        packet.header.crc32 = try packet.calculateCRC32();
        return packet;
    }

    pub fn calculateCRC32(self: Self) !u32 {
        var hasher = std.hash.Crc32.init();

        // Hash header fields (excluding CRC32)
        const header_size = @sizeOf(PacketHeader) - @sizeOf(u32);
        const header_bytes = std.mem.asBytes(&self.header)[0..header_size];
        hasher.update(header_bytes);

        // Hash payload if present
        if (self.payload) |payload| {
            hasher.update(payload);
        }

        return hasher.final();
    }

    pub fn validate(self: Self) !void {
        // Check marker
        if (self.header.marker != 0xAA) {
            return error.InvalidMarker;
        }

        // Validate CRC32
        const calculated_crc = try self.calculateCRC32();
        if (calculated_crc != self.header.crc32) {
            return error.InvalidCRC;
        }

        // Validate payload length
        if (self.payload) |payload| {
            if (payload.len != self.header.length) {
                return error.InvalidLength;
            }
        } else if (self.header.length != 0) {
            return error.InvalidLength;
        }
    }

    pub fn format(
        self: Self,
        comptime fmt: []const u8,
        options: std.fmt.FormatOptions,
        writer: anytype,
    ) !void {
        _ = fmt;
        _ = options;

        try writer.print("{s} seq={} len={} crc=0x{X:0>8}", .{
            @tagName(self.header.packet_type),
            self.header.sequence,
            self.header.length,
            self.header.crc32,
        });

        if (self.payload) |payload| {
            try writer.print(" payload=\"{s}\"", .{payload});
        }
    }
};

pub const PacketError = error{
    InvalidMarker,
    InvalidCRC,
    InvalidLength,
    InvalidPacketType,
    BufferTooSmall,
};
