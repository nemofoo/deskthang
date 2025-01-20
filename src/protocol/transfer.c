#include "../system/time.h"
#include "transfer.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../error/logging.h"
#include "../error/error.h"
#include "packet.h"
#include "../hardware/GC9A01.h"
#include "../hardware/display.h"
#include "../common/deskthang_constants.h"

// External declarations
extern const uint32_t crc32_table[256];

// Forward declarations of static functions
static bool transfer_process_image(void);
static void transfer_cleanup(void);

// Global transfer context
static TransferContext g_transfer_context;
static TransferStatus g_transfer_status;
static bool transfer_initialized = false;

// Helper macro
#define MIN(a,b) ((a) < (b) ? (a) : (b))

// Initialize transfer system
bool transfer_init(void) {
    memset(&g_transfer_context, 0, sizeof(TransferContext));
    g_transfer_context.mode = TRANSFER_MODE_NONE;
    g_transfer_context.state = TRANSFER_STATE_IDLE;
    transfer_initialized = true;
    return true;
}

bool transfer_is_initialized(void) {
    return transfer_initialized;
}

// Reset transfer state
void transfer_reset(void) {
    transfer_free_buffer();
    memset(&g_transfer_context, 0, sizeof(TransferContext));
    memset(&g_transfer_status, 0, sizeof(TransferStatus));
}

// Get transfer context
TransferContext *transfer_get_context(void) {
    return &g_transfer_context;
}

// Start new transfer
bool transfer_start(TransferMode mode, uint32_t total_size) {
    if (g_transfer_context.state != TRANSFER_STATE_IDLE) {
        return false;
    }
    
    // Allocate transfer buffer
    if (!transfer_allocate_buffer(total_size)) {
        return false;
    }
    
    // Initialize transfer context
    g_transfer_context.mode = mode;
    g_transfer_context.state = TRANSFER_STATE_STARTING;
    g_transfer_context.start_time = deskthang_time_get_ms();
    g_transfer_context.bytes_expected = total_size;
    g_transfer_context.chunks_expected = (total_size + CHUNK_SIZE - 1) / CHUNK_SIZE;
    
    // Initialize status
    g_transfer_status.active = true;
    g_transfer_status.progress = 0.0f;
    g_transfer_status.speed_bps = 0;
    g_transfer_status.errors = 0;
    
    return true;
}

// Process incoming data chunk
bool transfer_process_chunk(const Packet *packet) {
    if (!packet || g_transfer_context.state != TRANSFER_STATE_IN_PROGRESS) {
        return false;
    }
    
    // Validate packet
    if (!transfer_validate_chunk(packet)) {
        g_transfer_status.errors++;
        return false;
    }
    
    // Get chunk data
    const uint8_t *data = packet_get_payload(packet);
    uint16_t length = packet_get_length(packet);
    
    // Check buffer space
    if (g_transfer_context.buffer_offset + length > g_transfer_context.buffer_size) {
        g_transfer_status.errors++;
        return false;
    }
    
    // Copy data to buffer
    memcpy(g_transfer_context.buffer + g_transfer_context.buffer_offset, data, length);
    g_transfer_context.buffer_offset += length;
    g_transfer_context.bytes_received += length;
    g_transfer_context.chunks_received++;
    
    // Update status
    g_transfer_status.progress = transfer_get_progress();
    g_transfer_status.speed_bps = transfer_get_elapsed_time() > 0 ?
        (g_transfer_context.bytes_received * 1000) / transfer_get_elapsed_time() : 0;
    
    return true;
}

// Complete transfer
bool transfer_complete(void) {
    if (g_transfer_context.state != TRANSFER_STATE_IN_PROGRESS) {
        return false;
    }
    
    // Verify all data received
    if (g_transfer_context.bytes_received != g_transfer_context.bytes_expected) {
        return false;
    }
    
    g_transfer_context.state = TRANSFER_STATE_COMPLETING;
    
    // Process complete transfer buffer based on mode
    bool success = false;
    switch (g_transfer_context.mode) {
        case TRANSFER_MODE_IMAGE:
            success = transfer_process_image();
            break;
        default:
            success = false;
            break;
    }
    
    if (!success) {
        transfer_abort();
        return false;
    }
    
    // Update status
    g_transfer_status.active = false;
    g_transfer_status.progress = 1.0f;
    
    // Cleanup
    transfer_cleanup();
    
    return true;
}

