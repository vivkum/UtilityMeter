/**
 * @file dlms_security.h
 * @brief DLMS/COSEM Security Suite 0 definitions, keys, and security control
 * 
 * Standards: IEC 62056-5-3, Green Book Ed 9/10
 */

#ifndef DLMS_SECURITY_H
#define DLMS_SECURITY_H

#include "dlms_config.h"
#include "dlms_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Security Suite Definitions                                                 */
/* ========================================================================== */

typedef enum {
    DLMS_SECURITY_SUITE_0               = 0,    /**< AES-GCM-128, HLS 5 GMAC, AES Key Wrap */
    DLMS_SECURITY_SUITE_1               = 1,    /**< ECDSA P-256, SHA-256, AES-GCM-128 */
    DLMS_SECURITY_SUITE_2               = 2     /**< ECDSA P-384, SHA-384, AES-GCM-256 */
} dlms_security_suite_t;

/* ========================================================================== */
/* Security Control Byte (SC) Subfields (Green Book Sec 9.2.7.1)             */
/* ========================================================================== */

#define DLMS_SC_SUITE_MASK              0x0F    /**< Bits 0..3: Security Suite ID (0 = Suite 0) */
#define DLMS_SC_AUTH                    0x10    /**< Bit 4: Authentication required (A=1) */
#define DLMS_SC_ENC                     0x20    /**< Bit 5: Encryption required (E=1) */
#define DLMS_SC_BROADCAST_KEY           0x40    /**< Bit 6: 0 = Unicast key, 1 = Broadcast key */
#define DLMS_SC_COMPRESSION             0x80    /**< Bit 7: Compression active */

/* Standard SC byte values for Suite 0 */
#define DLMS_SC_NONE                    0x00
#define DLMS_SC_SUITE0_AUTH_ONLY        (DLMS_SC_AUTH)                                  /* 0x10 */
#define DLMS_SC_SUITE0_ENC_ONLY         (DLMS_SC_ENC)                                   /* 0x20 */
#define DLMS_SC_SUITE0_AUTH_ENC         (DLMS_SC_AUTH | DLMS_SC_ENC)                    /* 0x30 */
#define DLMS_SC_SUITE0_BCAST_AUTH_ENC   (DLMS_SC_AUTH | DLMS_SC_ENC | DLMS_SC_BROADCAST_KEY) /* 0x70 */

/* ========================================================================== */
/* Security Policy Flags for Class 64 (Security Setup)                       */
/* ========================================================================== */

typedef enum {
    DLMS_SEC_POLICY_NOTHING             = 0x00,
    DLMS_SEC_POLICY_REQ_AUTH            = (1 << 0), /**< Bit 0: Authenticated request */
    DLMS_SEC_POLICY_REQ_ENC             = (1 << 1), /**< Bit 1: Encrypted request */
    DLMS_SEC_POLICY_REQ_SIGN            = (1 << 2), /**< Bit 2: Digitally signed request */
    DLMS_SEC_POLICY_RESP_AUTH           = (1 << 3), /**< Bit 3: Authenticated response */
    DLMS_SEC_POLICY_RESP_ENC            = (1 << 4), /**< Bit 4: Encrypted response */
    DLMS_SEC_POLICY_RESP_SIGN           = (1 << 5), /**< Bit 5: Digitally signed response */
} dlms_security_policy_t;

/* ========================================================================== */
/* Authentication Mechanisms                                                 */
/* ========================================================================== */

typedef enum {
    DLMS_AUTH_NONE                      = 0,    /**< 2.16.756.5.8.2.0: Lowest Level Security (No Auth) */
    DLMS_AUTH_LLS                       = 1,    /**< 2.16.756.5.8.2.1: Low Level Security (Password) */
    DLMS_AUTH_HLS_2                     = 2,
    DLMS_AUTH_HLS_MD5                   = 3,    /**< 2.16.756.5.8.2.3: HLS MD5 */
    DLMS_AUTH_HLS_SHA1                  = 4,    /**< 2.16.756.5.8.2.4: HLS SHA-1 */
    DLMS_AUTH_HLS_GMAC                  = 5,    /**< 2.16.756.5.8.2.5: HLS 5 GMAC (Security Suite 0) */
    DLMS_AUTH_HLS_SHA256                = 6,    /**< 2.16.756.5.8.2.6: HLS SHA-256 */
    DLMS_AUTH_HLS_ECDSA                 = 7     /**< 2.16.756.5.8.2.7: HLS ECDSA */
} dlms_auth_mechanism_t;

/* ========================================================================== */
/* Security Context & Keys Structure                                         */
/* ========================================================================== */

typedef struct {
    uint8_t global_unicast_key[DLMS_AES_KEY_SIZE];   /**< Global Unicast Key (EK) */
    uint8_t authentication_key[DLMS_AES_KEY_SIZE];   /**< Authentication Key (AK) */
    uint8_t master_key[DLMS_AES_KEY_SIZE];           /**< Master Key / Key Encrypting Key (KEK) */
    uint8_t broadcast_key[DLMS_AES_KEY_SIZE];        /**< Broadcast Key (GBEK) */
    
    uint8_t server_system_title[8];                  /**< Meter / Server System Title */
    uint8_t client_system_title[8];                  /**< Current Client System Title */
    
    uint32_t server_invocation_counter;              /**< Tx Invocation counter (monotonically increments) */
    uint32_t client_invocation_counter;              /**< Rx Invocation counter (replay prevention) */
    
    uint8_t security_suite;                          /**< 0 = Suite 0 */
    uint8_t security_policy;                         /**< Active policy bitmask */
    
    /* HLS 5 Handshake Challenges */
    uint8_t c_to_s_challenge[DLMS_HLS_CHALLENGE_SIZE]; /**< Challenge sent by client */
    uint8_t s_to_c_challenge[DLMS_HLS_CHALLENGE_SIZE]; /**< Challenge sent by server */
    bool    challenge_pending;                       /**< Waiting for HLS reply */
} dlms_security_context_t;

#ifdef __cplusplus
}
#endif

#endif /* DLMS_SECURITY_H */
