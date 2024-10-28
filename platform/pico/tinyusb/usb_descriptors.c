/**
 * This file is based on a file originally part of the
 * MicroPython project, http://micropython.org/
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2019 Damien P. George
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * This file utilizes code under the MIT License. See "LICENSE" for details.
 */

/**
 * Source file of pico-fbw: https://github.com/pico-fbw/pico-fbw
 * Licensed under the GNU GPL-3.0
 */

#include <string.h>
#include "pico/stdio_usb/reset_interface.h"
#include "pico/unique_id.h"
#include "tusb.h"

#include "platform/types.h"

// FIXME: RPI_RESET isn't working?

// A reference for where fields are located in the string descriptor array (usbd_desc_str)
enum StringDescriptorIndex {
    USBD_STR_LANGID = 0,
    USBD_STR_MANUFACTURER,
    USBD_STR_PRODUCT,
    USBD_STR_SERIAL,
    USBD_STR_CDC,
#if PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE
    USBD_STR_RPI_RESET,
#endif
#if CFG_TUD_ECM_RNDIS
    USBD_STR_RNDIS,
    USBD_STR_CDC_ECM,
#else
    USBD_STR_CDC_NCM,
#endif
    USBD_STR_MACADDR,
};

// A reference for where fields are located in the configuration descriptor array (usbd_desc_cfg)
enum InterfaceNumber {
    // CDC needs two interfaces: one for control and one for data
    ITF_NUM_CDC = 0,
    ITF_NUM_CDC_DATA,
#if PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE
    ITF_NUM_RESET,
#endif
#if CFG_TUD_ECM_RNDIS
    ITF_NUM_RNDIS,
    ITF_NUM_CDC_ECM,
#else
    ITF_NUM_CDC_NCM,
#endif
    ITF_NUM_TOTAL
};

/* General device and endpoint configuration */

#ifndef USBD_VID
    #define USBD_VID (0x2E8A) // Raspberry Pi
#endif

#ifndef USBD_PID
    #if PICO_RP2040
        #define USBD_PID (0x000a) // Raspberry Pi Pico SDK CDC for RP2040
    #else
        #define USBD_PID (0x0009) // Raspberry Pi Pico SDK CDC
    #endif
#endif

#ifndef USBD_MANUFACTURER
    #define USBD_MANUFACTURER "Raspberry Pi"
#endif

#ifndef USBD_PRODUCT
    #define USBD_PRODUCT "Pico"
#endif

#define TUD_RPI_RESET_DESC_LEN 9
#if !PICO_STDIO_USB_DEVICE_SELF_POWERED
    #define USBD_CONFIGURATION_DESCRIPTOR_ATTRIBUTE (0)
    #define USBD_MAX_POWER_MA (250)
#else
    #define USBD_CONFIGURATION_DESCRIPTOR_ATTRIBUTE TUSB_DESC_CONFIG_ATT_SELF_POWERED
    #define USBD_MAX_POWER_MA (1)
#endif

#define USBD_CDC_EP_CMD (0x81)
#define USBD_CDC_EP_OUT (0x02)
#define USBD_CDC_EP_IN (0x82)
#define USBD_CDC_CMD_MAX_SIZE (8)
#define USBD_CDC_IN_OUT_MAX_SIZE (64)

#define USBD_NET_EP_NOTIF (0x83)
#define USBD_NET_EP_OUT (0x04)
#define USBD_NET_EP_IN (0x85)

/* Device descriptors */

// Note: descriptors returned from callbacks must exist long enough for transfer to complete

static const tusb_desc_device_t usbd_desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0210,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USBD_VID,
    .idProduct = USBD_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = USBD_STR_MANUFACTURER,
    .iProduct = USBD_STR_PRODUCT,
    .iSerialNumber = USBD_STR_SERIAL,
    .bNumConfigurations = 0x01,
};

const u8 *tud_descriptor_device_cb(void) {
    return (const u8 *)&usbd_desc_device;
}

/* Configuration descriptors */

#define TUD_RPI_RESET_DESCRIPTOR(_itfnum, _stridx)                                                                             \
    /* Interface */                                                                                                            \
    9, TUSB_DESC_INTERFACE, _itfnum, 0, 0, TUSB_CLASS_VENDOR_SPECIFIC, RESET_INTERFACE_SUBCLASS, RESET_INTERFACE_PROTOCOL,     \
        _stridx,

#define BASE_CONFIG_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)
#if PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE
    #define RPI_RESET_LEN (TUD_RPI_RESET_DESC_LEN)
#else
    #define RPI_RESET_LEN (0)
#endif
#define NET_DESC_LEN (CFG_TUD_ECM_RNDIS ? (TUD_RNDIS_DESC_LEN + TUD_CDC_NCM_DESC_LEN) : TUD_CDC_NCM_DESC_LEN)
#define CONFIG_TOTAL_LEN (BASE_CONFIG_LEN + RPI_RESET_LEN + NET_DESC_LEN)

