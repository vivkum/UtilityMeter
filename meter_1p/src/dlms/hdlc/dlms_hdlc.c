/**
 * @file dlms_hdlc.c
 * @brief HDLC Data Link Layer Implementation
 */

#include "dlms_hdlc.h"
#include "dlms_crc.h"
#include <string.h>

void dlms_hdlc_init(dlms_hdlc_channel_t *ch, uint8_t server_logical, uint8_t server_physical) {
    memset(ch, 0, sizeof(dlms_hdlc_channel_t));
    ch->server_logical_addr = server_logical;
    ch->server_physical_addr = server_physical;
    ch->max_rx_info_len = DLMS_HDLC_MAX_INFO_SIZE;
    ch->max_tx_info_len = DLMS_HDLC_MAX_INFO_SIZE;
}

static uint8_t parse_address(const uint8_t *buf, uint16_t max_len, uint16_t *addr_val, uint8_t *addr_bytes) {
    if (max_len < 1) return 0;

    if ((buf[0] & 0x01) != 0) {
        /* 1-byte address */
        *addr_val = (buf[0] >> 1);
        *addr_bytes = 1;
        return 1;
    } else if (max_len >= 2 && ((buf[1] & 0x01) != 0)) {
        /* 2-byte address: Logical (buf[0]>>1), Physical (buf[1]>>1) */
        *addr_val = (uint16_t)(((buf[0] >> 1) << 8) | (buf[1] >> 1));
        *addr_bytes = 2;
        return 2;
    } else if (max_len >= 4 && ((buf[3] & 0x01) != 0)) {
        /* 4-byte address */
        *addr_val = (uint16_t)(((buf[0] >> 1) << 8) | (buf[3] >> 1));
        *addr_bytes = 4;
        return 4;
    }
    return 0;
}

