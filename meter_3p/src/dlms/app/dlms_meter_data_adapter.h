/**
 * @file dlms_meter_data_adapter.h
 * @brief Default Meter Data Adapter for PIC32CXMTC Metrology & Energy
 */

#ifndef DLMS_METER_DATA_ADAPTER_H
#define DLMS_METER_DATA_ADAPTER_H

#include "dlms_meter_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize and register the default PIC32CXMTC meter data adapter */
void dlms_meter_data_adapter_init(void);

/** Set simulated metrology values (useful for testing and calibration) */
void dlms_meter_data_adapter_set_simulated(
    float v_a, float v_b, float v_c,
    float i_a, float i_b, float i_c,
    float p_total, double active_energy_wh);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_METER_DATA_ADAPTER_H */
