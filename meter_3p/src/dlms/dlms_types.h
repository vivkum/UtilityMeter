/**
 * @file dlms_types.h
 * @brief Standard DLMS/COSEM Types, Tags, OBIS definitions, and Result Codes
 * 
 * Standards: IEC 62056-5-3, IEC 62056-6-1, IEC 62056-6-2
 */

#ifndef DLMS_TYPES_H
#define DLMS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Return / Status Codes                                                      */
/* ========================================================================== */

typedef enum {
    DLMS_OK                     = 0,
    DLMS_ERR_GENERAL            = -1,
    DLMS_ERR_INVALID_PARAM      = -2,
    DLMS_ERR_BUFFER_OVERFLOW    = -3,
    DLMS_ERR_CHECKSUM           = -4,
    DLMS_ERR_INVALID_STATE      = -5,
    DLMS_ERR_AUTHENTICATION     = -6,
    DLMS_ERR_SECURITY_FAILED    = -7,
    DLMS_ERR_OBJECT_NOT_FOUND   = -8,
    DLMS_ERR_ACCESS_DENIED      = -9,
    DLMS_ERR_NOT_SUPPORTED      = -10,
    DLMS_ERR_IN_PROGRESS        = -11,
    DLMS_ERR_FRAME_INCOMPLETE   = -12,
    DLMS_ERR_REPLAY_ATTACK      = -13
} dlms_result_t;

/* ========================================================================== */
/* OBIS Code Definition (Object Identification System - IEC 62056-6-1)        */
/* ========================================================================== */

typedef struct {
    uint8_t a;  /**< Group A: Medium (1 = Electricity) */
    uint8_t b;  /**< Group B: Channel (0 = No channel) */
    uint8_t c;  /**< Group C: Abstract physical quantity */
    uint8_t d;  /**< Group D: Measurement type / calculation */
    uint8_t e;  /**< Group E: Tariff / harmonic */
    uint8_t f;  /**< Group F: Historical billing period / billing index (255 = Current) */
} dlms_obis_t;

#define DLMS_OBIS_EQUALS(o1, o2) \
    (((o1).a == (o2).a) && ((o1).b == (o2).b) && ((o1).c == (o2).c) && \
     ((o1).d == (o2).d) && ((o1).e == (o2).e) && ((o1).f == (o2).f))

/* Helper macro to initialize OBIS codes */
#define DLMS_OBIS_INIT(a, b, c, d, e, f) { (a), (b), (c), (d), (e), (f) }

/* ========================================================================== */
/* Standard COSEM Interface Class IDs (IEC 62056-6-2)                        */
/* ========================================================================== */

typedef enum {
    DLMS_CLASS_DATA                 = 1,
    DLMS_CLASS_REGISTER             = 3,
    DLMS_CLASS_EXTENDED_REGISTER    = 4,
    DLMS_CLASS_DEMAND_REGISTER      = 5,
    DLMS_CLASS_PROFILE_GENERIC      = 7,
    DLMS_CLASS_CLOCK                = 8,
    DLMS_CLASS_SCRIPT_TABLE         = 9,
    DLMS_CLASS_SCHEDULE             = 10,
    DLMS_CLASS_SPECIAL_DAYS_TABLE   = 11,
    DLMS_CLASS_ASSOCIATION_SN       = 12,
    DLMS_CLASS_ASSOCIATION_LN       = 15,
    DLMS_CLASS_SAP_ASSIGNMENT       = 17,
    DLMS_CLASS_IMAGE_TRANSFER       = 18,
    DLMS_CLASS_ACTIVITY_CALENDAR    = 20,
    DLMS_CLASS_REGISTER_MONITOR     = 21,
    DLMS_CLASS_SINGLE_ACTION_SCHED  = 22,
    DLMS_CLASS_IEC_LOCAL_PORT_SETUP = 19,
    DLMS_CLASS_IEC_HDLC_SETUP       = 23,
    DLMS_CLASS_IEC_TWISTED_PAIR     = 24,
    DLMS_CLASS_TCP_UDP_SETUP        = 25,
    DLMS_CLASS_IP_V4_SETUP          = 42,
    DLMS_CLASS_MAC_ADDRESS_SETUP    = 43,
    DLMS_CLASS_PPP_SETUP            = 44,
    DLMS_CLASS_GPRS_SETUP           = 45,
    DLMS_CLASS_SECURITY_SETUP       = 64,
    DLMS_CLASS_DISCONNECT_CONTROL   = 70,
    DLMS_CLASS_LIMITER              = 71
} dlms_class_id_t;

