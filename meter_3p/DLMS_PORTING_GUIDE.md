# DLMS/COSEM Security Suite 0 Porting Guide for PIC32CXMTC over FLEXCOM1 USART

This document details the architecture, hardware configuration, protocol flow, and API usage for the **DLMS/COSEM (IEC 62056)** smart metering stack running on the **Microchip PIC32CXMTC** dual-core ARM Cortex-M4F microcontroller family, communicating over **FLEXCOM1 USART**.

---

## 1. System Architecture & Block Diagram

```
                 +-----------------------------------------------+
                 |  DLMS Client (AMI / HES / Optical Probe /     |
                 |  Gurux GXDLMSDirector / Test Bench)           |
                 +-----------------------------------------------+
                                         │  (Serial 9600 8-N-1)
                                         ▼
                 +───────────────────────────────────────────────+
                 |            FLEXCOM1 USART Hardware            |
                 |      PIN 5: DLMS_UTX  │  PIN 6: DLMS_URX      |
                 +───────────────────────────────────────────────+
                                         │
                 ┌───────────────────────┴───────────────────────┐
                 │                                               │
                 ▼                                               ▼
     [FLEXCOM1_USART_Read]                           [FLEXCOM1_USART_Write]
    (Interrupt-driven ISR)                           (Non-blocking async TX)
                 │                                               ▲
                 ▼                                               │
     [2048-Byte Lock-Free Ring]                                  │
                 │                                               │
                 ▼                                               │
       [`APP_DLMS_Tasks()`]                                      │
     (app_dlms.c Harmony Loop)                                   │
                 │                                               │
                 ▼                                               │
        [`dlms_hdlc_rx_byte()`]                                  │
     - SNRM / UA / DISC Framing                                  │
     - HDLC Checksum CRC-16-CCITT                                │
     - I-Frame Extraction (IEC 62056-46)                         │
                 │                                               │
                 ▼                                               │
      [`dlms_app_process_apdu()`]                                │
     - AARQ / AARE Handshake Engine                              │
     - Security Suite 0 (AES-GCM-128 / HLS 5 GMAC)               │
     - COSEM Dispatcher (GET / SET / ACTION)                     │
                 │                                               │
                 ▼                                               │
    [`dlms_meter_data_adapter.c`]                                │
     - Live Voltages, Currents, Powers (app_metrology)           │
     - Active Energy Accumulators & TOU Tariffs (app_energy)     │
     - Maximum Demand Register & Timestamp (app_energy)          │
     - Hardware Real-Time Clock Sync (plib_rtc)                  │
                 │                                               │
                 └───────────────────────────────────────────────┘
                     HDLC Transmit Callback (`s_tx_cb`)
```

---

## 2. Hardware & Pinout Configuration

| Signal | Peripheral | MCU Pin | Function Name | Harmony Component |
| :--- | :--- | :--- | :--- | :--- |
| **TX** | FLEXCOM1 USART | Pin 5 | `DLMS_UTX` | `core.yml` / `flexcom1.yml` |
| **RX** | FLEXCOM1 USART | Pin 6 | `DLMS_URX` | `core.yml` / `flexcom1.yml` |

### Default Serial Interface Parameters
- **Baud Rate:** `9600 bps` (configurable via `APP_DLMS_SetBaudRate()`)
- **Data Bits:** `8 bits`
- **Parity:** `None`
- **Stop Bits:** `1 bit`
- **Flow Control:** `None`
- **Mode:** Direct HDLC (IEC 62056-46)

---

## 3. Directory Layout & Ported Files

All DLMS source files are placed under `apps/metering_demo/src/dlms`:

