/**
 * @file dlms_api.h
 * @brief Public Top-Level API for DLMS/COSEM Security Suite 0 Stack
 */

#ifndef DLMS_API_H
#define DLMS_API_H

#include "dlms_config.h"
#include "dlms_types.h"
#include "dlms_security.h"
#include "dlms_cosem.h"
#include "dlms_meter_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Transmit callback function pointer (e.g. sends via UART or TCP) */
typedef void (*dlms_tx_callback_fn)(const uint8_t *data, uint16_t length, void *user_data);

/**
 * @brief Initialize the DLMS/COSEM stack with Suite 0 security and default COSEM objects.
 * @return DLMS_OK on success.
 */
dlms_result_t dlms_init(void);

/**
 * @brief Main periodic tasks function. Call periodically from Harmony loop or FreeRTOS task.
 */
void dlms_tasks(void);

/**
 * @brief Register transmit callback for HDLC serial communications (UART/RS-485/Optical).
 * @param tx_cb Function pointer to transmit data.
 * @param user_data Optional user context passed to callback.
 */
void dlms_register_hdlc_tx_callback(dlms_tx_callback_fn tx_cb, void *user_data);

/**
 * @brief Feed a single received byte into HDLC receiver (e.g. from UART RX interrupt or FIFO).
 * @param byte Received byte.
 */
void dlms_hdlc_rx_byte(uint8_t byte);

/**
 * @brief Feed a buffer of received bytes into HDLC receiver.
 * @param data Pointer to received bytes.
 * @param length Number of bytes.
 */
void dlms_hdlc_rx_buffer(const uint8_t *data, uint16_t length);

/**
 * @brief Process an incoming DLMS TCP/IP Wrapper packet (IEC 62056-47).
 * @param in_data Received wrapper packet (starts with 8-byte wrapper header).
 * @param in_len Length of received packet.
 * @param out_resp Buffer to write response packet into.
 * @param max_resp_len Capacity of out_resp.
 * @param actual_resp_len Written response length.
 * @return DLMS_OK on success.
 */
dlms_result_t dlms_wrapper_process(
    const uint8_t *in_data, uint16_t in_len,
    uint8_t *out_resp, uint16_t max_resp_len, uint16_t *actual_resp_len);

/**
 * @brief Get access to the current security context (keys, invocation counters, system titles).
 * @param client_sap SAP of the association (e.g. DLMS_SAP_MANAGEMENT_CLIENT).
 * @return Pointer to security context, or NULL.
 */
dlms_security_context_t *dlms_get_security_context(uint8_t client_sap);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_API_H */