/* ========================================================================== */
/* A-XDR Data Type Tags (IEC 62056-5-3)                                      */
/* ========================================================================== */

typedef enum {
    DLMS_DATATYPE_NULL                  = 0,
    DLMS_DATATYPE_ARRAY                 = 1,
    DLMS_DATATYPE_STRUCTURE             = 2,
    DLMS_DATATYPE_BOOLEAN               = 3,
    DLMS_DATATYPE_BIT_STRING            = 4,
    DLMS_DATATYPE_DOUBLE_LONG           = 5,    /**< int32_t */
    DLMS_DATATYPE_DOUBLE_LONG_UNSIGNED  = 6,    /**< uint32_t */
    DLMS_DATATYPE_OCTET_STRING          = 9,
    DLMS_DATATYPE_VISIBLE_STRING        = 10,
    DLMS_DATATYPE_UTF8_STRING           = 12,
    DLMS_DATATYPE_BCD                   = 13,
    DLMS_DATATYPE_INTEGER               = 15,   /**< int8_t */
    DLMS_DATATYPE_LONG                  = 16,   /**< int16_t */
    DLMS_DATATYPE_UNSIGNED              = 17,   /**< uint8_t */
    DLMS_DATATYPE_LONG_UNSIGNED         = 18,   /**< uint16_t */
    DLMS_DATATYPE_COMPACT_ARRAY         = 19,
    DLMS_DATATYPE_LONG64                = 20,   /**< int64_t */
    DLMS_DATATYPE_LONG64_UNSIGNED       = 21,   /**< uint64_t */
    DLMS_DATATYPE_ENUM                  = 22,
    DLMS_DATATYPE_FLOAT32               = 23,   /**< float */
    DLMS_DATATYPE_FLOAT64               = 24,   /**< double */
    DLMS_DATATYPE_DATE_TIME             = 25,   /**< 12 bytes octet-string */
    DLMS_DATATYPE_DATE                  = 26,   /**< 5 bytes */
    DLMS_DATATYPE_TIME                  = 27,   /**< 4 bytes */
    DLMS_DATATYPE_DONT_CARE             = 255
} dlms_datatype_t;

/* ========================================================================== */
/* Standard Physical Measurement Units (IEC 62056-6-2)                        */
/* ========================================================================== */

typedef enum {
    DLMS_UNIT_NONE                      = 0,
    DLMS_UNIT_YEAR                      = 1,
    DLMS_UNIT_MONTH                     = 2,
    DLMS_UNIT_WEEK                      = 3,
    DLMS_UNIT_DAY                       = 4,
    DLMS_UNIT_HOUR                      = 5,
    DLMS_UNIT_MINUTE                    = 6,
    DLMS_UNIT_SECOND                    = 7,
    DLMS_UNIT_DEGREE                    = 8,
    DLMS_UNIT_DEGREE_CELSIUS            = 9,
    DLMS_UNIT_LOCAL_CURRENCY            = 11,
    DLMS_UNIT_METRE                     = 13,
    DLMS_UNIT_METRE_PER_SECOND          = 14,
    DLMS_UNIT_CUBIC_METRE               = 15,
    DLMS_UNIT_LITRE                     = 19,
    DLMS_UNIT_MASS_KG                   = 21,
    DLMS_UNIT_NEWTON                    = 22,
    DLMS_UNIT_PRESSURE_PASCAL           = 24,
    DLMS_UNIT_PRESSURE_BAR              = 25,
    DLMS_UNIT_ENERGY_JOULE              = 26,
    DLMS_UNIT_ACTIVE_POWER_WATT         = 27,
    DLMS_UNIT_APPARENT_POWER_VA         = 28,
    DLMS_UNIT_REACTIVE_POWER_VAR        = 29,
    DLMS_UNIT_ACTIVE_ENERGY_WH          = 30,
    DLMS_UNIT_APPARENT_ENERGY_VAH       = 31,
    DLMS_UNIT_REACTIVE_ENERGY_VARH      = 32,
    DLMS_UNIT_VOLTAGE_VOLT              = 33,
    DLMS_UNIT_CURRENT_AMPERE            = 35,
    DLMS_UNIT_FREQUENCY_HERTZ           = 44,
    DLMS_UNIT_ACTIVE_ENERGY_IMP         = 50,
    DLMS_UNIT_REACTIVE_ENERGY_IMP       = 51,
    DLMS_UNIT_COUNT                     = 255
} dlms_unit_t;