// Process image data and update display
static bool transfer_process_image(void) {
    // Validate buffer and context
    if (!g_transfer_context.buffer || g_transfer_context.buffer_size == 0) {
        logging_write("Transfer", "Invalid buffer or size");
        return false;
    }
    
    const size_t expected_size = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2; // 2 bytes per pixel (RGB565)
    if (g_transfer_context.buffer_size != expected_size) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Invalid buffer size: got %u, expected %u", 
                 g_transfer_context.buffer_size, expected_size);
        logging_write("Transfer", msg);
        return false;
    }
    
    // Ensure display is ready
    if (!display_ready()) {
        logging_write("Transfer", "Display not ready for update");
        return false;
    }
    
    // Set up frame for full display update
    struct GC9A01_frame frame = {
        .start = {0, 0},
        .end = {DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1}
    };
    GC9A01_set_frame(frame);
    
    // Write image data directly to display
    uint32_t bytes_written = 0;
    while (bytes_written < g_transfer_context.buffer_size) {
        uint32_t chunk_size = MIN(CHUNK_SIZE, g_transfer_context.buffer_size - bytes_written);
        if (!display_write_data(g_transfer_context.buffer + bytes_written, chunk_size)) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Display write failed at offset %u", bytes_written);
            logging_write("Transfer", msg);
            return false;
        }
        bytes_written += chunk_size;
    }
    
    // Finalize display update
    if (!display_end_write()) {
        logging_write("Transfer", "Display failed to process update");
        return false;
    }
    
    // Clear the buffer since we're done with it
    memset(g_transfer_context.buffer, 0, g_transfer_context.buffer_size);
    
    char msg[64];
    snprintf(msg, sizeof(msg), "Image transfer complete: %u bytes written", bytes_written);
    logging_write("Transfer", msg);
    return true;
}

// Cleanup after transfer completion
static void transfer_cleanup(void) {
    // Free transfer buffer
    transfer_free_buffer();
    
    // Reset transfer context
    g_transfer_context.mode = TRANSFER_MODE_NONE;
    g_transfer_context.state = TRANSFER_STATE_IDLE;
    g_transfer_context.bytes_received = 0;
    g_transfer_context.bytes_expected = 0;
    g_transfer_context.chunks_received = 0;
    g_transfer_context.chunks_expected = 0;
    g_transfer_context.error_count = 0;
    g_transfer_context.retry_count = 0;
    g_transfer_context.last_sequence = 0;
    g_transfer_context.last_checksum = 0;
    
    // Clear status
    memset(&g_transfer_status, 0, sizeof(TransferStatus));
}

// Abort transfer
bool transfer_abort(void) {
    if (g_transfer_context.state == TRANSFER_STATE_IDLE) {
        return false;
    }
    
    g_transfer_context.state = TRANSFER_STATE_ERROR;
    g_transfer_status.active = false;
    
    transfer_reset();
    return true;
}

// Buffer management
bool transfer_allocate_buffer(uint32_t size) {
    transfer_free_buffer();
    
    g_transfer_context.buffer = (uint8_t *)malloc(size);
    if (!g_transfer_context.buffer) {
        return false;
    }
    
    g_transfer_context.buffer_size = size;
    g_transfer_context.buffer_offset = 0;
    
    return true;
}

void transfer_free_buffer(void) {
    if (g_transfer_context.buffer) {
        free(g_transfer_context.buffer);
        g_transfer_context.buffer = NULL;
    }
    g_transfer_context.buffer_size = 0;
    g_transfer_context.buffer_offset = 0;
}

uint8_t *transfer_get_buffer(void) {
    return g_transfer_context.buffer;
}

uint32_t transfer_get_buffer_size(void) {
    return g_transfer_context.buffer_size;
}

// Progress tracking
float transfer_get_progress(void) {
    if (g_transfer_context.bytes_expected == 0) {
        return 0.0f;
    }
    return (float)g_transfer_context.bytes_received / g_transfer_context.bytes_expected;
}

uint32_t transfer_get_remaining_bytes(void) {
    return g_transfer_context.bytes_expected - g_transfer_context.bytes_received;
}

uint32_t transfer_get_elapsed_time(void) {
    return deskthang_time_get_ms() - g_transfer_context.start_time;
}

uint32_t transfer_get_estimated_time_remaining(void) {
    uint32_t elapsed = transfer_get_elapsed_time();
    if (elapsed == 0 || g_transfer_context.bytes_received == 0) {
        return 0;
    }
    
    float bytes_per_ms = (float)g_transfer_context.bytes_received / elapsed;
    return (uint32_t)(transfer_get_remaining_bytes() / bytes_per_ms);
}

// Validation
bool transfer_validate_chunk(const Packet *packet) {
    if (!packet || packet_get_type(packet) != PACKET_TYPE_DATA) {
        return false;
    }
    
    // Validate sequence
    if (!transfer_validate_sequence(packet_get_sequence(packet))) {
        return false;
    }
    
    // Validate checksum
    if (!transfer_validate_checksum(packet_get_payload(packet),
                                  packet_get_length(packet),
                                  packet_get_checksum(packet))) {
        return false;
    }
    
    return true;
}

