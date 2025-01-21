const std = @import("std");

pub const MessageType = enum(u8) {
    Debug = 'D',
    Command = 'C',
    Image = 'I',
    Ack = 'A',
    Nack = 'N',
};

pub const Message = struct {
    msg_type: MessageType,
    sequence: u8,
    payload: []const u8,

    const Self = @This();

    pub fn format(self: Self, allocator: std.mem.Allocator) ![]u8 {
        // Calculate total message size: ~TYPE(1)SEQ(2):PAYLOAD#CRC\n
        const total_size = 1 + 1 + 2 + 1 + self.payload.len + 1 + 4 + 1;

        // Allocate buffer for complete message
        var buffer = try allocator.alloc(u8, total_size);
        errdefer allocator.free(buffer);

        // Format the message prefix
        var prefix = buffer[0..5];
        prefix[0] = '~';
        prefix[1] = @intFromEnum(self.msg_type);
        _ = try std.fmt.bufPrint(prefix[2..4], "{:0>2}", .{self.sequence});
        prefix[4] = ':';

        // Copy payload
        @memcpy(buffer[5..][0..self.payload.len], self.payload);

        // Add checksum marker
        buffer[5 + self.payload.len] = '#';

        // Calculate CRC32 of everything before the #
        var hasher = std.hash.Crc32.init();
        hasher.update(buffer[0 .. 5 + self.payload.len]);
        const checksum = hasher.final();

        // Add checksum and newline
        _ = try std.fmt.bufPrint(buffer[6 + self.payload.len ..], "{X:0>4}\n", .{checksum});

        return buffer;
    }

    pub fn parse(line: []const u8) !Self {
        if (line.len < 9 or line[0] != '~' or line[line.len - 1] != '\n') {
            return error.InvalidFormat;
        }

        // Parse message type
        const msg_type = std.meta.stringToEnum(MessageType, line[1..2]) orelse
            return error.InvalidType;

        // Parse sequence number
        const sequence = try std.fmt.parseInt(u8, line[2..4], 10);

        if (line[4] != ':') return error.InvalidFormat;

        // Find checksum marker
        const hash_pos = std.mem.indexOf(u8, line, "#") orelse
            return error.InvalidFormat;

        // Extract payload
        const payload = line[5..hash_pos];

        // Verify checksum
        var hasher = std.hash.Crc32.init();
        hasher.update(line[0..hash_pos]);
        const checksum = hasher.final();

        const expected = try std.fmt.parseInt(u32, line[hash_pos + 1 .. line.len - 1], 16);

        if (checksum != expected) return error.InvalidChecksum;

        return Self{
            .msg_type = msg_type,
            .sequence = sequence,
            .payload = payload,
        };
    }
};

pub const MessageError = error{
    InvalidFormat,
    InvalidType,
    InvalidChecksum,
    InvalidSequence,
};
