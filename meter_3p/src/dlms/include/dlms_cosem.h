/**
 * @file dlms_cosem.h
 * @brief COSEM Interface Classes, Object Modeling, and Association Contexts
 * 
 * Standards: IEC 62056-6-2, IEC 62056-5-3
 */

#ifndef DLMS_COSEM_H
#define DLMS_COSEM_H

#include "dlms_types.h"
#include "dlms_security.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* Attribute Access Rights                                                    */
/* ========================================================================== */

typedef enum {
    DLMS_ACCESS_NONE                    = 0,
    DLMS_ACCESS_READ_ONLY               = 1,
    DLMS_ACCESS_WRITE_ONLY              = 2,
    DLMS_ACCESS_READ_WRITE              = 3,
    DLMS_ACCESS_AUTH_READ               = 4,
    DLMS_ACCESS_AUTH_WRITE              = 5,
    DLMS_ACCESS_AUTH_READ_WRITE         = 6
} dlms_access_mode_t;

/* Method Access Rights */
typedef enum {
    DLMS_METHOD_ACCESS_NONE             = 0,
    DLMS_METHOD_ACCESS_ALLOWED          = 1,
    DLMS_METHOD_ACCESS_AUTH_ALLOWED     = 2
} dlms_method_access_mode_t;

/* Association State */
typedef enum {
    DLMS_ASSOC_STATE_NON_ASSOCIATED     = 0,
    DLMS_ASSOC_STATE_PENDING_HLS        = 1,
    DLMS_ASSOC_STATE_ASSOCIATED         = 2
} dlms_assoc_state_t;

/* Forward declarations */
struct dlms_cosem_object;
struct dlms_connection;

/* Function pointer types for Attribute Read, Write, and Method Execution */
typedef dlms_data_access_result_t (*dlms_attr_read_fn)(
    const struct dlms_cosem_object *obj,
    uint8_t attr_index,
    uint8_t *out_buf,
    uint16_t max_out_len,
    uint16_t *actual_len,
    void *user_ctx);

typedef dlms_data_access_result_t (*dlms_attr_write_fn)(
    struct dlms_cosem_object *obj,
    uint8_t attr_index,
    const uint8_t *in_buf,
    uint16_t in_len,
    void *user_ctx);

typedef dlms_action_result_t (*dlms_method_exec_fn)(
    struct dlms_cosem_object *obj,
    uint8_t method_index,
    const uint8_t *in_data,
    uint16_t in_len,
    uint8_t *out_data,
    uint16_t max_out_len,
    uint16_t *actual_out_len,
    void *user_ctx);

/* Attribute Descriptor */
typedef struct {
    uint8_t             attr_index;     /**< 1-based attribute index */
    dlms_datatype_t     type;           /**< Expected A-XDR data type */
    dlms_access_mode_t  access_public;  /**< Access permissions for Public client */
    dlms_access_mode_t  access_mgmt;    /**< Access permissions for Management client */
} dlms_attr_desc_t;

/* Method Descriptor */
typedef struct {
    uint8_t                   method_index;  /**< 1-based method index */
    dlms_method_access_mode_t access_public; /**< Permissions for Public client */
    dlms_method_access_mode_t access_mgmt;   /**< Permissions for Management client */
} dlms_method_desc_t;

/* COSEM Object Model */
typedef struct dlms_cosem_object {
    dlms_class_id_t     class_id;
    uint8_t             version;
    dlms_obis_t         obis;
    uint8_t             num_attrs;
    const dlms_attr_desc_t *attrs;
    uint8_t             num_methods;
    const dlms_method_desc_t *methods;
    dlms_attr_read_fn   read_fn;
    dlms_attr_write_fn  write_fn;
    dlms_method_exec_fn method_fn;
    void               *user_data;
} dlms_cosem_object_t;

/* ========================================================================== */
/* Application Association Structure                                         */
/* ========================================================================== */

typedef struct {
    uint8_t             client_sap;         /**< Client SAP (e.g. 0x10 Public, 0x01 Mgmt) */
    uint8_t             server_sap;         /**< Server SAP (Default: 0x01) */
    dlms_auth_mechanism_t auth_mechanism;   /**< Authentication mechanism */
    dlms_assoc_state_t  state;              /**< Non-associated, Pending HLS, Associated */
    uint32_t            conformance;        /**< 24-bit negotiated conformance bitfield */
    uint16_t            max_pdu_size;       /**< Negotiated maximum PDU size */
    dlms_security_context_t security_ctx;   /**< Security Suite 0 keys & counters */
} dlms_association_t;

/* Standard Conformance Bitfield Flags (IEC 62056-5-3) */
#define DLMS_CONFORMANCE_RESERVED_0         (1UL << 23)
#define DLMS_CONFORMANCE_RESERVED_1         (1UL << 22)
#define DLMS_CONFORMANCE_RESERVED_2         (1UL << 21)
#define DLMS_CONFORMANCE_READ               (1UL << 20)
#define DLMS_CONFORMANCE_WRITE              (1UL << 19)
#define DLMS_CONFORMANCE_UNCONFIRMED_WRITE  (1UL << 18)
#define DLMS_CONFORMANCE_ATTR_0_SET         (1UL << 17)
#define DLMS_CONFORMANCE_ATTR_0_GET         (1UL << 16)
#define DLMS_CONFORMANCE_BLOCK_TRANSFER_GET (1UL << 15)
#define DLMS_CONFORMANCE_BLOCK_TRANSFER_SET (1UL << 14)
#define DLMS_CONFORMANCE_BLOCK_TRANSFER_ACT (1UL << 13)
#define DLMS_CONFORMANCE_MULTIPLE_REFS      (1UL << 12)
#define DLMS_CONFORMANCE_INFO_REPORT        (1UL << 11)
#define DLMS_CONFORMANCE_DATA_NOTIF         (1UL << 10)
#define DLMS_CONFORMANCE_ACCESS             (1UL << 9)
#define DLMS_CONFORMANCE_PARAM_VARIATION    (1UL << 8)
#define DLMS_CONFORMANCE_GET                (1UL << 4)
#define DLMS_CONFORMANCE_SET                (1UL << 3)
#define DLMS_CONFORMANCE_SELECTIVE_ACCESS   (1UL << 2)
#define DLMS_CONFORMANCE_EVENT_NOTIF        (1UL << 1)
#define DLMS_CONFORMANCE_ACTION             (1UL << 0)

/** Default Conformance Block for Server (GET, SET, ACTION, Block-Transfer, Selective Access) */
#define DLMS_DEFAULT_SERVER_CONFORMANCE \
    (DLMS_CONFORMANCE_GET | DLMS_CONFORMANCE_SET | DLMS_CONFORMANCE_ACTION | \
     DLMS_CONFORMANCE_BLOCK_TRANSFER_GET | DLMS_CONFORMANCE_BLOCK_TRANSFER_SET | \
     DLMS_CONFORMANCE_SELECTIVE_ACCESS | DLMS_CONFORMANCE_MULTIPLE_REFS)

#ifdef __cplusplus
}
#endif

#endif /* DLMS_COSEM_H */