bool dlms_hdlc_feed_byte(dlms_hdlc_channel_t *ch, uint8_t byte, dlms_hdlc_rx_frame_t *out_frame) {
    if (!ch->rx_in_frame) {
        if (byte == DLMS_HDLC_FLAG) {
            ch->rx_in_frame = true;
            ch->rx_idx = 0;
        }
        return false;
    }

    if (byte == DLMS_HDLC_FLAG) {
        if (ch->rx_idx == 0) {
            /* Repeated flag, keep waiting */
            return false;
        }

        /* Frame closing flag received */
        ch->rx_in_frame = false;

        if (ch->rx_idx < 7) {
            /* Minimum HDLC frame size is 7 bytes (Format 2, Dest 1, Src 1, Ctrl 1, FCS 2) */
            return false;
        }

        /* Verify overall FCS */
        uint16_t calc_fcs = dlms_crc16_calculate(ch->rx_buf, ch->rx_idx - 2);
        uint16_t rx_fcs = (uint16_t)ch->rx_buf[ch->rx_idx - 2] | ((uint16_t)ch->rx_buf[ch->rx_idx - 1] << 8);
        if (calc_fcs != rx_fcs) {
            return false; /* FCS checksum error */
        }

        /* Parse format field */
        uint16_t format = ((uint16_t)ch->rx_buf[0] << 8) | ch->rx_buf[1];
        if ((format & 0xF000) != 0xA000) {
            return false; /* Invalid frame type (must be 0xA) */
        }

        uint16_t frame_len = format & 0x07FF;
        if (frame_len != ch->rx_idx) {
            return false; /* Length mismatch */
        }

        memset(out_frame, 0, sizeof(dlms_hdlc_rx_frame_t));
        out_frame->frame_len = frame_len;
        out_frame->segmented = ((format & 0x0800) != 0);

        /* Parse Destination Address (Server) */
        uint16_t dest_val = 0;
        uint8_t dest_len = 0;
        uint16_t offset = 2;
        if (!parse_address(ch->rx_buf + offset, ch->rx_idx - offset, &dest_val, &dest_len)) {
            return false;
        }
        out_frame->dest_address = dest_val;
        offset += dest_len;

        /* Parse Source Address (Client SAP) */
        uint16_t src_val = 0;
        uint8_t src_len = 0;
        if (!parse_address(ch->rx_buf + offset, ch->rx_idx - offset, &src_val, &src_len)) {
            return false;
        }
        out_frame->src_address = (uint8_t)src_val;
        offset += src_len;

        /* Parse Control field */
        if (offset >= ch->rx_idx) return false;
        uint8_t ctrl = ch->rx_buf[offset++];
        out_frame->control = ctrl;
        out_frame->poll_final = ((ctrl & 0x10) != 0);

        /* Check for HCS (Header Check Sequence) */
        uint16_t header_len = offset;
        bool has_info = (ch->rx_idx > header_len + 4); /* Header + HCS(2) + FCS(2) */

        if (has_info) {
            uint16_t rx_hcs = (uint16_t)ch->rx_buf[offset] | ((uint16_t)ch->rx_buf[offset + 1] << 8);
            uint16_t calc_hcs = dlms_crc16_calculate(ch->rx_buf, header_len);
            if (calc_hcs != rx_hcs) {
                return false; /* HCS checksum error */
            }
            offset += 2; /* Skip HCS */
        }

        /* Classify frame */
        if ((ctrl & 0x01) == 0) {
            /* I-Frame (Information) */
            out_frame->type = DLMS_HDLC_FRAME_I;
            out_frame->n_s = (ctrl >> 1) & 0x07;
            out_frame->n_r = (ctrl >> 5) & 0x07;
        } else if ((ctrl & 0x03) == 0x01) {
            /* Supervisory: RR */
            out_frame->type = DLMS_HDLC_FRAME_RR;
            out_frame->n_r = (ctrl >> 5) & 0x07;
        } else if (ctrl == DLMS_HDLC_CTRL_SNRM) {
            out_frame->type = DLMS_HDLC_FRAME_SNRM;
        } else if (ctrl == DLMS_HDLC_CTRL_DISC) {
            out_frame->type = DLMS_HDLC_FRAME_DISC;
        } else if (ctrl == DLMS_HDLC_CTRL_UI) {
            out_frame->type = DLMS_HDLC_FRAME_UI;
        } else {
            out_frame->type = DLMS_HDLC_FRAME_UNKNOWN;
        }

        /* Extract Information Payload if present */
        if (has_info && (out_frame->type == DLMS_HDLC_FRAME_I || out_frame->type == DLMS_HDLC_FRAME_UI)) {
            uint16_t remaining = ch->rx_idx - offset - 2; /* Exclude trailing FCS */
            /* Verify LLC Header (0xE6 0xE6 0x00 or 0xE6 0xE7 0x00) */
            if (remaining >= 3 && ch->rx_buf[offset] == DLMS_HDLC_LLC_DEST) {
                out_frame->info_payload = ch->rx_buf + offset + 3;
                out_frame->info_len = remaining - 3;
            } else {
                out_frame->info_payload = ch->rx_buf + offset;
                out_frame->info_len = remaining;
            }
        }

        return true;
    }

    /* Store byte in buffer */
    if (ch->rx_idx < sizeof(ch->rx_buf)) {
        ch->rx_buf[ch->rx_idx++] = byte;
    } else {
        /* Buffer overflow, drop frame */
        ch->rx_in_frame = false;
        ch->rx_idx = 0;
    }

    return false;
}

