/**
 * @file dlms_objects.h
 * @brief COSEM Object Dictionary and xDLMS Services (GET/SET/ACTION)
 * 
 * Standards: IEC 62056-6-2, IEC 62056-5-3
 */

#ifndef DLMS_OBJECTS_H
#define DLMS_OBJECTS_H

#include "dlms_types.h"
#include "dlms_cosem.h"
#include "dlms_meter_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize COSEM Object Dictionary */
void dlms_objects_init(void);

/** Find object in dictionary by Class ID and OBIS */
dlms_cosem_object_t *dlms_find_object(uint16_t class_id, const dlms_obis_t *obis);

/**
 * @brief Process incoming xDLMS Request (GET, SET, ACTION).
 * @param assoc Active client association.
 * @param req_apdu Raw request APDU.
 * @param req_len Request APDU length.
 * @param resp_apdu Output buffer for response APDU.
 * @param max_resp_len Capacity of resp_apdu.
 * @param resp_len Output actual length of response APDU.
 * @return DLMS_OK on success.
 */
dlms_result_t dlms_process_request(
    dlms_association_t *assoc,
    const uint8_t *req_apdu, uint16_t req_len,
    uint8_t *resp_apdu, uint16_t max_resp_len, uint16_t *resp_len);

/** Capture an entry into Load Profile Generic (called periodically e.g. every 15 mins) */
void dlms_profile_generic_capture(void);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_OBJECTS_H */
