/**
 * @file dlms_meter_adapter.h
 * @brief Interface between DLMS/COSEM stack and PIC32CXMTC Metrology & Energy Engine
 */

#ifndef DLMS_METER_ADAPTER_H
#define DLMS_METER_ADAPTER_H

#include "dlms_types.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Metering Instantaneous Measurements (from PIC32CXMTC metrology engine) */
typedef struct {
    float voltage_a;        /**< Phase A RMS Voltage (V) */
    float voltage_b;        /**< Phase B RMS Voltage (V) */
    float voltage_c;        /**< Phase C RMS Voltage (V) */
    
    float current_a;        /**< Phase A RMS Current (A) */
    float current_b;        /**< Phase B RMS Current (A) */
    float current_c;        /**< Phase C RMS Current (A) */
    float current_neutral;  /**< Neutral RMS Current (A) */
    
    float active_power_total;/**< Total Active Power (W) */
    float active_power_a;    /**< Phase A Active Power (W) */
    float active_power_b;    /**< Phase B Active Power (W) */
    float active_power_c;    /**< Phase C Active Power (W) */
    
    float reactive_power_total; /**< Total Reactive Power (var) */
    float apparent_power_total; /**< Total Apparent Power (VA) */
    
    float frequency;        /**< Grid Frequency (Hz) */
    float power_factor;     /**< Total Power Factor (-1.0 .. +1.0) */
} dlms_meter_instantaneous_t;

/* Energy Accumulators (from app_energy.c) */
typedef struct {
    double active_energy_import;   /**< Total Active Import (Wh) */
    double active_energy_export;   /**< Total Active Export (Wh) */
    double reactive_energy_import; /**< Total Reactive Import (varh) */
    double reactive_energy_export; /**< Total Reactive Export (varh) */
    double apparent_energy_import; /**< Total Apparent Import (VAh) */
    
    /* TOU Tariff accumulators */
    double tariff_active_import[4];
} dlms_meter_energy_t;

/* Maximum Demand */
typedef struct {
    float            value;         /**< Max Demand Power (W) */
    dlms_date_time_t timestamp;     /**< Capture timestamp */
} dlms_meter_max_demand_t;

/* Adapter Function Table */
typedef struct {
    /* Function to read instantaneous values */
    bool (*get_instantaneous)(dlms_meter_instantaneous_t *data);
    
    /* Function to read energy accumulators */
    bool (*get_energy)(dlms_meter_energy_t *data);
    
    /* Function to read maximum demand */
    bool (*get_max_demand)(dlms_meter_max_demand_t *data);
    
    /* Function to get current RTC time */
    bool (*get_rtc_time)(dlms_date_time_t *datetime);
    
    /* Function to set RTC time */
    bool (*set_rtc_time)(const dlms_date_time_t *datetime);
    
    /* Disconnect / Reconnect Relay Control (Class 70) */
    bool (*set_relay_state)(bool connect);
    bool (*get_relay_state)(bool *connected);
    
    /* Serial number & Device info */
    const char *(*get_serial_number)(void);
} dlms_meter_adapter_t;

/** Register the meter adapter implementation */
void dlms_meter_adapter_register(const dlms_meter_adapter_t *adapter);

/** Get active meter adapter */
const dlms_meter_adapter_t *dlms_meter_adapter_get(void);

#ifdef __cplusplus
}
#endif

#endif /* DLMS_METER_ADAPTER_H */