uint16_t dlms_hdlc_build_ua(
    const dlms_hdlc_channel_t *ch,
    uint8_t client_sap,
    uint8_t *out_buf,
    uint16_t max_out_len)
{
    /*
     * Build UA frame with HDLC negotiation parameters:
     * Header: Flag (0x7E), Format (0xA0..), Dest (Client SAP), Src (Server Address), Control (0x73),
     * Info field: Max Info RX/TX, Window size
     */
    uint8_t temp[64];
    uint16_t idx = 0;

    /* Format field placeholder */
    temp[idx++] = 0xA0;
    temp[idx++] = 0x00;

    /* Destination: Client SAP (1 byte) */
    temp[idx++] = (uint8_t)((client_sap << 1) | 0x01);

    /* Source: Server Address (2 bytes: Logical + Physical) */
    temp[idx++] = (uint8_t)((ch->server_logical_addr << 1) | 0x00);
    temp[idx++] = (uint8_t)((ch->server_physical_addr << 1) | 0x01);

    /* Control: UA with Poll/Final bit = 1 (0x73) */
    temp[idx++] = DLMS_HDLC_CTRL_UA;

    /* Information field with negotiation parameters (Format Identifier 0x81 0x80) */
    temp[idx++] = 0x81;
    temp[idx++] = 0x80;
    temp[idx++] = 0x14; /* Length */

    /* Max info field length transmit (0x05 0x02 0x02 0x00 = 512) */
    temp[idx++] = 0x05; temp[idx++] = 0x02;
    temp[idx++] = (uint8_t)(ch->max_tx_info_len >> 8);
    temp[idx++] = (uint8_t)(ch->max_tx_info_len & 0xFF);

    /* Max info field length receive (0x06 0x02 0x02 0x00 = 512) */
    temp[idx++] = 0x06; temp[idx++] = 0x02;
    temp[idx++] = (uint8_t)(ch->max_rx_info_len >> 8);
    temp[idx++] = (uint8_t)(ch->max_rx_info_len & 0xFF);

    /* Window size transmit (0x07 0x01 0x01 = 1) */
    temp[idx++] = 0x07; temp[idx++] = 0x01; temp[idx++] = 0x01;

    /* Window size receive (0x08 0x01 0x01 = 1) */
    temp[idx++] = 0x08; temp[idx++] = 0x01; temp[idx++] = 0x01;

    /* Calculate HCS over header (format..control: 6 bytes) */
    uint16_t hcs = dlms_crc16_calculate(temp, 6);

    /* Insert HCS right after control */
    memmove(temp + 8, temp + 6, idx - 6);
    temp[6] = (uint8_t)(hcs & 0xFF);
    temp[7] = (uint8_t)(hcs >> 8);
    idx += 2;

    /* Total frame length = idx + 2 (FCS) */
    uint16_t frame_len = idx + 2;
    temp[0] = (uint8_t)(0xA0 | ((frame_len >> 8) & 0x07));
    temp[1] = (uint8_t)(frame_len & 0xFF);

    /* Recompute HCS because length changed */
    hcs = dlms_crc16_calculate(temp, 6);
    temp[6] = (uint8_t)(hcs & 0xFF);
    temp[7] = (uint8_t)(hcs >> 8);

    /* FCS over all bytes */
    uint16_t fcs = dlms_crc16_calculate(temp, idx);
    temp[idx++] = (uint8_t)(fcs & 0xFF);
    temp[idx++] = (uint8_t)(fcs >> 8);

    /* Copy to out_buf with flags */
    if (idx + 2 > max_out_len) return 0;
    out_buf[0] = DLMS_HDLC_FLAG;
    memcpy(out_buf + 1, temp, idx);
    out_buf[idx + 1] = DLMS_HDLC_FLAG;

    return idx + 2;
}