/* Scaler-Unit pair for registers */
typedef struct {
    int8_t      scaler;     /**< Exponent: value * 10^scaler (e.g. -3 = milli, 0 = 1, 3 = kilo) */
    dlms_unit_t unit;       /**< Measurement unit code */
} dlms_scaler_unit_t;

/* ========================================================================== */
/* APDU Command Tags (IEC 62056-5-3)                                          */
/* ========================================================================== */

typedef enum {
    DLMS_TAG_AARQ                       = 0x60, /**< Association Request */
    DLMS_TAG_AARE                       = 0x61, /**< Association Response */
    DLMS_TAG_RLRQ                       = 0x62, /**< Release Request */
    DLMS_TAG_RLRE                       = 0x63, /**< Release Response */
    
    DLMS_TAG_GET_REQUEST                = 0xC0,
    DLMS_TAG_SET_REQUEST                = 0xC1,
    DLMS_TAG_EVENT_NOTIFICATION_REQUEST = 0xC2,
    DLMS_TAG_ACTION_REQUEST             = 0xC3,
    DLMS_TAG_GET_RESPONSE               = 0xC4,
    DLMS_TAG_SET_RESPONSE               = 0xC5,
    DLMS_TAG_ACTION_RESPONSE            = 0xC7,
    
    /* Security Suite 0 Ciphered APDUs */
    DLMS_TAG_GLO_GET_REQUEST            = 0xC8,
    DLMS_TAG_GLO_SET_REQUEST            = 0xC9,
    DLMS_TAG_GLO_EVENT_NOTIF_REQUEST    = 0xCA,
    DLMS_TAG_GLO_ACTION_REQUEST         = 0xCB,
    DLMS_TAG_GLO_GET_RESPONSE           = 0xCC,
    DLMS_TAG_GLO_SET_RESPONSE           = 0xCD,
    DLMS_TAG_GLO_ACTION_RESPONSE        = 0xCF,
    
    DLMS_TAG_GENERAL_GLO_CIPHERING      = 0xDB,
    DLMS_TAG_GENERAL_DED_CIPHERING      = 0xDC
} dlms_apdu_tag_t;

/* GET Request Sub-types */
typedef enum {
    DLMS_GET_REQUEST_NORMAL             = 1,
    DLMS_GET_REQUEST_NEXT               = 2,
    DLMS_GET_REQUEST_WITH_LIST          = 3
} dlms_get_request_type_t;

/* GET Response Sub-types */
typedef enum {
    DLMS_GET_RESPONSE_NORMAL            = 1,
    DLMS_GET_RESPONSE_DATABLOCK         = 2,
    DLMS_GET_RESPONSE_WITH_LIST         = 3
} dlms_get_response_type_t;

/* SET Request Sub-types */
typedef enum {
    DLMS_SET_REQUEST_NORMAL             = 1,
    DLMS_SET_REQUEST_FIRST_DATABLOCK    = 2,
    DLMS_SET_REQUEST_DATABLOCK          = 3,
    DLMS_SET_REQUEST_WITH_LIST          = 4
} dlms_set_request_type_t;

