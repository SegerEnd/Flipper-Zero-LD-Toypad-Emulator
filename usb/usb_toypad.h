#pragma once
#include <furi.h>
#include <furi_hal.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HID_EP_SZ  0x20 // 32 bytes packet size
#define HID_EP_IN  0x81
#define HID_EP_OUT 0x01

#define MAX_TOKENS 7

typedef struct {
    unsigned char index;
    unsigned int id;
    unsigned int pad;
    unsigned char uid[7];
    uint8_t token[180];
    char name[16];
} Token;

typedef struct {
    Token* tokens[8];
    int token_count;
    uint8_t tea_key[16];
} ToyPadEmu;

extern ToyPadEmu* emulator;

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

extern FuriHalUsbInterface usb_hid_ldtoypad;

int32_t hid_toypad_read_IN();

char* get_debug_text_ep_in();
char* get_debug_text_ep_out();

char* get_debug_text();

void set_debug_text(char* text);

void set_debug_text_ep_in(char* text);

usbd_device* get_usb_device();

bool ToyPadEmu_remove(int index, int selectedBox);

Token* createCharacter(int id);

Token* createVehicle(int id, uint32_t upgrades[2]);

int build_frame(Frame* frame, unsigned char* buf);

int build_response(Response* response, unsigned char* buf);

int get_connected_status();
void set_connected_status(int status);

void hexArrayToString(unsigned char* array, int size, char* outputBuffer, int bufferSize);

#ifdef __cplusplus
}
#endif
