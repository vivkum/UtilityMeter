/**
 * @file dlms_association.h
 * @brief Application Association (AARQ/AARE, RLRQ/RLRE) and Handshake
 * 
 * Standards: IEC 62056-5-3, Green Book Ed 9/10
 */

#ifndef DLMS_ASSOCIATION_H
#define DLMS_ASSOCIATION_H

#include "dlms_types.h"
#include "dlms_cosem.h"
#include "dlms_security.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Standard DLMS Object Identifiers (OIDs) */
#define DLMS_APP_CONTEXT_LN_NO_CIPHER       1   /**< 2.16.756.5.8.1.1 */
#define DLMS_APP_CONTEXT_SN_NO_CIPHER       2   /**< 2.16.756.5.8.1.2 */
#define DLMS_APP_CONTEXT_LN_CIPHER          3   /**< 2.16.756.5.8.1.3 (Suite 0) */
#define DLMS_APP_CONTEXT_SN_CIPHER          4   /**< 2.16.756.5.8.1.4 */

/* Parsed AARQ Parameters */
typedef struct {
    uint8_t  app_context;
    uint8_t  auth_mechanism;
    uint8_t  calling_auth_val[64];
    uint16_t calling_auth_len;
    uint32_t proposed_conformance;
    uint16_t proposed_max_pdu_size;
    bool     has_user_info;
} dlms_aarq_params_t;

/**
 * @brief Parse incoming AARQ (Association Request).
 */
bool dlms_association_parse_aarq(
    const uint8_t *apdu, uint16_t apdu_len,
    dlms_aarq_params_t *params);

/**
 * @brief Process AARQ and generate AARE response for an association.
 */
dlms_result_t dlms_association_process_aarq(
    dlms_association_t *assoc,
    const dlms_aarq_params_t *aarq,
    uint8_t *out_aare, uint16_t max_out_len, uint16_t *actual_len);

/**
 * @brief Process Release Request (RLRQ) and generate Release Response (RLRE).
 */
dlms_result_t dlms_association_process_rlrq(
    dlms_association_t *assoc,
    const uint8_t *in_rlrq, uint16_t in_len,
    uint8_t *out_rlre, uint16_t max_out_len, uint16_t *actual_len);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_ASSOCIATION_H */