/* ACTION Request Sub-types */
typedef enum {
    DLMS_ACTION_REQUEST_NORMAL          = 1,
    DLMS_ACTION_REQUEST_NEXT_BLOCK      = 2,
    DLMS_ACTION_REQUEST_WITH_LIST       = 3
} dlms_action_request_type_t;

/* ========================================================================== */
/* Data Access Result Codes (IEC 62056-5-3)                                   */
/* ========================================================================== */

typedef enum {
    DLMS_RESULT_SUCCESS                 = 0,
    DLMS_RESULT_HARDWARE_FAULT          = 1,
    DLMS_RESULT_TEMPORARY_FAILURE       = 2,
    DLMS_RESULT_READ_WRITE_DENIED       = 3,
    DLMS_RESULT_OBJECT_UNDEFINED        = 4,
    DLMS_RESULT_OBJECT_CLASS_INCONSIST  = 9,
    DLMS_RESULT_OBJECT_UNAVAILABLE      = 11,
    DLMS_RESULT_TYPE_UNMATCHED          = 12,
    DLMS_RESULT_SCOPE_OF_ACCESS_VIOLATED= 13,
    DLMS_RESULT_DATA_BLOCK_UNAVAILABLE  = 14,
    DLMS_RESULT_LONG_GET_ABORTED        = 15,
    DLMS_RESULT_NO_LONG_GET_IN_PROGRESS = 16,
    DLMS_RESULT_LONG_SET_ABORTED        = 17,
    DLMS_RESULT_NO_LONG_SET_IN_PROGRESS = 18,
    DLMS_RESULT_DATA_BLOCK_NUMBER_INV   = 19,
    DLMS_RESULT_OTHER_REASON            = 250
} dlms_data_access_result_t;

/* Action Result Codes */
typedef enum {
    DLMS_ACTION_RESULT_SUCCESS          = 0,
    DLMS_ACTION_RESULT_HARDWARE_FAULT   = 1,
    DLMS_ACTION_RESULT_TEMPORARY_FAIL   = 2,
    DLMS_ACTION_RESULT_READ_WRITE_DENIED= 3,
    DLMS_ACTION_RESULT_OBJECT_UNDEFINED = 4,
    DLMS_ACTION_RESULT_OBJECT_CLASS_INCONSIST = 9,
    DLMS_ACTION_RESULT_OBJECT_UNAVAILABLE = 11,
    DLMS_ACTION_RESULT_TYPE_UNMATCHED   = 12,
    DLMS_ACTION_RESULT_SCOPE_VIOLATED   = 13,
    DLMS_ACTION_RESULT_DATA_BLOCK_UNAVAIL = 14,
    DLMS_ACTION_RESULT_OTHER_REASON     = 250
} dlms_action_result_t;

/* ========================================================================== */
/* DLMS Date-Time Structure (12 bytes - IEC 62056-6-2)                        */
/* ========================================================================== */

typedef struct {
    uint16_t year;          /**< Year (e.g. 2026), 0xFFFF = not specified */
    uint8_t  month;         /**< Month 1..12, 0xFD=daylight_savings_end, 0xFE=daylight_savings_begin, 0xFF=not specified */
    uint8_t  day_of_month;  /**< Day 1..31, 0xFD=2nd last, 0xFE=last, 0xFF=not specified */
    uint8_t  day_of_week;   /**< 1=Monday .. 7=Sunday, 0xFF=not specified */
    uint8_t  hour;          /**< Hour 0..23, 0xFF=not specified */
    uint8_t  minute;        /**< Minute 0..59, 0xFF=not specified */
    uint8_t  second;        /**< Second 0..59, 0xFF=not specified */
    uint8_t  hundredths;    /**< Hundredths 0..99, 0xFF=not specified */
    int16_t  deviation;     /**< Deviation from UTC in minutes (-720..+720), 0x8000 = not specified */
    uint8_t  clock_status;  /**< Bit 0: invalid, Bit 1: doubtful, Bit 2: daylight_saving, Bit 7: sync */
} dlms_date_time_t;

#ifdef __cplusplus
}
#endif

#endif /* DLMS_TYPES_H */