static const u8 usbd_desc_cfg[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, USBD_CONFIGURATION_DESCRIPTOR_ATTRIBUTE, USBD_MAX_POWER_MA),
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC, USBD_STR_CDC, USBD_CDC_EP_CMD, USBD_CDC_CMD_MAX_SIZE, USBD_CDC_EP_OUT, USBD_CDC_EP_IN,
                       USBD_CDC_IN_OUT_MAX_SIZE),
#if PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE
    TUD_RPI_RESET_DESCRIPTOR(ITF_NUM_RESET, USBD_STR_RPI_RESET)
#endif
#if CFG_TUD_ECM_RNDIS
        TUD_RNDIS_DESCRIPTOR(ITF_NUM_RNDIS, USBD_STR_RNDIS, USBD_NET_EP_NOTIF, 8, USBD_NET_EP_OUT, USBD_NET_EP_IN,
                             CFG_TUD_NET_ENDPOINT_SIZE),
    TUD_ECM_DESCRIPTOR(ITF_NUM_CDC_ECM, USBD_STR_CDC_ECM, USBD_STR_MACADDR, USBD_NET_EP_NOTIF, 64, USBD_NET_EP_OUT,
                       USBD_NET_EP_IN, CFG_TUD_NET_ENDPOINT_SIZE, CFG_TUD_NET_MTU),
#else
    TUD_CDC_NCM_DESCRIPTOR(ITF_NUM_CDC_NCM, USBD_STR_CDC_NCM, USBD_STR_MACADDR, USBD_NET_EP_NOTIF, 64, USBD_NET_EP_OUT,
                           USBD_NET_EP_IN, CFG_TUD_NET_ENDPOINT_SIZE, CFG_TUD_NET_MTU),
#endif
};

const u8 *tud_descriptor_configuration_cb(__unused u8 index) {
    return usbd_desc_cfg;
}

/* String descriptors */

static char usbd_serial_str[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];

static const char *const usbd_desc_str[] = {
    [USBD_STR_LANGID] = (const char[]){0x09, 0x04}, // supported language is English (0x0409)
    [USBD_STR_MANUFACTURER] = USBD_MANUFACTURER,
    [USBD_STR_PRODUCT] = USBD_PRODUCT,
    [USBD_STR_SERIAL] = usbd_serial_str,
    [USBD_STR_CDC] = "Board CDC",
#if PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE
    [USBD_STR_RPI_RESET] = "Reset",
#endif
#if CFG_TUD_ECM_RNDIS
    [USBD_STR_RNDIS] = "USB Ethernet RNDIS",
    [USBD_STR_CDC_ECM] = "USB Ethernet ECM",
#else
    [USBD_STR_CDC_NCM] = "USB Ethernet NCM",
#endif
};

const u16 *tud_descriptor_string_cb(u8 index, __unused u16 langid) {
#ifndef USBD_DESC_STR_MAX
    #define USBD_DESC_STR_MAX (20)
#elif USBD_DESC_STR_MAX > 127
    #error USBD_DESC_STR_MAX too high (max is 127).
#elif USBD_DESC_STR_MAX < 17
    #error USBD_DESC_STR_MAX too low (min is 17).
#endif
    static u16 desc_str[USBD_DESC_STR_MAX];

    // If we haven't generated a serial number yet, do so now
    if (!usbd_serial_str[0]) {
        pico_get_unique_board_id_string(usbd_serial_str, sizeof(usbd_serial_str));
    }

    u8 len;
    switch (index) {
        case USBD_STR_LANGID:
            // Special case for language ID string
            memcpy(&desc_str[1], usbd_desc_str[index], 2);
            len = 1;
            break;
        case USBD_STR_MACADDR:
            // Special case for MAC address request: convert MAC address into UTF-16
            for (unsigned i = 0; i < sizeof(tud_network_mac_address); i++) {
                desc_str[1 + i * 2] = "0123456789ABCDEF"[(tud_network_mac_address[i] >> 4) & 0xf];
                desc_str[1 + i * 2 + 1] = "0123456789ABCDEF"[(tud_network_mac_address[i] >> 0) & 0xf];
            }
            len = sizeof(tud_network_mac_address) * 2;
            break;
        default:
            // For all other requests, return the string if it exists
            if (index >= sizeof(usbd_desc_str) / sizeof(usbd_desc_str[0]))
                return NULL;
            // Convert ASCII string into UTF-16
            const char *str = usbd_desc_str[index];
            for (len = 0; len < USBD_DESC_STR_MAX - 1 && str[len]; ++len) {
                desc_str[1 + len] = str[len];
            }
            break;
    }

    // First byte is length (including header), second byte is string type
    desc_str[0] = (u16)((TUSB_DESC_STRING << 8) | (2 * len + 2));
    return desc_str;
}
