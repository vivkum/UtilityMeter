/**
 * @file dlms_config.h
 * @brief Configuration settings for DLMS/COSEM Security Suite 0 Stack
 * 
 * Target: Microchip PIC32CXMTC Smart Metering Family
 * Standards: IEC 62056-5-3, IEC 62056-6-1, IEC 62056-6-2, IEC 62056-46, IEC 62056-47
 */

#ifndef DLMS_CONFIG_H
#define DLMS_CONFIG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* PDU Buffer & Memory Configuration                                         */
/* ========================================================================== */

/** Maximum APDU buffer size (bytes) - typical for smart metering */
#define DLMS_MAX_PDU_SIZE                   1024

/** Maximum HDLC information frame size */
#define DLMS_HDLC_MAX_INFO_SIZE             512

/** HDLC frame receive buffer size */
#define DLMS_HDLC_RX_BUF_SIZE               1024

/** HDLC frame transmit buffer size */
#define DLMS_HDLC_TX_BUF_SIZE               1024

/** Maximum block transfer data size per block */
#define DLMS_BLOCK_TRANSFER_SIZE            256

/* ========================================================================== */
/* Device Identification & Addressing                                        */
/* ========================================================================== */

/** Logical Device Name (16 bytes max string) */
#define DLMS_DEVICE_NAME                    "MCH_PIC32CX_001"

/** Meter Manufacturer ID (3 characters) */
#define DLMS_METER_MANUFACTURER_ID          "MCH"

/** Server System Title (8 bytes) - Identifies meter in Suite 0 ciphering */
#define DLMS_SERVER_SYSTEM_TITLE            { 'M', 'C', 'H', 0x00, 0x00, 0x00, 0x01, 0x01 }

/** Default Client System Title (8 bytes) */
#define DLMS_DEFAULT_CLIENT_SYSTEM_TITLE    { 'G', 'X', 'D', 0x00, 0x00, 0x00, 0x00, 0x01 }

/** HDLC Server Logical Device Address (Default: 1) */
#define DLMS_HDLC_SERVER_LOGICAL_ADDR       0x01

/** HDLC Server Physical Device Address (Default: 1) */
#define DLMS_HDLC_SERVER_PHYSICAL_ADDR      0x01

/** DLMS TCP/IP Wrapper default port (IEC 62056-47 standard: 4059) */
#define DLMS_WRAPPER_DEFAULT_PORT           4059

/* ========================================================================== */
/* Security Suite 0 Cryptographic Configuration                               */
/* ========================================================================== */

/** Security Suite: Suite 0 (0 = Suite 0: AES-GCM-128, HLS 5 GMAC, AES Key Wrap) */
#define DLMS_SECURITY_SUITE                 0

/** AES Key size in bytes (128-bit) */
#define DLMS_AES_KEY_SIZE                   16

/** Authentication Tag length in bytes for Suite 0 (12 bytes / 96-bit tag) */
#define DLMS_AUTH_TAG_SIZE                  12

/** Challenge length for HLS 5 (16 bytes) */
#define DLMS_HLS_CHALLENGE_SIZE             16

/** Invocation counter size (4 bytes) */
#define DLMS_INVOCATION_COUNTER_SIZE        4

/**
 * Default Global Unicast Encryption Key (128-bit / 16 bytes)
 * Used for AES-GCM message encryption/decryption
 */
#define DLMS_DEFAULT_GLOBAL_UNICAST_KEY \
    { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, \
      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F }

/**
 * Default Authentication Key (128-bit / 16 bytes)
 * Used for AES-GMAC authentication & HLS 5 handshake
 */
#define DLMS_DEFAULT_AUTHENTICATION_KEY \
    { 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, \
      0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF }

/**
 * Default Master Key / Key Encrypting Key (KEK) (128-bit / 16 bytes)
 * Used for AES Key Wrap when transferring new keys
 */
#define DLMS_DEFAULT_MASTER_KEY \
    { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, \
      0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF }

/**
 * Default Low Level Security (LLS) password
 */
#define DLMS_DEFAULT_LLS_PASSWORD           "Microchip123"

/* ========================================================================== */
/* Client Association SAPs                                                   */
/* ========================================================================== */

#define DLMS_SAP_PUBLIC_CLIENT              0x10    /**< Public client (Lowest level security) */
#define DLMS_SAP_READING_CLIENT             0x12    /**< Reading client (LLS security) */
#define DLMS_SAP_MANAGEMENT_CLIENT          0x01    /**< Management client (HLS 5 Suite 0) */
#define DLMS_SAP_PC_CLIENT                  0x13    /**< Optical / Field PC client */

/* ========================================================================== */
/* Profile Generic Buffer Limits                                              */
/* ========================================================================== */

/** Maximum entries stored in Load Survey Profile Generic buffer */
#define DLMS_LOAD_PROFILE_MAX_ENTRIES       96      /**< 24 hours at 15-min intervals */

/** Maximum entries in Daily Billing Profile Generic */
#define DLMS_DAILY_PROFILE_MAX_ENTRIES      30      /**< 30 days */

/** Maximum entries in Event Log Profile */
#define DLMS_EVENT_PROFILE_MAX_ENTRIES      50      /**< 50 events */

#ifdef __cplusplus
}
#endif

#endif /* DLMS_CONFIG_H */
