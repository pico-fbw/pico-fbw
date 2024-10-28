/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2020 Damien P. George
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * This file utilizes code under the MIT License. See "LICENSE" for details.
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#pragma once

#include "lwipopts.h"
#include "pico/stdio_usb.h"

#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE) // Enable USB device mode

/* Class configuration -- enable CDC, NCM, and vendor */

#define CFG_TUD_CDC (1)
#define CFG_TUD_CDC_RX_BUFSIZE (256)
#define CFG_TUD_CDC_TX_BUFSIZE (256)
// Network class has 2 drivers: ECM/RNDIS and NCM
// Only one of the drivers can be enabled
#define CFG_TUD_ECM_RNDIS (0) // FIXME: rndis builds broken?
#define CFG_TUD_NCM (!CFG_TUD_ECM_RNDIS)
#if CFG_TUD_NCM
    // Must be >> MTU
    // Can be set to 2048 without impact
    #define CFG_TUD_NCM_IN_NTB_MAX_SIZE (2 * TCP_MSS + 100)

    // Must be >> MTU
    // Can be set to smaller values if wNtbOutMaxDatagrams==1
    #define CFG_TUD_NCM_OUT_NTB_MAX_SIZE (2 * TCP_MSS + 100)

    // Number of NCM transfer blocks for reception side
    #ifndef CFG_TUD_NCM_OUT_NTB_N
        #define CFG_TUD_NCM_OUT_NTB_N (1)
    #endif

    // Number of NCM transfer blocks for transmission side
    #ifndef CFG_TUD_NCM_IN_NTB_N
        #define CFG_TUD_NCM_IN_NTB_N (1)
    #endif
#endif

// We use a vendor specific interface but with our own driver
// Vendor driver only used for Microsoft OS 2.0 descriptor
#if !PICO_STDIO_USB_RESET_INTERFACE_SUPPORT_MS_OS_20_DESCRIPTOR
    #define CFG_TUD_VENDOR (0)
#else
    #define CFG_TUD_VENDOR (1)
    #define CFG_TUD_VENDOR_RX_BUFSIZE (256)
    #define CFG_TUD_VENDOR_TX_BUFSIZE (256)
#endif
