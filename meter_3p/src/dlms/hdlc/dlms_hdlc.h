/**
 * @file dlms_hdlc.h
 * @brief HDLC Data Link Layer Implementation (IEC 62056-46)
 */

#ifndef DLMS_HDLC_H
#define DLMS_HDLC_H

#include "dlms_types.h"
#include "dlms_config.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DLMS_HDLC_FLAG                  0x7E
#define DLMS_HDLC_LLC_DEST              0xE6
#define DLMS_HDLC_LLC_SRC               0xE6
#define DLMS_HDLC_LLC_QOS               0x00

/* HDLC Control Field Types */
#define DLMS_HDLC_CTRL_SNRM             0x93
#define DLMS_HDLC_CTRL_UA               0x73
#define DLMS_HDLC_CTRL_DISC             0x53
#define DLMS_HDLC_CTRL_DM               0x1F
#define DLMS_HDLC_CTRL_UI               0x03
#define DLMS_HDLC_CTRL_RR               0x01

/* Frame Type Flags */
typedef enum {
    DLMS_HDLC_FRAME_UNKNOWN             = 0,
    DLMS_HDLC_FRAME_SNRM                = 1,
    DLMS_HDLC_FRAME_DISC                = 2,
    DLMS_HDLC_FRAME_UA                  = 3,
    DLMS_HDLC_FRAME_DM                  = 4,
    DLMS_HDLC_FRAME_RR                  = 5,
    DLMS_HDLC_FRAME_I                   = 6,
    DLMS_HDLC_FRAME_UI                  = 7
} dlms_hdlc_frame_type_t;

/* Parsed HDLC Frame Header */
typedef struct {
    dlms_hdlc_frame_type_t type;
    uint16_t frame_len;
    bool     segmented;
    uint16_t dest_address;      /**< Server Address (Logical + Physical) */
    uint8_t  src_address;       /**< Client SAP */
    uint8_t  control;
    uint8_t  n_r;               /**< Receive sequence number */
    uint8_t  n_s;               /**< Send sequence number */
    bool     poll_final;
    const uint8_t *info_payload;/**< Pointer to LLC / xDLMS payload */
    uint16_t info_len;          /**< Length of payload excluding LLC header */
} dlms_hdlc_rx_frame_t;

/* HDLC Connection / Channel State Machine */
typedef struct {
    bool     connected;
    uint8_t  server_logical_addr;
    uint8_t  server_physical_addr;
    uint8_t  client_sap;
    uint8_t  send_seq;          /**< N(S) 0..7 */
    uint8_t  recv_seq;          /**< N(R) 0..7 */
    uint16_t max_rx_info_len;
    uint16_t max_tx_info_len;
    
    /* RX State & Buffer */
    uint8_t  rx_buf[DLMS_HDLC_RX_BUF_SIZE];
    uint16_t rx_idx;
    bool     rx_in_frame;
} dlms_hdlc_channel_t;

/** Initialize HDLC channel */
void dlms_hdlc_init(dlms_hdlc_channel_t *ch, uint8_t server_logical, uint8_t server_physical);

/**
 * @brief Feed incoming byte from UART / serial port into HDLC channel.
 * @param ch HDLC channel.
 * @param byte Incoming byte.
 * @param out_frame If a complete frame was parsed, filled with frame info.
 * @return true if a complete valid frame has been assembled, false otherwise.
 */
bool dlms_hdlc_feed_byte(dlms_hdlc_channel_t *ch, uint8_t byte, dlms_hdlc_rx_frame_t *out_frame);

/**
 * @brief Build a UA (Unnumbered Acknowledge) response frame.
 */
uint16_t dlms_hdlc_build_ua(
    const dlms_hdlc_channel_t *ch,
    uint8_t client_sap,
    uint8_t *out_buf,
    uint16_t max_out_len);

/**
 * @brief Build a DM (Disconnected Mode) response frame.
 */
uint16_t dlms_hdlc_build_dm(
    const dlms_hdlc_channel_t *ch,
    uint8_t client_sap,
    uint8_t *out_buf,
    uint16_t max_out_len);

/**
 * @brief Build an Information (I-frame) response frame carrying xDLMS APDU.
 */
uint16_t dlms_hdlc_build_iframe(
    dlms_hdlc_channel_t *ch,
    uint8_t client_sap,
    const uint8_t *apdu,
    uint16_t apdu_len,
    uint8_t *out_buf,
    uint16_t max_out_len);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_HDLC_H */
