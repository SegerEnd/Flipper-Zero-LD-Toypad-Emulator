#pragma once
#include <furi.h>
#include <furi_hal.h>

#ifdef __cplusplus
extern "C" {
#endif

extern FuriHalUsbInterface usb_hid_ldtoypad;

// typedef enum {
//     HidDisconnected,
//     HidConnected,
//     HidRequest,
// } HidEvent;

int32_t hid_toypad_read_IN();
// int32_t hid_toypad_read_OUT();
// uint32_t hid_ldtoypad_usbinfo();

char* get_debug_text_ep_in();
char* get_debug_text_ep_out();

char* get_debug_text();

usbd_device* get_usb_device();

#ifdef __cplusplus
}
#endif