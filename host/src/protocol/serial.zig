const std = @import("std");
const c = @cImport({
    @cInclude("fcntl.h");
    @cInclude("termios.h");
    @cInclude("unistd.h");
});

pub const SerialError = error{
    DeviceNotFound,
    PermissionDenied,
    ConfigurationFailed,
    ReadError,
    WriteError,
    Timeout,
};

pub const Serial = struct {
    file: std.fs.File,

    const Self = @This();

    pub fn init(device_path: []const u8) !Self {
        const stdout = std.io.getStdOut().writer();
        try stdout.print("Opening serial device '{s}'...\n", .{device_path});

        const file = std.fs.openFileAbsolute(device_path, .{ .mode = .read_write }) catch |err| {
            try stdout.print("Failed to open serial device: {}\n", .{err});
            return switch (err) {
                error.AccessDenied => error.PermissionDenied,
                error.FileNotFound => error.DeviceNotFound,
                else => error.ConfigurationFailed,
            };
        };
        errdefer file.close();

        try stdout.print("Configuring serial port...\n", .{});

        // Configure for non-blocking I/O
        const flags = c.fcntl(@as(c_int, @intCast(file.handle)), c.F_GETFL, @as(c_int, 0));
        if (flags < 0) {
            try stdout.print("Failed to get file flags\n", .{});
            return error.ConfigurationFailed;
        }

        const result = c.fcntl(@as(c_int, @intCast(file.handle)), c.F_SETFL, @as(c_int, flags | c.O_NONBLOCK));
        if (result < 0) {
            try stdout.print("Failed to set non-blocking mode\n", .{});
            return error.ConfigurationFailed;
        }

        // Configure serial port settings
        var tty: c.termios = undefined;
        if (c.tcgetattr(@as(c_int, @intCast(file.handle)), &tty) != 0) {
            try stdout.print("Failed to get terminal attributes\n", .{});
            return error.ConfigurationFailed;
        }

        // 8N1 (8 bits, no parity, 1 stop bit)
        tty.c_cflag &= ~@as(c_uint, c.PARENB);
        tty.c_cflag &= ~@as(c_uint, c.CSTOPB);
        tty.c_cflag &= ~@as(c_uint, c.CSIZE);
        tty.c_cflag |= c.CS8;

        // No flow control
        tty.c_cflag &= ~@as(c_uint, c.CRTSCTS);

        // Turn on READ & ignore control lines (CLOCAL = 1)
        tty.c_cflag |= c.CREAD | c.CLOCAL;

        // Disable canonical mode
        tty.c_lflag &= ~@as(c_uint, c.ICANON);

        // Disable echo
        tty.c_lflag &= ~@as(c_uint, c.ECHO);
        tty.c_lflag &= ~@as(c_uint, c.ECHOE);
        tty.c_lflag &= ~@as(c_uint, c.ECHONL);

        // Disable interpretation of INTR, QUIT and SUSP
        tty.c_lflag &= ~@as(c_uint, c.ISIG);

        // Turn off software flow control
        tty.c_iflag &= ~@as(c_uint, c.IXON | c.IXOFF | c.IXANY);

        // Prevent special interpretation of output bytes
        tty.c_oflag &= ~@as(c_uint, c.OPOST);

        // Prevent conversion of newline to carriage return/line feed
        tty.c_oflag &= ~@as(c_uint, c.ONLCR);

        // Wait for up to 1 second (10 deciseconds)
        tty.c_cc[c.VTIME] = 10;

        // No minimum number of bytes for read
        tty.c_cc[c.VMIN] = 0;

        // Save tty settings
        if (c.tcsetattr(@as(c_int, @intCast(file.handle)), c.TCSANOW, &tty) != 0) {
            try stdout.print("Failed to set terminal attributes\n", .{});
            return error.ConfigurationFailed;
        }

        try stdout.print("Serial port configured successfully\n", .{});

        return Self{
            .file = file,
        };
    }

    pub fn deinit(self: *Self) void {
        self.file.close();
    }

    pub fn read(self: *Self, buffer: []u8) !usize {
        const bytes_read = self.file.read(buffer) catch |err| switch (err) {
            error.WouldBlock => return 0,
            else => return error.ReadError,
        };

        if (bytes_read > 0) {
            // Log raw bytes for debugging
            const stdout = std.io.getStdOut().writer();
            try stdout.print("Read {} bytes: ", .{bytes_read});
            for (buffer[0..bytes_read]) |byte| {
                try stdout.print("{X:0>2} ", .{byte});
            }
            try stdout.print("\n", .{});
        }

        return bytes_read;
    }

    pub fn write(self: *Self, data: []const u8) !usize {
        return self.file.write(data) catch |err| switch (err) {
            error.WouldBlock => return 0,
            else => return error.WriteError,
        };
    }

    pub fn flush(self: *Self) !void {
        try self.file.sync();
    }

    pub fn clearInput(self: *Self) !void {
        var buffer: [1024]u8 = undefined;
        while (true) {
            const bytes_read = try self.read(&buffer);
            if (bytes_read == 0) break;
        }
    }
};
