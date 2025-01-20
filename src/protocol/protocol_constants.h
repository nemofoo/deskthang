#ifndef PROTOCOL_CONSTANTS_H
#define PROTOCOL_CONSTANTS_H

// Protocol version
#define PROTOCOL_VERSION 1

// Buffer sizes
#define MAX_PACKET_SIZE 512
#define CHUNK_SIZE 256
#define HEADER_SIZE 8

// Timing constants
#define BASE_TIMEOUT_MS 1000
#define MIN_RETRY_DELAY_MS 50
#define MAX_RETRY_DELAY_MS 1000
#define MAX_RETRIES 8

#endif // PROTOCOL_CONSTANTS_H 