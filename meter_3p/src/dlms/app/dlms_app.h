/**
 * @file dlms_app.h
 * @brief DLMS/COSEM Application Engine
 */

#ifndef DLMS_APP_H
#define DLMS_APP_H

#include "dlms_api.h"
#include "dlms_hdlc.h"
#include "dlms_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Process an APDU directly (used by both HDLC and Wrapper transports) */
dlms_result_t dlms_app_process_apdu(
    uint8_t client_sap,
    const uint8_t *in_apdu, uint16_t in_len,
    uint8_t *out_apdu, uint16_t max_out_len, uint16_t *out_len);

/** Get active association context by Client SAP */
dlms_association_t *dlms_app_get_association(uint8_t client_sap);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_APP_H */
