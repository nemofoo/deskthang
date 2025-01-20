#ifndef BOARD_H_
#define BOARD_H_

#ifdef __cplusplus
extern "C" {
#endif

// RHPort number used for device can be defined by board.mk
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT     0
#endif

// RHPort max operational speed can be defined by board.mk
#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED  OPT_MODE_FULL_SPEED
#endif

// Device mode configuration
#define CFG_TUD_ENABLED      1
#define CFG_TUD_MAX_SPEED    BOARD_TUD_MAX_SPEED

#include "../../../protocol/protocol_constants.h"

// CDC FIFO size of TX and RX
#define CFG_TUD_CDC_RX_BUFSIZE   MAX_PACKET_SIZE
#define CFG_TUD_CDC_TX_BUFSIZE   MAX_PACKET_SIZE

// CDC Endpoint transfer buffer size, more is faster
#define CFG_TUD_CDC_EP_BUFSIZE   64

//------------- CLASS -------------//
#define CFG_TUD_CDC              1
#define CFG_TUD_MSC              0
#define CFG_TUD_HID              0
#define CFG_TUD_MIDI             0
#define CFG_TUD_VENDOR          0

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H_ */
