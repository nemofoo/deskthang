const std = @import("std");
const Packet = @import("packet.zig").Packet;

pub const Direction = enum {
    TX,
    RX,
    DBG,
};

pub const Logger = struct {
    file: std.fs.File,

    const Self = @This();

    pub fn init(log_path: []const u8) !Self {
        const file = try std.fs.cwd().createFile(log_path, .{
            .read = true,
            .truncate = false,
        });

        return Self{
            .file = file,
        };
    }

    pub fn deinit(self: *Self) void {
        self.file.close();
    }

    pub fn logPacket(self: *Self, direction: Direction, packet: Packet) !void {
        const timestamp = std.time.timestamp();
        try self.file.writer().print("[{d}] {s}: {}\n", .{ timestamp, @tagName(direction), packet });
    }

    pub fn logDebug(self: *Self, comptime format: []const u8, args: anytype) !void {
        const timestamp = std.time.timestamp();
        try self.file.writer().print("[{d}] DEBUG: " ++ format ++ "\n", .{timestamp} ++ args);
    }

    pub fn logError(self: *Self, comptime format: []const u8, args: anytype) !void {
        const timestamp = std.time.timestamp();
        try self.file.writer().print("[{d}] ERROR: " ++ format ++ "\n", .{timestamp} ++ args);
    }
};
