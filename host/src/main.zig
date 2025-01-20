const std = @import("std");
const protocol = @import("protocol");
const Transfer = protocol.Transfer;
const Serial = protocol.Serial;
const Logger = protocol.Logger;
const StateMachine = protocol.StateMachine;

const Command = enum { pattern, image, help, ping, monitor };

const Args = struct { command: Command, value: ?[]const u8, device: []const u8 };

fn printUsage() void {
    std.debug.print(
        \\Usage: deskthang <command> [value] [--device path]
        \\Commands:
        \\  pattern <pattern>    Display a test pattern (1-9)
        \\  image <file>      Display an image from a PNG file
        \\  ping             Test connection (returns PONG)
        \\  monitor          Monitor raw serial data
        \\  help             Show this help message
        \\
        \\Options:
        \\  --device <path>  Serial device path (default: /dev/ttyACM0)
        \\
    , .{});
}

fn parseArgs(args: []const []const u8) !Args {
    if (args.len < 2) {
        printUsage();
        return error.InvalidArgs;
    }

    var result = Args{ .command = .help, .value = null, .device = "/dev/ttyACM0" };

    const cmd = args[1];
    if (std.mem.eql(u8, cmd, "pattern")) {
        if (args.len < 3) {
            std.debug.print("Error: test command requires a pattern number (1-9)\n", .{});
            return error.InvalidArgs;
        }
        result.command = .pattern;
        result.value = args[2];
    } else if (std.mem.eql(u8, cmd, "image")) {
        if (args.len < 3) {
            std.debug.print("Error: image command requires a file path\n", .{});
            return error.InvalidArgs;
        }
        result.command = .image;
        result.value = args[2];
    } else if (std.mem.eql(u8, cmd, "help")) {
        result.command = .help;
    } else if (std.mem.eql(u8, cmd, "ping")) {
        result.command = .ping;
    } else if (std.mem.eql(u8, cmd, "monitor")) {
        result.command = .monitor;
    } else {
        std.debug.print("Error: unknown command '{s}'\n", .{cmd});
        return error.InvalidArgs;
    }

    // Check for --device option
    var i: usize = 2;
    while (i < args.len - 1) : (i += 1) {
        if (std.mem.eql(u8, args[i], "--device")) {
            if (i + 1 >= args.len) {
                std.debug.print("Error: --device requires a path\n", .{});
                return error.InvalidArgs;
            }
            result.device = args[i + 1];
            break;
        }
    }

    return result;
}

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);

    const parsed_args = parseArgs(args) catch {
        std.process.exit(1);
    };

    if (parsed_args.command == .help) {
        printUsage();
        return;
    }

    // Initialize components
    var serial = try Serial.init(parsed_args.device);
    defer serial.deinit();

    var logger = try Logger.init("serial.log");
    defer logger.deinit();

    var state = StateMachine.init();

    var transfer = Transfer.init(&serial, &logger, &state);

    switch (parsed_args.command) {
        .pattern => {
            const pattern_number = std.fmt.parseInt(u8, parsed_args.value.?, 10) catch {
                std.debug.print("Error: invalid pattern number\n", .{});
                return error.InvalidArgs;
            };
            if (pattern_number < 1 or pattern_number > 9) {
                std.debug.print("Error: pattern must be between 1 and 9\n", .{});
                return error.InvalidArgs;
            }
            try transfer.sendTestPattern(pattern_number);
        },
        .image => {
            try transfer.sendImage(parsed_args.value.?);
        },
        .ping => {
            try transfer.sync();
        },
        .monitor => {
            const stdout = std.io.getStdOut().writer();
            try stdout.print("Monitoring serial data (Ctrl+C to exit)...\n\n", .{});

            var buffer: [1024]u8 = undefined;
            while (true) {
                const bytes_read = try serial.read(&buffer);
                if (bytes_read > 0) {
                    // Log raw bytes to file
                    try logger.logDebug("Raw bytes received: ", .{});
                    for (buffer[0..bytes_read]) |byte| {
                        try logger.logDebug("{X:0>2} ", .{byte});
                    }
                    try logger.logDebug("\n", .{});

                    // Print raw bytes in hex and ASCII to console
                    for (buffer[0..bytes_read]) |byte| {
                        try stdout.print("{X:0>2} ", .{byte});
                    }
                    try stdout.print("  |  ", .{});
                    for (buffer[0..bytes_read]) |byte| {
                        const c = if (std.ascii.isPrint(byte)) byte else '.';
                        try stdout.print("{c}", .{c});
                    }
                    try stdout.print("\n", .{});
                }
                std.time.sleep(10 * std.time.ns_per_ms); // Small delay to prevent busy-waiting
            }
        },
        .help => unreachable,
    }
}