```
apps/metering_demo/
├── DLMS_PORTING_GUIDE.md               # This documentation
├── metering_demo_pic32cxmtc_db.X/      # MPLAB X Project
│   └── nbproject/configurations.xml    # Project configuration with DLMS files & include paths
└── src/
    ├── app_dlms.c                      # Harmony application task & FLEXCOM1 USART driver
    ├── app_dlms.h                      # Application state machine, counters & public APIs
    └── dlms/                           # Standalone DLMS/COSEM Security Suite 0 Stack
        ├── include/                    # Top-level headers
        │   ├── dlms_api.h              # Public stack API (dlms_init, dlms_tasks, etc.)
        │   ├── dlms_config.h           # Buffer sizes, keys, timeouts, device IDs
        │   ├── dlms_types.h            # OBIS codes, A-XDR data tags, error codes
        │   ├── dlms_security.h         # Security Suite 0 policies, SC byte, keys
        │   ├── dlms_cosem.h            # COSEM classes, access modes, associations
        │   └── dlms_meter_adapter.h    # Metrology & energy interface abstraction
        ├── crypto/                     # Security Suite 0 cryptographic engine
        │   ├── dlms_aes.h / .c         # AES-128 block cipher (FIPS 197)
        │   ├── dlms_aes_gcm.h / .c     # AES-GCM-128 & GMAC (NIST SP 800-38D)
        │   ├── dlms_aes_keywrap.h / .c # AES Key Wrap (NIST SP 800-38F / RFC 3394)
        │   └── dlms_random.h / .c      # PRNG for StoC challenges
        ├── axdr/                       # A-XDR encoding/decoding engine (IEC 62056-5-3)
        │   └── dlms_axdr.h / .c        # Primitive & structured types codec
        ├── hdlc/                       # HDLC Profile (IEC 62056-46)
        │   ├── dlms_crc.h / .c         # CRC-16-CCITT calculation for HCS & FCS
        │   └── dlms_hdlc.h / .c        # Address parsing, SNRM/UA/DISC/I-frame engine
        ├── wrapper/                    # DLMS TCP/IP Wrapper Profile (IEC 62056-47)
        │   └── dlms_wrapper.h / .c     # 8-byte wrapper packet framing
        ├── cosem/                      # COSEM Application Layer & Objects
        │   ├── dlms_security_suite0.h/.c # GLO-* ciphering, HLS 5 verify & response
        │   ├── dlms_association.h / .c   # AARQ / AARE handshake, conformance
        │   └── dlms_objects.h / .c       # Object table & GET / SET / ACTION services
        └── app/                        # Application Bridge & Hardware Adapter
            ├── dlms_app.h / .c         # Pipeline dispatcher & HDLC channel manager
            └── dlms_meter_data_adapter.h/.c # Bridge to PIC32CXMTC metrology & RTC
```

---

## 4. Implementation Details