bool transfer_validate_sequence(uint8_t sequence) {
    // Handle wraparound case
    if (g_transfer_context.last_sequence == 255) {
        if (sequence == 0) {
            g_transfer_context.last_sequence = sequence;
            return true;
        }
        return false;
    }
    
    if (sequence == g_transfer_context.last_sequence + 1) {
        g_transfer_context.last_sequence = sequence;
        return true;
    }
    return false;
}

bool transfer_validate_checksum(const uint8_t *data, uint16_t length, uint32_t checksum) {
    if (!data || length == 0) {
        return false;
    }
    
    uint32_t crc = 0xFFFFFFFF;
    
    for (uint16_t i = 0; i < length; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
    }
    
    crc = crc ^ 0xFFFFFFFF;
    
    return crc == checksum;
}

// Error handling
bool transfer_handle_error(ErrorType error) {
    g_transfer_status.errors++;
    g_transfer_context.error_count++;
    
    if (g_transfer_context.error_count > MAX_RETRIES) {
        transfer_abort();
        return false;
    }
    
    return transfer_should_retry();
}

bool transfer_should_retry(void) {
    return g_transfer_context.retry_count < MAX_RETRIES;
}

uint32_t transfer_get_retry_delay(void) {
    return protocol_calculate_backoff(g_transfer_context.retry_count);
}

// Status reporting
TransferStatus *transfer_get_status(void) {
    return &g_transfer_status;
}

void transfer_update_status(const char *message) {
    if (message) {
        strncpy(g_transfer_status.message, message, sizeof(g_transfer_status.message) - 1);
    }
}

// Debug support
void transfer_print_status(void) {
    printf("Transfer Status:\n");
    printf("  Mode: %s\n", transfer_mode_to_string(g_transfer_context.mode));
    printf("  State: %s\n", transfer_state_to_string(g_transfer_context.state));
    printf("  Progress: %.1f%%\n", g_transfer_status.progress * 100.0f);
    printf("  Speed: %u bytes/sec\n", g_transfer_status.speed_bps);
    printf("  Bytes: %u/%u\n", g_transfer_context.bytes_received, g_transfer_context.bytes_expected);
    printf("  Chunks: %u/%u\n", g_transfer_context.chunks_received, g_transfer_context.chunks_expected);
    printf("  Errors: %u\n", g_transfer_status.errors);
    if (g_transfer_status.message[0]) {
        printf("  Message: %s\n", g_transfer_status.message);
    }
}

const char *transfer_mode_to_string(TransferMode mode) {
    switch (mode) {
        case TRANSFER_MODE_NONE:     return "NONE";
        case TRANSFER_MODE_IMAGE:    return "IMAGE";
        default:                     return "UNKNOWN";
    }
}

const char *transfer_state_to_string(TransferState state) {
    switch (state) {
        case TRANSFER_STATE_IDLE:        return "IDLE";
        case TRANSFER_STATE_STARTING:    return "STARTING";
        case TRANSFER_STATE_IN_PROGRESS: return "IN_PROGRESS";
        case TRANSFER_STATE_COMPLETING:  return "COMPLETING";
        case TRANSFER_STATE_ERROR:       return "ERROR";
        default:                         return "UNKNOWN";
    }
}

// Add these implementations
bool transfer_buffer_available(void) {
    return g_transfer_context.buffer != NULL && 
           g_transfer_context.buffer_size > 0 && 
           g_transfer_context.buffer_offset < g_transfer_context.buffer_size;
}

bool transfer_sequence_valid(void) {
    // Check if we're in a valid transfer state
    if (g_transfer_context.state != TRANSFER_STATE_IN_PROGRESS &&
        g_transfer_context.state != TRANSFER_STATE_STARTING) {
        return false;
    }
    
    // Check if we've received all expected chunks
    if (g_transfer_context.chunks_received >= g_transfer_context.chunks_expected) {
        return false;
    }
    
    return true;
}

bool transfer_checksum_valid(void) {
    // This is a higher-level function that validates the entire transfer buffer
    if (!g_transfer_context.buffer || g_transfer_context.buffer_size == 0) {
        return false;
    }
    
    // Calculate CRC32 of entire buffer
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < g_transfer_context.buffer_offset; i++) {
        uint8_t byte = g_transfer_context.buffer[i];
        crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ byte];
    }
    crc = crc ^ 0xFFFFFFFF;
    
    // Compare with stored checksum
    return crc == g_transfer_context.last_checksum;
}
