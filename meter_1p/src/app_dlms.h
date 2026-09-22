/*******************************************************************************
  MPLAB Harmony Application Header File

  Company:
    Microchip Technology Inc.

  File Name:
    app_dlms.h

  Summary:
    DLMS/COSEM Application Task Header for PIC32CXMTC over FLEXCOM1 USART.

  Description:
    This header file provides prototypes and definitions for the DLMS/COSEM
    application task executing on the PIC32CXMTC using FLEXCOM1 USART as the
    physical HDLC communication interface.
*******************************************************************************/

#ifndef _APP_DLMS_H
#define _APP_DLMS_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "configuration.h"

// DOM-IGNORE-BEGIN
#ifdef __cplusplus
extern "C" {
#endif
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: Type Definitions
// *****************************************************************************
// *****************************************************************************

typedef enum
{
    /* Application's state machine's initial state. */
    APP_DLMS_STATE_INIT = 0,
    APP_DLMS_STATE_SERVICE_TASKS,
    APP_DLMS_STATE_ERROR

} APP_DLMS_STATES;

typedef struct
{
    /* The application's current state */
    APP_DLMS_STATES state;

    /* Operational statistics */
    volatile uint32_t rx_bytes;
    volatile uint32_t tx_bytes;
    volatile uint32_t rx_errors;
    volatile uint32_t rx_frames;
    volatile uint32_t tx_frames;

} APP_DLMS_DATA;

extern APP_DLMS_DATA app_dlmsData;

// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

void APP_DLMS_Initialize ( void );
void APP_DLMS_Tasks( void );

// *****************************************************************************
// *****************************************************************************
// Section: Utility APIs for DLMS over FLEXCOM1
// *****************************************************************************
// *****************************************************************************

/**
 * @brief Configure FLEXCOM1 USART baud rate dynamically (e.g. 9600, 19200, 115200)
 * @param baudRate Desired baud rate in bps
 * @return true on success, false otherwise
 */
bool APP_DLMS_SetBaudRate( uint32_t baudRate );

/**
 * @brief Get total bytes received from FLEXCOM1
 */
uint32_t APP_DLMS_GetRxCount( void );

/**
 * @brief Get total bytes transmitted over FLEXCOM1
 */
uint32_t APP_DLMS_GetTxCount( void );

/**
 * @brief Get total UART receiver errors encountered
 */
uint32_t APP_DLMS_GetErrorCount( void );

/**
 * @brief Check if DLMS HDLC session is currently connected
 */
bool APP_DLMS_IsHdlcConnected( void );

// DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
// DOM-IGNORE-END

#endif /* _APP_DLMS_H */
/*******************************************************************************
 End of File
 */