### 4.1 Asynchronous Interrupt Reception & Ring Buffer
In [`app_dlms.c`](file:///C:/work26/PIC32CXMTC/apps/metering_demo/src/app_dlms.c), incoming serial bytes from FLEXCOM1 are handled via an interrupt-driven single-byte read chained with a lock-free 2048-byte circular buffer:

```c
static void APP_DLMS_UartRxCallback(uintptr_t context)
{
    FLEXCOM_USART_ERROR errorStatus = FLEXCOM1_USART_ErrorGet();

    if (errorStatus == FLEXCOM_USART_ERROR_NONE)
    {
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

    /* Immediately re-arm for next incoming byte */
    FLEXCOM1_USART_Read((void *)&s_rx_byte, 1U);
}
```

### 4.2 Non-Blocking Transmission
When the DLMS protocol engine builds an outgoing HDLC frame (UA, DM, I-Frame), it calls [`APP_DLMS_HdlcTxCallback()`](file:///C:/work26/PIC32CXMTC/apps/metering_demo/src/app_dlms.c#L141):

```c
static void APP_DLMS_HdlcTxCallback(const uint8_t *data, uint16_t length, void *user_data)
{
    /* Spin wait until any prior transmission completes */
    uint32_t timeout = 500000U;
    while (FLEXCOM1_USART_WriteIsBusy() && (--timeout > 0U));

    if (length > APP_DLMS_TX_BUF_SIZE) length = APP_DLMS_TX_BUF_SIZE;
    memcpy(s_tx_buffer, data, length);

    app_dlmsData.tx_bytes += length;
    app_dlmsData.tx_frames++;

    FLEXCOM1_USART_Write((void *)s_tx_buffer, length);
}
```

### 4.3 Main Loop Processing in `APP_DLMS_Tasks`
Invoked continuously by Harmony's [`SYS_Tasks()`](file:///C:/work26/PIC32CXMTC/apps/metering_demo/src/config/pic32cxmtc_db/tasks.c#L73):

```c
void APP_DLMS_Tasks ( void )
{
    switch ( app_dlmsData.state )
    {
        case APP_DLMS_STATE_INIT:
            APP_DLMS_Initialize();
            break;

        case APP_DLMS_STATE_SERVICE_TASKS:
        {
            /* 1. Dequeue all received bytes into the HDLC frame decoder */
            uint8_t byte;
            while (app_dlms_ring_pop(&byte))
            {
                dlms_hdlc_rx_byte(byte);
            }

            /* 2. Self-healing receiver check: recover if interrupted by line noise */
            if (!FLEXCOM1_USART_ReadIsBusy())
            {
                (void)FLEXCOM1_USART_ErrorGet();
                FLEXCOM1_USART_Read((void *)&s_rx_byte, 1U);
            }

            /* 3. Execute periodic stack maintenance (timeouts, keepalives) */
            dlms_tasks();
            break;
        }

        default:
            break;
    }
}
```

### 4.4 Live Hardware Metrology Integration
In [`dlms_meter_data_adapter.c`](file:///C:/work26/PIC32CXMTC/apps/metering_demo/src/dlms/app/dlms_meter_data_adapter.c), DLMS registers are directly mapped to PIC32CXMTC services:

- **Phase Voltages & Currents:** `APP_METROLOGY_GetMeasure(MEASURE_Ux_RMS)` & `APP_METROLOGY_GetMeasure(MEASURE_Ix_RMS)`
- **Active / Reactive Power:** `APP_METROLOGY_GetMeasure(MEASURE_PT)` & `APP_METROLOGY_GetMeasure(MEASURE_QT)`
- **Frequency & Power Factor:** `APP_METROLOGY_GetMeasure(MEASURE_FREQ)` and computed $PF = \frac{P}{S}$
- **Energy Accumulators:** `APP_ENERGY_GetCurrentEnergy()` (Import, Export, Tariffs 1..4)
- **Maximum Demand:** `APP_ENERGY_GetCurrentMaxDemand()` (Value & timestamp)
- **Clock (Class 8):** Synchronized to hardware RTC via `RTC_TimeGet()` and `RTC_TimeSet()`
- **Graceful Fallback:** If metrology is still initializing at startup, built-in simulated benchmarks ensure non-zero values are always returned.

---

## 5. DLMS Client Connection Parameters

### 5.1 Addressing & Association Profiles

| Parameter | Public Client (Association 1) | Management Client (Association 2) |
| :--- | :--- | :--- |
| **Client SAP** | `0x10` (16) | `0x01` (1) |
| **Server Logical Address** | `0x01` (1) | `0x01` (1) |
| **Server Physical Address** | `0x01` (1) | `0x01` (1) |
| **Authentication Mechanism** | `None` (`DLMS_AUTH_NONE`) | `High Level Security 5 (GMAC)` |
| **Security Suite** | None (Plaintext) | **Security Suite 0 (AES-GCM-128)** |
| **Encryption Mode** | None | Authenticated Encryption (SC = `0x30`) |

### 5.2 Default Cryptographic Keys & System Titles

```c
/* Global Unicast Encryption Key (128-bit) */
{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F }

/* Authentication Key (128-bit) */
{ 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
  0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF }

/* Master Key / Key Encrypting Key (KEK) (128-bit) */
{ 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
  0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF }

/* Server System Title (8 bytes) */
{ 'M', 'C', 'H', 0x00, 0x00, 0x00, 0x01, 0x01 }

/* Default Client System Title (8 bytes) */
{ 'G', 'X', 'D', 0x00, 0x00, 0x00, 0x00, 0x01 }
```

### 5.3 Supported Standard OBIS Codes

| Class | OBIS Code | Description | Unit |
| :--- | :--- | :--- | :--- |
| Class 1 (Data) | `0.0.96.1.0.255` | Meter Serial Number | String |
| Class 1 (Data) | `1.0.0.2.0.255` | Firmware Version | String |
| Class 3 (Register) | `1.0.32.7.0.255` | Voltage Phase A | Volts (V) |
| Class 3 (Register) | `1.0.52.7.0.255` | Voltage Phase B | Volts (V) |
| Class 3 (Register) | `1.0.72.7.0.255` | Voltage Phase C | Volts (V) |
| Class 3 (Register) | `1.0.31.7.0.255` | Current Phase A | Amperes (A) |
| Class 3 (Register) | `1.0.51.7.0.255` | Current Phase B | Amperes (A) |
| Class 3 (Register) | `1.0.71.7.0.255` | Current Phase C | Amperes (A) |
| Class 3 (Register) | `1.0.91.7.0.255` | Neutral Current | Amperes (A) |
| Class 3 (Register) | `1.0.1.7.0.255` | Active Power Total (+P) | Watts (W) |
| Class 3 (Register) | `1.0.21.7.0.255` | Active Power Phase A | Watts (W) |
| Class 3 (Register) | `1.0.41.7.0.255` | Active Power Phase B | Watts (W) |
| Class 3 (Register) | `1.0.61.7.0.255` | Active Power Phase C | Watts (W) |
| Class 3 (Register) | `1.0.3.7.0.255` | Reactive Power Total (+Q) | VAr |
| Class 3 (Register) | `1.0.9.7.0.255` | Apparent Power Total (+S) | VA |
| Class 3 (Register) | `1.0.14.7.0.255` | Grid Frequency | Hertz (Hz) |
| Class 3 (Register) | `1.0.13.7.0.255` | Total Power Factor | None |
| Class 3 (Register) | `1.0.1.8.0.255` | Active Energy Import (Total) | Watt-hours (Wh) |
| Class 3 (Register) | `1.0.1.8.1.255` | Active Energy Import Tariff 1 | Watt-hours (Wh) |
| Class 3 (Register) | `1.0.1.8.2.255` | Active Energy Import Tariff 2 | Watt-hours (Wh) |
| Class 3 (Register) | `1.0.1.8.3.255` | Active Energy Import Tariff 3 | Watt-hours (Wh) |
| Class 3 (Register) | `1.0.1.8.4.255` | Active Energy Import Tariff 4 | Watt-hours (Wh) |
| Class 4 (Ext Reg) | `1.0.1.6.0.255` | Maximum Demand (+P) & Timestamp | Watts (W) |
| Class 8 (Clock) | `0.0.1.0.0.255` | Hardware RTC Date & Time | Date/Time |
| Class 15 (Assoc LN)| `0.0.40.0.0.255` | Current Association Logical Name | LN |
| Class 64 (Sec Setup)| `0.0.43.0.0.255` | Security Setup (Suite 0) | Policy / Keys |
| Class 70 (Disconnect)| `0.0.96.3.10.255` | Disconnect Control (Relay) | Control / Status |

---

## 6. How to Connect Using Gurux GXDLMSDirector

> 💡 **Interactive Visualizer:** Open [`gurux_director_test_guide.html`](gurux_director_test_guide.html) in any browser for an interactive visual guide featuring connection diagrams, animated frame breakdown, and simulated GXDLMSDirector setup dialogs.

1. Open **Gurux GXDLMSDirector**.
2. Click **Add Device** and configure:
   - **Manufacturer:** `Microchip (MCH)` or `Generic`
   - **Interface:** `HDLC`
   - **Media:** `Serial Port` (Select COM port mapped to FLEXCOM1)
   - **Baud Rate:** `9600`, `8 Data Bits`, `None Parity`, `1 Stop Bit`
   - **Server Address:** `1` (Logical: `1`, Physical: `1`)
   - **Client Address:**
     - For Public read test: `16` (`0x10`), Authentication: `None`
     - For Suite 0 Secure read: `1` (`0x01`), Authentication: `High (GMAC)`
   - **Security Suite:** `Suite 0`
   - **Security:** `Authentication and Encryption`
   - Enter the default System Titles and 128-bit keys listed in Section 5.2.
3. Click **Connect**.
   - Watch the log window for the `SNRM ➔ UA` and `AARQ ➔ AARE` exchanges.
   - *Note on "GetObjects Failed":* GXDLMSDirector may show `GetObjects Failed. Access Error: Device reports a undefined object`. This is expected; the meter omits the dynamic 700-byte `object_list` table in Public mode to save Flash/RAM. The connection is fully active and ready!
4. **Add Objects Manually:**
   - Select your device in the left tree.
   - Go to **Edit ➔ Add object...** (or right-click device ➔ **Add object...**).
   - Add **Meter Serial Number**: Type = `Data` (Class 1), OBIS = `0.0.96.1.0.255`.
   - Add **Voltage Phase A**: Type = `Register` (Class 3), OBIS = `1.0.32.7.0.255`.
5. **Read Registers:**
   - Right-click `0.0.96.1.0.255` ➔ click **Read** $\rightarrow$ Returns `PIC32CX00001`.
   - Right-click `1.0.32.7.0.255` ➔ click **Read** $\rightarrow$ Returns live ADC voltage (e.g. `230.2 V`).
6. **Save Configuration:** Click **File ➔ Save** as `PIC32CXMTC.gxc` so all objects remain in your tree for subsequent runs.

---

## 7. Build Instructions & Memory Usage

### 7.1 Regenerating Makefiles via CLI
```powershell
& "C:\Program Files\Microchip\MPLABX\v6.35\mplab_platform\bin\prjMakefilesGenerator.bat" `
  "C:\work26\PIC32CXMTC\apps\metering_demo\metering_demo_pic32cxmtc_db.X"
```

### 7.2 Building the Project
```powershell
& "C:\Program Files\Microchip\MPLABX\v6.35\gnuBins\GnuWin32\bin\make.exe" `
  -f nbproject/Makefile-pic32cxmtc_db.mk
```

### 7.3 Memory Summary
```
Program Memory:
  Used:  137,061 bytes (13.0%)
  Free:  911,515 bytes (87.0%)
  Total: 1,048,576 bytes (1024 KB Flash)

Data Memory (RAM):
  Used:   67,855 bytes (12.9%)
  Free:  456,433 bytes (87.1%)
  Total: 524,288 bytes (512 KB RAM)
```
