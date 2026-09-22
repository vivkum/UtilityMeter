/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app_dlms.c

  Summary:
    DLMS/COSEM Application Task Source File for PIC32CXMTC using FLEXCOM1 USART.

  Description:
    This file implements the DLMS/COSEM application task on PIC32CXMTC.
    It bridges the DLMS/COSEM Security Suite 0 stack with FLEXCOM1 USART
    operating in HDLC serial mode (IEC 62056-46).
*******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "app_dlms.h"
#include "definitions.h"
#include "peripheral/flexcom/usart/plib_flexcom1_usart.h"
#include "dlms_api.h"
#include "dlms_app.h"
#include "dlms_meter_adapter.h"
#include "dlms_hdlc.h"
#include <string.h>

// *****************************************************************************
// *****************************************************************************
// Section: Constant & Macro Definitions
// *****************************************************************************
// *****************************************************************************

/** RX circular buffer capacity (must be a power of 2) */
#define APP_DLMS_RX_RING_SIZE       2048U
#define APP_DLMS_RX_RING_MASK       (APP_DLMS_RX_RING_SIZE - 1U)

/** Maximum HDLC transmit frame buffer size */
#define APP_DLMS_TX_BUF_SIZE        DLMS_HDLC_TX_BUF_SIZE

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

/** Global Application Data */
APP_DLMS_DATA app_dlmsData;

/** Single-byte buffer for continuous interrupt-driven UART reception */
static volatile uint8_t  s_rx_byte;

/** Circular buffer for incoming UART bytes */
static volatile uint8_t  s_rx_ring[APP_DLMS_RX_RING_SIZE];
static volatile uint16_t s_rx_head = 0;
static volatile uint16_t s_rx_tail = 0;

/** Static transmission buffer for outgoing HDLC frames */
static uint8_t           s_tx_buffer[APP_DLMS_TX_BUF_SIZE];
static volatile bool     s_tx_busy = false;

// *****************************************************************************
// *****************************************************************************
// Section: Local Helper Functions
// *****************************************************************************
// *****************************************************************************

/**
 * @brief Push a received byte into the lock-free circular buffer (called from ISR context)
 */
static inline bool app_dlms_ring_push(uint8_t byte)
{
    uint16_t next = (s_rx_head + 1U) & APP_DLMS_RX_RING_MASK;
    if (next != s_rx_tail)
    {
        s_rx_ring[s_rx_head] = byte;
        s_rx_head = next;
        return true;
    }
    return false; /* Buffer overflow */
}

/**
 * @brief Pop a byte from the circular buffer (called from task context)
 */
static inline bool app_dlms_ring_pop(uint8_t *byte)
{
    if (s_rx_tail == s_rx_head)
    {
        return false; /* Buffer empty */
    }
    *byte = s_rx_ring[s_rx_tail];
    s_rx_tail = (s_rx_tail + 1U) & APP_DLMS_RX_RING_MASK;
    return true;
}

// *****************************************************************************
// *****************************************************************************
// Section: FLEXCOM1 USART Callbacks
// *****************************************************************************
// *****************************************************************************

/**
 * @brief FLEXCOM1 USART RX interrupt completion callback
 */
static void APP_DLMS_UartRxCallback(uintptr_t context)
{
    (void)context;

    FLEXCOM_USART_ERROR errorStatus = FLEXCOM1_USART_ErrorGet();

    if (errorStatus == FLEXCOM_USART_ERROR_NONE)
    {
        /* Push the received byte into ring buffer */
        if (app_dlms_ring_push(s_rx_byte))
        {
            app_dlmsData.rx_bytes++;
        }
        else
        {
            app_dlmsData.rx_errors++;
        }
    }
    else
    {
        app_dlmsData.rx_errors++;
    }

    /* Immediately re-arm reception for the next byte */
    FLEXCOM1_USART_Read((void *)&s_rx_byte, 1U);
}

/**
 * @brief FLEXCOM1 USART TX interrupt completion callback
 */
static void APP_DLMS_UartTxCallback(uintptr_t context)
{
    (void)context;
    s_tx_busy = false;
}

// *****************************************************************************
// *****************************************************************************
// Section: DLMS Stack Transmit Callback
// *****************************************************************************
// *****************************************************************************

/**
 * @brief Transmit callback registered with the DLMS HDLC engine
 *
 * Invoked by the DLMS protocol engine whenever a response frame (UA, DM, I-Frame)
 * needs to be transmitted back to the client over FLEXCOM1 USART.
 */
static void APP_DLMS_HdlcTxCallback(const uint8_t *data, uint16_t length, void *user_data)
{
    (void)user_data;

    if (!data || length == 0U)
    {
        return;
    }

    /* Wait for any previous UART transmission to complete with a safety timeout */
    uint32_t timeout = 500000U;
    while (FLEXCOM1_USART_WriteIsBusy() && (--timeout > 0U))
    {
        /* Spin wait until hardware FIFO / DMA is free */
    }

    /* Prevent buffer overflow */
    if (length > APP_DLMS_TX_BUF_SIZE)
    {
        length = APP_DLMS_TX_BUF_SIZE;
    }

    /* Copy into dedicated static TX buffer to ensure memory remains valid during async write */
    memcpy(s_tx_buffer, data, length);

    app_dlmsData.tx_bytes += length;
    app_dlmsData.tx_frames++;
    s_tx_busy = true;

    /* Initiate non-blocking interrupt-driven transmission */
    FLEXCOM1_USART_Write((void *)s_tx_buffer, length);
}

// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/**
 * @brief Initialize the DLMS application, FLEXCOM1 hardware and protocol stack
 */
void APP_DLMS_Initialize ( void )
{
    /* Reset data structure & counters */
    memset(&app_dlmsData, 0, sizeof(app_dlmsData));
    app_dlmsData.state = APP_DLMS_STATE_INIT;

    s_rx_head = 0;
    s_rx_tail = 0;
    s_tx_busy = false;

    /* Register FLEXCOM1 USART interrupt callbacks */
    FLEXCOM1_USART_ReadCallbackRegister(APP_DLMS_UartRxCallback, (uintptr_t)NULL);
    FLEXCOM1_USART_WriteCallbackRegister(APP_DLMS_UartTxCallback, (uintptr_t)NULL);

    /* Initialize the DLMS/COSEM Security Suite 0 stack */
    dlms_init();

    /* Register the HDLC transmit callback to send frames via FLEXCOM1 USART */
    dlms_register_hdlc_tx_callback(APP_DLMS_HdlcTxCallback, NULL);

    /* Arm the first asynchronous single-byte reception on FLEXCOM1 */
    FLEXCOM1_USART_Read((void *)&s_rx_byte, 1U);

    /* Transition state to service tasks */
    app_dlmsData.state = APP_DLMS_STATE_SERVICE_TASKS;
}

/**
 * @brief DLMS Application periodic tasks routine
 *
 * Called continuously by MPLAB Harmony SYS_Tasks() main loop.
 */
void APP_DLMS_Tasks ( void )
{
    switch ( app_dlmsData.state )
    {
        case APP_DLMS_STATE_INIT:
        {
            APP_DLMS_Initialize();
            break;
        }

        case APP_DLMS_STATE_SERVICE_TASKS:
        {
            /* 1. Drain RX ring buffer and feed bytes into DLMS HDLC frame parser */
            uint8_t byte;
            while (app_dlms_ring_pop(&byte))
            {
                dlms_hdlc_rx_byte(byte);
            }

            /* 2. Monitor USART receiver health: if stalled by error or line noise, recover */
            if (!FLEXCOM1_USART_ReadIsBusy())
            {
                (void)FLEXCOM1_USART_ErrorGet();
                FLEXCOM1_USART_Read((void *)&s_rx_byte, 1U);
            }

            /* 3. Run DLMS stack maintenance tasks (timeouts, profile captures) */
            dlms_tasks();
            break;
        }

        case APP_DLMS_STATE_ERROR:
        default:
        {
            /* Handle application error: recover receiver and restart service tasks */
            FLEXCOM1_USART_ReadAbort();
            (void)FLEXCOM1_USART_ErrorGet();
            FLEXCOM1_USART_Read((void *)&s_rx_byte, 1U);
            app_dlmsData.state = APP_DLMS_STATE_SERVICE_TASKS;
            break;
        }
    }
}

// *****************************************************************************
// *****************************************************************************
// Section: Utility APIs for DLMS over FLEXCOM1
// *****************************************************************************
// *****************************************************************************

bool APP_DLMS_SetBaudRate( uint32_t baudRate )
{
    FLEXCOM_USART_SERIAL_SETUP setup;
    setup.baudRate  = baudRate;
    setup.dataWidth = FLEXCOM_USART_DATA_8_BIT;
    setup.parity    = FLEXCOM_USART_PARITY_NONE;
    setup.stopBits  = FLEXCOM_USART_STOP_1_BIT;

    return FLEXCOM1_USART_SerialSetup(&setup, 0U);
}

uint32_t APP_DLMS_GetRxCount( void )
{
    return app_dlmsData.rx_bytes;
}

uint32_t APP_DLMS_GetTxCount( void )
{
    return app_dlmsData.tx_bytes;
}

uint32_t APP_DLMS_GetErrorCount( void )
{
    return app_dlmsData.rx_errors;
}

bool APP_DLMS_IsHdlcConnected( void )
{
    dlms_association_t *assoc_mgmt = dlms_app_get_association(DLMS_SAP_MANAGEMENT_CLIENT);
    dlms_association_t *assoc_pub  = dlms_app_get_association(DLMS_SAP_PUBLIC_CLIENT);

    bool mgmt_conn = (assoc_mgmt && (assoc_mgmt->state == DLMS_ASSOC_STATE_ASSOCIATED));
    bool pub_conn  = (assoc_pub && (assoc_pub->state == DLMS_ASSOC_STATE_ASSOCIATED));

    return (mgmt_conn || pub_conn);
}

/*******************************************************************************
 End of File
 */
