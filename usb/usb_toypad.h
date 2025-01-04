#pragma once
#include <furi.h>
#include <furi_hal.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HID_EP_SZ 0x20

typedef struct {
    int index;
    int id;
    int pad;
    char uid[7];
    char token[180];
} Token;

typedef struct {
    Token tokens[10];
    int token_count;
    void (*transport_write)(const char* data);
    uint8_t tea_key[16];
} ToyPadEmu;

typedef struct {
    unsigned char type;
    unsigned char len;
    unsigned char payload[HID_EP_SZ - 2];
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

// Event structure

typedef struct {
    Frame frame;
    int pad;
    int index;
    int dir;
    int uid[7];
} Event;

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

void set_debug_text(char* text);

void set_debug_text_ep_in(char* text);

usbd_device* get_usb_device();

void ToyPadEmu_randomUID(unsigned char* uid);

Token createCharacter(int id, unsigned char* uid);

void ToyPadEmu_place(ToyPadEmu* emu, int pad, int index, unsigned char* uid);

void Event_init(Event* event);

int Event_build(Event* event, unsigned char* buf);

int build_frame(Frame* frame, unsigned char* buf);

int build_response(Response* response, unsigned char* buf);

ToyPadEmu* get_emulator();

bool get_connected_status();

void hexArrayToString(char* array, int size, char* outputBuffer, int bufferSize);

#ifdef __cplusplus
}
#endif