uint16_t dlms_hdlc_build_dm(
    const dlms_hdlc_channel_t *ch,
    uint8_t client_sap,
    uint8_t *out_buf,
    uint16_t max_out_len)
{
    uint8_t temp[16];
    uint16_t idx = 0;

    /* Format */
    temp[idx++] = 0xA0;
    temp[idx++] = 0x08; /* Total length = 8 */

    /* Destination: Client SAP */
    temp[idx++] = (uint8_t)((client_sap << 1) | 0x01);

    /* Source: Server Address (2 bytes) */
    temp[idx++] = (uint8_t)((ch->server_logical_addr << 1) | 0x00);
    temp[idx++] = (uint8_t)((ch->server_physical_addr << 1) | 0x01);

    /* Control: DM (0x1F) */
    temp[idx++] = DLMS_HDLC_CTRL_DM;

    /* FCS over bytes 0..5 */
    uint16_t fcs = dlms_crc16_calculate(temp, idx);
    temp[idx++] = (uint8_t)(fcs & 0xFF);
    temp[idx++] = (uint8_t)(fcs >> 8);

    if (idx + 2 > max_out_len) return 0;
    out_buf[0] = DLMS_HDLC_FLAG;
    memcpy(out_buf + 1, temp, idx);
    out_buf[idx + 1] = DLMS_HDLC_FLAG;

    return idx + 2;
}

uint16_t dlms_hdlc_build_iframe(
    dlms_hdlc_channel_t *ch,
    uint8_t client_sap,
    const uint8_t *apdu,
    uint16_t apdu_len,
    uint8_t *out_buf,
    uint16_t max_out_len)
{
    /*
     * Frame structure:
     * Flag: 0x7E
     * Format: 2 bytes (0xA0 | len)
     * Dest: 1 byte (Client SAP)
     * Src: 2 bytes (Server Logical + Physical)
     * Control: (N_R << 5) | (poll_final << 4) | (N_S << 1)
     * HCS: 2 bytes
     * LLC: 3 bytes (0xE6 0xE7 0x00)
     * APDU: apdu_len bytes
     * FCS: 2 bytes
     * Flag: 0x7E
     */
    uint16_t total_payload = 3 + apdu_len; /* LLC + APDU */
    uint16_t frame_len = 2 + 1 + 2 + 1 + 2 + total_payload + 2; /* 10 + payload */

    if (frame_len + 2 > max_out_len) return 0;

    uint16_t idx = 0;
    out_buf[idx++] = DLMS_HDLC_FLAG;

    /* Format */
    out_buf[idx++] = (uint8_t)(0xA0 | ((frame_len >> 8) & 0x07));
    out_buf[idx++] = (uint8_t)(frame_len & 0xFF);

    /* Destination: Client SAP */
    out_buf[idx++] = (uint8_t)((client_sap << 1) | 0x01);

    /* Source: Server Address */
    out_buf[idx++] = (uint8_t)((ch->server_logical_addr << 1) | 0x00);
    out_buf[idx++] = (uint8_t)((ch->server_physical_addr << 1) | 0x01);

    /* Control */
    uint8_t ctrl = (uint8_t)(((ch->recv_seq & 0x07) << 5) | 0x10 | ((ch->send_seq & 0x07) << 1));
    out_buf[idx++] = ctrl;
    ch->send_seq = (ch->send_seq + 1) & 0x07;

    /* HCS over header (out_buf[1] .. out_buf[idx-1]) */
    uint16_t hcs = dlms_crc16_calculate(out_buf + 1, idx - 1);
    out_buf[idx++] = (uint8_t)(hcs & 0xFF);
    out_buf[idx++] = (uint8_t)(hcs >> 8);

    /* LLC header */
    out_buf[idx++] = DLMS_HDLC_LLC_DEST;
    out_buf[idx++] = 0xE7; /* Response source LSAP */
    out_buf[idx++] = DLMS_HDLC_LLC_QOS;

    /* APDU */
    memcpy(out_buf + idx, apdu, apdu_len);
    idx += apdu_len;

    /* FCS over all bytes between opening and closing flags */
    uint16_t fcs = dlms_crc16_calculate(out_buf + 1, idx - 1);
    out_buf[idx++] = (uint8_t)(fcs & 0xFF);
    out_buf[idx++] = (uint8_t)(fcs >> 8);

    out_buf[idx++] = DLMS_HDLC_FLAG;

    return idx;
}
