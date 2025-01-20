pub const VERSION: u8 = 1; // Only supported version

// Protocol configuration
pub const CHUNK_SIZE: usize = 256;
pub const MAX_RETRIES: u8 = 8;
pub const BASE_TIMEOUT_MS: u64 = 1000;
pub const MIN_RETRY_DELAY_MS: u64 = 50;
pub const MAX_RETRY_DELAY_MS: u64 = 1000;

// Packet structure limits
pub const MAX_PACKET_SIZE: usize = 512;
pub const HEADER_SIZE: usize = 8;

// Commands
pub const Command = enum(u8) {
    checkerboard = '1',
    stripes = '2',
    gradient = '3',
    image = 'I',
    help = 'H',
    end = 'E',
};
