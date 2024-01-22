#pragma once
#include <furi.h>
#include <furi_hal.h>

#ifdef __cplusplus
extern "C" {
#endif

extern FuriHalUsbInterface usb_hid_ldtoypad;

int32_t hid_toypad_read_IN();
// int32_t hid_toypad_read_OUT();
// uint32_t hid_ldtoypad_usbinfo();

#ifdef __cplusplus
}
#endif