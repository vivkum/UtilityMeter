/*******************************************************************************
  System Configuration Header

  File Name:
    configuration.h

  Summary:
    Build-time configuration header for the system defined by this project.

  Description:
    An MPLAB Project may have multiple configurations.  This file defines the
    build-time options for a single configuration.

  Remarks:
    This configuration header must not define any prototypes or data
    definitions (or include any files that do).  It only provides macro
    definitions for build-time configuration options

*******************************************************************************/

// DOM-IGNORE-BEGIN
/*******************************************************************************
* Copyright (C) 2018 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/
// DOM-IGNORE-END

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************
/*  This section Includes other configuration headers necessary to completely
    define this configuration.
*/

#include "user.h"
#include "device.h"

// DOM-IGNORE-BEGIN
#ifdef __cplusplus  // Provide C++ Compatibility

extern "C" {

#endif
// DOM-IGNORE-END

// *****************************************************************************
// *****************************************************************************
// Section: System Configuration
// *****************************************************************************
// *****************************************************************************



// *****************************************************************************
// *****************************************************************************
// Section: System Service Configuration
// *****************************************************************************
// *****************************************************************************

#define SYS_CMD_ENABLE
#define SYS_CMD_DEVICE_MAX_INSTANCES       SYS_CONSOLE_DEVICE_MAX_INSTANCES
#define SYS_CMD_PRINT_BUFFER_SIZE          1024U
#define SYS_CMD_BUFFER_DMA_READY


/* TIME System Service Configuration Options */
#define SYS_TIME_INDEX_0                            (0)
#define SYS_TIME_MAX_TIMERS                         (10)
#define SYS_TIME_HW_COUNTER_WIDTH                   (32)
#define SYS_TIME_HW_COUNTER_PERIOD                  (4294967295U)
#define SYS_TIME_HW_COUNTER_HALF_PERIOD             (SYS_TIME_HW_COUNTER_PERIOD>>1)
#define SYS_TIME_CPU_CLOCK_FREQUENCY                (200000000)
#define SYS_TIME_COMPARE_UPDATE_EXECUTION_CYCLES    (232)

#define SYS_CONSOLE_DEVICE_MAX_INSTANCES   			(1U)
#define SYS_CONSOLE_UART_MAX_INSTANCES 	   			(1U)
#define SYS_CONSOLE_USB_CDC_MAX_INSTANCES 	   		(0U)
#define SYS_CONSOLE_PRINT_BUFFER_SIZE        		(1024U)


#define SYS_CONSOLE_INDEX_0                       0






// *****************************************************************************
// *****************************************************************************
// Section: Driver Configuration
// *****************************************************************************
// *****************************************************************************
/* Memory Driver Global Configuration Options */
#define DRV_MEMORY_INSTANCES_NUMBER          (1U)
/* SST26 Driver Instance Configuration */
#define DRV_SST26_INDEX                 (0U)
#define DRV_SST26_CLIENTS_NUMBER        (1U)
#define DRV_SST26_START_ADDRESS         (0x0U)
#define DRV_SST26_PAGE_SIZE             (256U)
#define DRV_SST26_ERASE_BUFFER_SIZE     (4096U)


/* Memory Driver Instance 0 Configuration */
#define DRV_MEMORY_INDEX_0                   0
#define DRV_MEMORY_CLIENTS_NUMBER_IDX0       1
#define DRV_MEMORY_BUF_Q_SIZE_IDX0    1

/* Metrology Configuration Options */
#define DRV_METROLOGY_REG_BASE_ADDRESS        0x20088000UL
/* Metrology Default Config: FEATURE_CTRL */
#define DRV_METROLOGY_CONF_FCTRL              0x300UL
/* Metrology Default Config: AFE SELECTION */
#define DRV_METROLOGY_CONF_AFE_SEL            0x0UL
/* Metrology Default Config: CHANNEL_MATRIX */
#define DRV_METROLOGY_CONF_CHN_MATRIX         0xfff3210fUL
/* Metrology Default Config: HARMONIC_CTRL */
#define DRV_METROLOGY_CONF_HARMONIC_CTRL      0x0UL
/* Metrology Default Config: Meter Type */
#define DRV_METROLOGY_CONF_MT                 0xccUL
/* Metrology Default Config: PULSE0_CTRL */
#define DRV_METROLOGY_CONF_PULSE0_CTRL        0x810001d0UL
/* Metrology Default Config: PULSE0_K_t */
#define DRV_METROLOGY_CONF_PULSE0_KT          0x500000UL
/* Metrology Default Config: PULSE1_CTRL */
#define DRV_METROLOGY_CONF_PULSE1_CTRL        0x810201d0UL
/* Metrology Default Config: PULSE1_K_t */
#define DRV_METROLOGY_CONF_PULSE1_KT          0x500000UL
/* Metrology Default Config: PULSE2_CTRL */
#define DRV_METROLOGY_CONF_PULSE2_CTRL        0x1d0UL
/* Metrology Default Config: PULSE2_K_t */
#define DRV_METROLOGY_CONF_PULSE2_KT          0x500000UL
/* Metrology Default Config: PULSE2_K_t */
#define DRV_METROLOGY_CONF_SYNTH_ADDR         0x0UL
/* Metrology Default Config: CREEP P */
#define DRV_METROLOGY_CONF_CREEP_P            0x2e9aUL
/* Metrology Default Config: CREEP PA */
#define DRV_METROLOGY_CONF_CREEP_PA           0x2e9aUL
/* Metrology Default Config: CREEP PB */
#define DRV_METROLOGY_CONF_CREEP_PB           0x2e9aUL
/* Metrology Default Config: CREEP PC */
#define DRV_METROLOGY_CONF_CREEP_PC           0x0UL
/* Metrology Default Config: CREEP Q */
#define DRV_METROLOGY_CONF_CREEP_Q            0x2e9aUL
/* Metrology Default Config: CREEP QA */
#define DRV_METROLOGY_CONF_CREEP_QA           0x2e9aUL
/* Metrology Default Config: CREEP QB */
#define DRV_METROLOGY_CONF_CREEP_QB           0x2e9aUL
/* Metrology Default Config: CREEP QC */
#define DRV_METROLOGY_CONF_CREEP_QC           0x0UL
/* Metrology Default Config: CREEP I */
#define DRV_METROLOGY_CONF_CREEP_I            0x212dUL
/* Metrology Default Config: CREEP IA */
#define DRV_METROLOGY_CONF_CREEP_IA           0x212dUL
/* Metrology Default Config: CREEP IB */
#define DRV_METROLOGY_CONF_CREEP_IB           0x212dUL
/* Metrology Default Config: CREEP IC */
#define DRV_METROLOGY_CONF_CREEP_IC           0x0UL
/* Metrology Default Config: CREEP S */
#define DRV_METROLOGY_CONF_CREEP_S            0x2e9aUL
/* Metrology Default Config: SWELLA */
#define DRV_METROLOGY_CONF_SWELLA             0x5e84f62UL
/* Metrology Default Config: SWELLB */
#define DRV_METROLOGY_CONF_SWELLB             0x5e84f62UL
/* Metrology Default Config: SWELLC */
#define DRV_METROLOGY_CONF_SWELLC             0x0UL
/* Metrology Default Config: SAGA */
#define DRV_METROLOGY_CONF_SAGA               0x1a2ec26UL
/* Metrology Default Config: SAGB */
#define DRV_METROLOGY_CONF_SAGB               0x1a2ec26UL
/* Metrology Default Config: SAGC */
#define DRV_METROLOGY_CONF_SAGC               0x0UL
/* Metrology Default Config: INTA */
#define DRV_METROLOGY_CONF_INTA               0x68bb0aUL
/* Metrology Default Config: INTB */
#define DRV_METROLOGY_CONF_INTB               0x68bb0aUL
/* Metrology Default Config: INTC */
#define DRV_METROLOGY_CONF_INTC               0x0UL
/* Metrology Default Config: Current conversion factor */
#define DRV_METROLOGY_CONF_KIA                0x9a523UL
/* Metrology Default Config: Voltage conversion factor */
#define DRV_METROLOGY_CONF_KVA                0x19cc00UL
/* Metrology Default Config: Current conversion factor */
#define DRV_METROLOGY_CONF_KIB                0x9a523UL
/* Metrology Default Config: Voltage conversion factor */
#define DRV_METROLOGY_CONF_KVB                0x19cc00UL
/* Metrology Default Config: Current conversion factor */
#define DRV_METROLOGY_CONF_KIC                0x0UL
/* Metrology Default Config: Voltage conversion factor */
#define DRV_METROLOGY_CONF_KVC                0x0UL
/* Metrology Default Config: Current conversion factor */
#define DRV_METROLOGY_CONF_KIN                0x0UL
/* Metrology Default Config: Voltage conversion factor */
#define DRV_METROLOGY_CONF_KVD                0x0UL
/* Metrology Default Config: AFE_CTRL */
#define DRV_METROLOGY_CONF_AFE_CTRL           0x0UL




// *****************************************************************************
// *****************************************************************************
// Section: Middleware & Other Library Configuration
// *****************************************************************************
// *****************************************************************************


// *****************************************************************************
// *****************************************************************************
// Section: Application Configuration
// *****************************************************************************
// *****************************************************************************


//DOM-IGNORE-BEGIN
#ifdef __cplusplus
}
#endif
//DOM-IGNORE-END

#endif // CONFIGURATION_H
/*******************************************************************************
 End of File
*/
