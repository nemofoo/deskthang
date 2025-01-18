// main.zig
const std = @import("std");
const fs = std.fs;

pub fn main() !void {
    const stdout = std.io.getStdOut().writer();

    // Open serial port
    const file = fs.openFileAbsolute("/dev/ttyACM0", .{ .mode = .read_write }) catch |err| {
        try stdout.print("Failed to open serial port: {any}\n", .{err});
        return;
    };
    defer file.close();

    while (true) {
        try stdout.writeAll(
            \\Select pattern:
            \\1. Checkerboard
            \\2. Stripes
            \\3. Gradient
            \\4. Exit
            \\Choice: 
        );

        var buf: [2]u8 = undefined;
        const stdin = std.io.getStdIn().reader();
        if (try stdin.readUntilDelimiterOrEof(&buf, '\n')) |user_input| {
            if (user_input.len > 0) {
                switch (user_input[0]) {
                    '1'...'3' => {
                        // Send command directly to serial port
                        _ = try file.writeAll(&[_]u8{user_input[0]});
                        try stdout.print("Sent command: {c}\n", .{user_input[0]});
                    },
                    '4' => break,
                    else => try stdout.writeAll("Invalid choice\n"),
                }
            }
        }
    }
}
