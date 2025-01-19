pub const Protocol = struct {
    // Protocol version
    pub const VERSION: u8 = 1;

    // Packet types
    pub const PacketType = enum(u8) {
        sync = 0x1B,
        sync_ack = 0x1C,
        cmd = 0x1D,
        data = 0x1E,
        ack = 0x1F,
        nack = 0x20,
    };

    // Commands
    pub const Command = enum(u8) {
        checkerboard = '1',
        stripes = '2',
        gradient = '3',
        image = 'I',
        help = 'H',
        end = 'E',
    };

    // Protocol configuration
    pub const CHUNK_SIZE: usize = 256;
    pub const MAX_RETRIES: u8 = 8;
    pub const BASE_TIMEOUT_MS: u64 = 1000;
    pub const MIN_RETRY_DELAY_MS: u64 = 50;
    pub const MAX_RETRY_DELAY_MS: u64 = 1000;

    // Packet structure
    pub const MAX_PACKET_SIZE: usize = 512;
    pub const HEADER_SIZE: usize = 8;

    // Error set
    pub const Error = error{
        InvalidSync,
        InvalidChecksum,
        InvalidSequence,
        Timeout,
        NackReceived,
        InvalidPacketType,
    };

    // CRC32 calculation
    pub fn calculateCrc32(data: []const u8) u32 {
        var crc: u32 = 0xFFFFFFFF;
        for (data) |byte| {
            crc = (crc >> 8) ^ CRC_TABLE[(crc & 0xFF) ^ byte];
        }
        return ~crc;
    }
};

// CRC32 lookup table
const CRC_TABLE = [_]u32{
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    // ... (rest of table)
    0x2D02EF8D,
};

// Packet header structure
pub const PacketHeader = packed struct {
    type: Protocol.PacketType,
    sequence: u8,
    length: u16,
    checksum: u32,

    pub fn init(ptype: Protocol.PacketType, seq: u8, len: u16, crc: u32) PacketHeader {
        return .{
            .type = ptype,
            .sequence = seq,
            .length = len,
            .checksum = crc,
        };
    }
};
