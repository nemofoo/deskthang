const std = @import("std");
const constants = @import("constants.zig");
const Packet = @import("packet.zig").Packet;
const PacketType = @import("packet.zig").PacketType;

pub const State = enum(u8) {
    idle,
    syncing,
    ready,
    sending_command,
    receiving_data,
    error_state,
};

pub const StateError = error{
    InvalidTransition,
    InvalidState,
    Timeout,
};

pub const StateMachine = struct {
    current_state: State,
    sequence: u8,
    retry_count: u8,

    const Self = @This();

    pub fn init() Self {
        return Self{
            .current_state = .idle,
            .sequence = 0,
            .retry_count = 0,
        };
    }

    pub fn transition(self: *Self, to: State) !void {
        switch (self.current_state) {
            .idle => {
                if (to != .syncing) return error.InvalidTransition;
            },
            .syncing => {
                if (to != .ready and to != .error_state) return error.InvalidTransition;
            },
            .ready => {
                if (to != .sending_command and to != .error_state) return error.InvalidTransition;
            },
            .sending_command => {
                if (to != .receiving_data and to != .ready and to != .error_state) return error.InvalidTransition;
            },
            .receiving_data => {
                if (to != .ready and to != .error_state) return error.InvalidTransition;
            },
            .error_state => {
                if (to != .idle) return error.InvalidTransition;
            },
        }

        self.current_state = to;
    }

    pub fn nextSequence(self: *Self) u8 {
        const current = self.sequence;
        self.sequence = self.sequence +% 1; // Wrapping add
        return current;
    }

    pub fn incrementRetry(self: *Self) !void {
        self.retry_count += 1;
        if (self.retry_count > constants.MAX_RETRIES) {
            try self.transition(.error_state);
            return error.Timeout;
        }
    }

    pub fn resetRetry(self: *Self) void {
        self.retry_count = 0;
    }

    pub fn getRetryDelay(self: Self) u64 {
        const base_delay = constants.MIN_RETRY_DELAY_MS;
        const max_delay = constants.MAX_RETRY_DELAY_MS;

        // Exponential backoff: delay = base * 2^retry_count
        const delay = base_delay * (@as(u64, 1) << @min(self.retry_count, 6));
        return @min(delay, max_delay);
    }
};
