#pragma once
#include <furi.h>
#include <furi_hal.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HID_EP_SZ 0x20

typedef struct {
    unsigned char type;
    unsigned char len;
    unsigned char payload[HID_EP_SZ];
    unsigned char chksum;
} Frame;

// Define a Response structure
typedef struct {
    Frame frame;
    unsigned char cid;
    unsigned char payload[HID_EP_SZ];
    int payload_len;
    // int _cancel;
    // int _preventDefault;
} Response;

// Define a Request structure
typedef struct {
    Frame frame;
    unsigned char cmd;
    unsigned char cid;
    unsigned char payload[HID_EP_SZ - 2];
    int payload_len;
} Request;

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
