#ifndef PROTOCOL_CONSTANTS_H
#define PROTOCOL_CONSTANTS_H

/**
 * Protocol version number.
 * Must match between host and device for connection to succeed.
 * See protocol_architecture.md for version compatibility rules.
 */
#define PROTOCOL_VERSION 1

/**
 * Buffer size constants.
 * These values are critical for proper packet handling and must match
 * between host and device implementations.
 */
#define MAX_PACKET_SIZE 512  // Maximum size of a complete packet including header
#define CHUNK_SIZE 256      // Size of individual data transfer chunks
#define HEADER_SIZE 8       // Size of packet header in bytes

/**
 * Timing constants.
 * These values control protocol timing behavior including timeouts,
 * retry delays, and recovery attempts.
 * See protocol_architecture.md for timing requirements.
 */
#define BASE_TIMEOUT_MS 1000     // Base timeout for operations
#define MIN_RETRY_DELAY_MS 50    // Minimum delay between retry attempts
#define MAX_RETRY_DELAY_MS 1000  // Maximum delay between retry attempts
#define MAX_RETRIES 8            // Maximum number of retry attempts

#endif // PROTOCOL_CONSTANTS_H
