#include <furi_hal_version.h>
#include <furi_hal_usb.h>
#include <furi_hal_usb_hid.h>
#include <furi.h>
#include "usb.h"
#include "usb_hid.h"

#include "../views/EmulateToyPad_scene.h"
#include "../tea.h"
#include "../burtle.h"

// Define all the possible commands
#define CMD_WAKE   0xB0
#define CMD_READ   0xD2
#define CMD_MODEL  0xD4
#define CMD_SEED   0xB1
#define CMD_CHAL   0xB3
#define CMD_COL    0xC0
#define CMD_GETCOL 0xC1
#define CMD_FADE   0xC2
#define CMD_FLASH  0xC3
#define CMD_FADRD  0xC4
#define CMD_FADAL  0xC6
#define CMD_FLSAL  0xC7
#define CMD_COLAL  0xC8
#define CMD_TGLST  0xD0
#define CMD_WRITE  0xD3
#define CMD_PWD    0xE1
#define CMD_ACTIVE 0xE5
#define CMD_LEDSQ  0xFF

#define HID_EP_IN  0x81
#define HID_EP_OUT 0x01

#define HID_INTERVAL 2

#define HID_VID_DEFAULT 0x0e6f // Logic3
#define HID_PID_DEFAULT 0x0241

#define USB_EP0_SIZE 64
PLACE_IN_SECTION("MB_MEM2") static uint32_t ubuf[0x20];

// Function to parse a Frame into a Request
void parse_request(Request* request, Frame* f) {
    if(request == NULL || f == NULL) return;

    request->frame = *f;
    uint8_t* p = f->payload;

    request->cmd = p[0];
    request->cid = p[1];
    memcpy(request->payload, p + 2, f->len - 2); // Copy payload, excluding cmd and cid
}

// Function to parse a Frame from a buffer
void parse_frame(Frame* frame, unsigned char* buf, int len) {
    UNUSED(len);
    frame->type = buf[0];
    frame->len = buf[1];
    memcpy(frame->payload, buf + 2, frame->len);
    frame->chksum = buf[frame->len + 2];
}

// Function to calculate checksum
unsigned char calculate_checksum(unsigned char* buf, int len) {
    unsigned char sum = 0;
    for(int i = 0; i < len; i++) {
        sum += buf[i];
    }
    return sum % 256;
}

// Function to build a Frame into a buffer
int build_frame(Frame* frame, unsigned char* buf) {
    buf[0] = frame->type;
    buf[1] = frame->len;
    memcpy(buf + 2, frame->payload, frame->len);
    buf[frame->len + 2] = calculate_checksum(buf, frame->len + 2);
    return frame->len + 3;
}

// Function to parse a Response from a Frame
void parse_response(Response* response, Frame* frame) {
    response->frame = *frame;
    response->cid = frame->payload[0];
    response->payload_len = frame->len - 1;
    memcpy(response->payload, frame->payload + 1, response->payload_len);
}

// Function to build a Response into a Frame
int build_response(Response* response, unsigned char* buf) {
    response->frame.type = 0x55;
    response->frame.len = response->payload_len + 1;
    response->frame.payload[0] = response->cid;
    memcpy(response->frame.payload + 1, response->payload, response->payload_len);
    return build_frame(&response->frame, buf);
}

void Event_init(Event* event, unsigned char* data, int len) {
    if(data && len > 0) {
        Frame frame;
        parse_frame(&frame, data, len);
        event->pad = frame.payload[0];
        event->index = frame.payload[2];
        event->dir = frame.payload[3];
        memcpy(event->uid, frame.payload + 4, 16);
        event->frame = frame;
    } else {
        event->pad = 0;
        event->index = 0;
        event->dir = 0;
        memset(event->uid, 0, sizeof(event->uid));
    }
}

// Function to build the event into a frame
int Event_build(Event* event, unsigned char* buf) {
    unsigned char b[11] = {0};
    b[0] = event->pad;
    b[1] = 0;
    b[2] = event->index;
    b[3] = event->dir & 0x1; // Direction is either 0 or 1
    memcpy(b + 4, event->uid, sizeof(event->uid));

    // Update the event's frame
    event->frame.type = 0x56;
    event->frame.len = 11;
    memcpy(event->frame.payload, b, 11);

    // Build the frame and return the size of the frame
    return build_frame(&event->frame, buf);
}

/* String descriptors */
enum UsbDevDescStr {
    UsbDevLang = 0,
    UsbDevManuf = 1,
    UsbDevProduct = 2,
    UsbDevSerial = 3,
};

struct HidIntfDescriptor {
    struct usb_interface_descriptor hid;
    struct usb_hid_descriptor hid_desc;
    struct usb_endpoint_descriptor hid_ep_in;
    struct usb_endpoint_descriptor hid_ep_out;
};

struct HidConfigDescriptor {
    struct usb_config_descriptor config;
    struct HidIntfDescriptor intf_0;
} __attribute__((packed));

/* HID report descriptor */
static const uint8_t hid_report_desc[] = {
    0x06, 0x00, 0xFF, // Usage Page (Vendor Defined)
    0x09, 0x01, // Usage (Vendor Usage 1)
    0xA1, 0x01, // Collection (Application)
    0x19, 0x01, //   Usage Minimum (Vendor Usage 1)
    0x29, 0x20, //   Usage Maximum (Vendor Usage 32)
    0x15, 0x00, //   Logical Minimum (0)
    0x26, 0xFF, 0x00, //   Logical Maximum (255)
    0x75, 0x08, //   Report Size (8 bits)
    0x95, 0x20, //   Report Count (32 bytes)
    0x81, 0x00, //   Input (Data, Array, Absolute)
    0x19, 0x01, //   Usage Minimum (Vendor Usage 1)
    0x29, 0x20, //   Usage Maximum (Vendor Usage 32)
    0x91, 0x00, //   Output (Data, Array, Absolute)
    0xC0 // End Collection
};

/* Device descriptor */
static struct usb_device_descriptor hid_device_desc = {
    .bLength = sizeof(struct usb_device_descriptor),
    .bDescriptorType = USB_DTYPE_DEVICE,
    .bcdUSB = VERSION_BCD(2, 0, 0),
    .bDeviceClass = USB_CLASS_PER_INTERFACE,
    .bDeviceSubClass = USB_SUBCLASS_NONE,
    .bDeviceProtocol = USB_PROTO_NONE,
    .bMaxPacketSize0 = USB_EP0_SIZE,
    .idVendor = HID_VID_DEFAULT,
    .idProduct = HID_PID_DEFAULT,
    .bcdDevice = VERSION_BCD(1, 0, 0),
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1,
};

/* Device configuration descriptor */
static const struct HidConfigDescriptor hid_cfg_desc = {
    .config =
        {
            .bLength = sizeof(struct usb_config_descriptor),
            .bDescriptorType = USB_DTYPE_CONFIGURATION,
            .wTotalLength = sizeof(struct HidConfigDescriptor),
            .bNumInterfaces = 1,
            .bConfigurationValue = 1,
            .iConfiguration = NO_DESCRIPTOR,
            .bmAttributes = USB_CFG_ATTR_RESERVED,
            .bMaxPower = USB_CFG_POWER_MA(500),
        },
    .intf_0 =
        {
            .hid =
                {
                    .bLength = sizeof(struct usb_interface_descriptor),
                    .bDescriptorType = USB_DTYPE_INTERFACE,
                    .bInterfaceNumber = 0,
                    .bAlternateSetting = 0,
                    .bNumEndpoints = 2,
                    .bInterfaceClass = USB_CLASS_HID,
                    .bInterfaceSubClass = USB_HID_SUBCLASS_NONBOOT,
                    .bInterfaceProtocol = USB_HID_PROTO_NONBOOT,
                    .iInterface = NO_DESCRIPTOR,
                },
            .hid_desc =
                {
                    .bLength = sizeof(struct usb_hid_descriptor),
                    .bDescriptorType = USB_DTYPE_HID,
                    .bcdHID = VERSION_BCD(1, 0, 0),
                    .bCountryCode = USB_HID_COUNTRY_NONE,
                    .bNumDescriptors = 1,
                    .bDescriptorType0 = USB_DTYPE_HID_REPORT,
                    .wDescriptorLength0 = sizeof(hid_report_desc),
                },
            .hid_ep_in =
                {
                    .bLength = sizeof(struct usb_endpoint_descriptor),
                    .bDescriptorType = USB_DTYPE_ENDPOINT,
                    .bEndpointAddress = HID_EP_IN,
                    .bmAttributes = USB_EPTYPE_INTERRUPT,
                    .wMaxPacketSize = HID_EP_SZ,
                    .bInterval = HID_INTERVAL,
                },
            .hid_ep_out =
                {
                    .bLength = sizeof(struct usb_endpoint_descriptor),
                    .bDescriptorType = USB_DTYPE_ENDPOINT,
                    .bEndpointAddress = HID_EP_OUT,
                    .bmAttributes = USB_EPTYPE_INTERRUPT,
                    .wMaxPacketSize = HID_EP_SZ,
                    .bInterval = HID_INTERVAL,
                },
        },
};

static void hid_init(usbd_device* dev, FuriHalUsbInterface* intf, void* ctx);
static void hid_deinit(usbd_device* dev);
static void hid_on_wakeup(usbd_device* dev);
static void hid_on_suspend(usbd_device* dev);

FuriHalUsbInterface usb_hid_ldtoypad = {
    .init = hid_init,
    .deinit = hid_deinit,
    .wakeup = hid_on_wakeup,
    .suspend = hid_on_suspend,

    .dev_descr = (struct usb_device_descriptor*)&hid_device_desc,

    .str_manuf_descr = NULL,
    .str_prod_descr = NULL,
    .str_serial_descr = NULL,

    .cfg_descr = (void*)&hid_cfg_desc,
};

// static bool hid_send_report(uint8_t report_id);
static usbd_respond hid_ep_config(usbd_device* dev, uint8_t cfg);
static usbd_respond hid_control(usbd_device* dev, usbd_ctlreq* req, usbd_rqc_callback* callback);
static usbd_device* usb_dev;
static FuriSemaphore* hid_semaphore = NULL;
static bool hid_connected = false;
static HidStateCallback callback;
static void* cb_ctx;
// static uint8_t led_state;
static bool boot_protocol = false;

bool furi_hal_hid_is_connected() {
    return hid_connected;
}

void furi_hal_hid_set_state_callback(HidStateCallback cb, void* ctx) {
    if(callback != NULL) {
        if(hid_connected == true) callback(false, cb_ctx);
    }

    callback = cb;
    cb_ctx = ctx;

    if(callback != NULL) {
        if(hid_connected == true) callback(true, cb_ctx);
    }
}

static void* hid_set_string_descr(char* str) {
    furi_assert(str);

    size_t len = strlen(str);
    struct usb_string_descriptor* dev_str_desc = malloc(len * 2 + 2);
    dev_str_desc->bLength = len * 2 + 2;
    dev_str_desc->bDescriptorType = USB_DTYPE_STRING;
    for(size_t i = 0; i < len; i++)
        dev_str_desc->wString[i] = str[i];

    return dev_str_desc;
}

usbd_device* get_usb_device() {
    return usb_dev;
}

ToyPadEmu* emulator;
ToyPadEmu* get_emulator() {
    return emulator;
}

Burtle* burtle; // Define the Burtle object

// Generate random UID
void ToyPadEmu_randomUID(char* uid) {
    srand(furi_get_tick()); // Set the seed to random value
    uid[0] = 0x04; // vendor id = NXP
    uid[6] = 0x80; // Set last byte to 0x80
    for(int i = 1; i < 6; i++) {
        uid[i] = rand() % 256;
    }
    uid[7] = '\0'; // null terminate
}

void ToyPadEmu_init(ToyPadEmu* emu) {
    emu->token_count = 0;

    // Set default TEA key
    // uint8_t default_tea_key[16] = {
    //     0x55,
    //     0xFE,
    //     0xF6,
    //     0xB0,
    //     0x62,
    //     0xBF,
    //     0x0B,
    //     0x41,
    //     0xC9,
    //     0xB3,
    //     0x7C,
    //     0xB4,
    //     0x97,
    //     0x3E,
    //     0x29,
    //     0x7B};

    // memcpy(emu->tea_key, default_tea_key, sizeof(emu->tea_key));
}

Token createCharacter(int id, const char* uid) {
    Token token; // Declare a token structure
    // memset(token.data, 0, sizeof(token.data)); // Fill the array with zeros
    strncpy(token.uid, uid, sizeof(token.uid)); // Set the UID
    token.id = id; // Set the ID
    return token; // Return the created token
}

// Add a token to a pad
void ToyPadEmu_place(ToyPadEmu* emu, int pad, int index, const char* uid) {
    if(emu->token_count > 7) {
        return;
    }

    Token new_token = createCharacter(index, uid);
    new_token.index = index;
    new_token.pad = pad;

    emu->tokens[emu->token_count++] = new_token;

    // send to usb
    // make a event
    Event event;
    Event_init(&event, NULL, 0);

    // set the pad
    event.pad = pad;
    event.index = index;
    memcpy(event.uid, uid, 8);

    // build the event
    unsigned char buf[HID_EP_SZ];
    int len = Event_build(&event, buf);

    if(len == 0) {
        set_debug_text("Length of event is 0");
        return;
    }

    // send the event
    usbd_ep_write(usb_dev, HID_EP_IN, buf, sizeof(buf));

    return;
}

// Remove a token
bool ToyPadEmu_remove(ToyPadEmu* emu, int index) {
    for(int i = 0; i < emu->token_count; i++) {
        if(emu->tokens[i].index == index) {
            // Shift tokens
            for(int j = i; j < emu->token_count - 1; j++) {
                emu->tokens[j] = emu->tokens[j + 1];
            }
            emu->token_count--;
            return true;
        }
    }
    return false;
}

static void hid_init(usbd_device* dev, FuriHalUsbInterface* intf, void* ctx) {
    UNUSED(intf);
    FuriHalUsbHidConfig* cfg = (FuriHalUsbHidConfig*)ctx;
    if(hid_semaphore == NULL) hid_semaphore = furi_semaphore_alloc(1, 1);
    usb_dev = dev;

    if(emulator == NULL) emulator = malloc(sizeof(ToyPadEmu));
    if(burtle == NULL) burtle = malloc(sizeof(Burtle));

    // hid_report.keyboard.report_id = ReportIdKeyboard;
    // hid_report.mouse.report_id = ReportIdMouse;
    // hid_report.consumer.report_id = ReportIdConsumer;

    usb_hid.dev_descr->iManufacturer = 0;
    usb_hid.dev_descr->iProduct = 0;
    usb_hid.str_manuf_descr = NULL;
    usb_hid.str_prod_descr = NULL;
    usb_hid.dev_descr->idVendor = HID_VID_DEFAULT;
    usb_hid.dev_descr->idProduct = HID_PID_DEFAULT;

    if(cfg != NULL) {
        usb_hid.dev_descr->idVendor = cfg->vid;
        usb_hid.dev_descr->idProduct = cfg->pid;

        if(cfg->manuf[0] != '\0') {
            usb_hid.str_manuf_descr = hid_set_string_descr(cfg->manuf);
            usb_hid.dev_descr->iManufacturer = UsbDevManuf;
        }

        if(cfg->product[0] != '\0') {
            usb_hid.str_prod_descr = hid_set_string_descr(cfg->product);
            usb_hid.dev_descr->iProduct = UsbDevProduct;
        }
    }

    usbd_reg_config(dev, hid_ep_config);
    usbd_reg_control(dev, hid_control);

    // Manually initialize the USB because of the custom modiefied USB_EP0_SIZE to 64 from the default 8
    // I did this because the official Toy Pad has a 64 byte EP0 size
    usbd_init(dev, &usbd_hw, USB_EP0_SIZE, ubuf, sizeof(ubuf));

    usbd_connect(dev, true);
}

static void hid_deinit(usbd_device* dev) {
    // Set the USB Endpoint 0 size back to 8
    usbd_init(dev, &usbd_hw, 8, ubuf, sizeof(ubuf));

    usbd_reg_config(dev, NULL);
    usbd_reg_control(dev, NULL);

    // free(callback);
    // free(cb_ctx);

    free(usb_hid_ldtoypad.str_manuf_descr);
    free(usb_hid_ldtoypad.str_prod_descr);

    free(emulator);
    free(burtle);
}

static void hid_on_wakeup(usbd_device* dev) {
    UNUSED(dev);
    if(!hid_connected) {
        hid_connected = true;
        if(callback != NULL) {
            callback(true, cb_ctx);
        }
    }
}

static void hid_on_suspend(usbd_device* dev) {
    UNUSED(dev);
    if(hid_connected) {
        hid_connected = false;
        furi_semaphore_release(hid_semaphore);
        if(callback != NULL) {
            callback(false, cb_ctx);
        }
    }
}

// create a string variablethat contains the text: nothing to debug yet
char debug_text_ep_in[HID_EP_SZ] = "nothing";

// char debug_text_ep_out[] = "nothing to debug yet";
char debug_text_ep_out[HID_EP_SZ] = "nothing";

char debug_text[HID_EP_SZ] = " ";

void set_debug_text(char* text) {
    sprintf(debug_text, "%s", text);
}

// a function that returns a pointer to the string
char* get_debug_text_ep_in() {
    return debug_text_ep_in;
}
char* get_debug_text_ep_out() {
    return debug_text_ep_out;
}
char* get_debug_text() {
    return debug_text;
}

void hid_in_callback(usbd_device* dev, uint8_t event, uint8_t ep) {
    UNUSED(ep);
    UNUSED(event);
    // Handle the IN endpoint

    unsigned char in_buf[HID_EP_SZ] = {0};

    int len = usbd_ep_read(dev, HID_EP_IN, in_buf, HID_EP_SZ);

    sprintf(debug_text_ep_in, "%s", in_buf);

    if(len <= 0) return;
}

uint32_t readUInt32LE(const unsigned char* buffer, int offset) {
    return (uint32_t)buffer[offset] | ((uint32_t)buffer[offset + 1] << 8) |
           ((uint32_t)buffer[offset + 2] << 16) | ((uint32_t)buffer[offset + 3] << 24);
}

uint32_t readUInt32BE(const unsigned char* buffer, int offset) {
    return ((uint32_t)buffer[offset] << 24) | ((uint32_t)buffer[offset + 1] << 16) |
           ((uint32_t)buffer[offset + 2] << 8) | (uint32_t)buffer[offset + 3];
}

// Function to write uint32_t to little-endian
void writeUInt32LE(uint8_t* buffer, uint32_t value) {
    buffer[0] = value & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
    buffer[2] = (value >> 16) & 0xFF;
    buffer[3] = (value >> 24) & 0xFF;
}

// Function to write uint32_t to big-endian
void writeUInt32BE(uint8_t* buffer, uint32_t value) {
    buffer[0] = (value >> 24) & 0xFF;
    buffer[1] = (value >> 16) & 0xFF;
    buffer[2] = (value >> 8) & 0xFF;
    buffer[3] = value & 0xFF;
}

void hid_out_callback(usbd_device* dev, uint8_t event, uint8_t ep) {
    UNUSED(ep);
    UNUSED(event);

    usb_dev = dev;

    unsigned char req_buf[HID_EP_SZ] = {0};

    // Read data from the OUT endpoint
    int32_t len = usbd_ep_read(dev, HID_EP_OUT, req_buf, HID_EP_SZ);

    // Make from the data a string and save it to the debug_text_ep_in string
    sprintf(debug_text_ep_out, "%s", req_buf);

    if(len <= 0) return;

    // uint8_t cmd = req_buf[2]; // Command ID
    // uint8_t cid = req[1];
    // uint8_t* payload = &req[3]; // Payload starts from the third byte

    Frame frame;
    parse_frame(&frame, req_buf, len);

    if(frame.len == 0) {
        return;
    }

    Request request;

    memset(&request, 0, sizeof(Request));

    // parse request
    parse_request(&request, &frame);

    // request.cmd = frame.payload[0];
    // request.cid = frame.payload[1];
    // request.payload_len = frame.len - 2;
    // memcpy(request.payload, frame.payload + 2, request.payload_len);

    Response response;
    memset(&response, 0, sizeof(Response));

    response.cid = request.cid;
    response.payload_len = 0;

    uint32_t conf;

    switch(request.cmd) {
    case CMD_WAKE:
        // handle_cmd_wake(req + 1, res, &res_size);
        sprintf(debug_text, "CMD_WAKE");

        ToyPadEmu_init(emulator); // Initialize the emulator / setup tea key

        uint8_t default_tea_key[16] = {
            0x55,
            0xFE,
            0xF6,
            0xB0,
            0x62,
            0xBF,
            0x0B,
            0x41,
            0xC9,
            0xB3,
            0x7C,
            0xB4,
            0x97,
            0x3E,
            0x29,
            0x7B};

        memcpy(emulator->tea_key, default_tea_key, sizeof(emulator->tea_key));

        // int8_t wake_payload[HID_EP_SZ] = {0x55, 0x0f, 0xb0, 0x01, 0x28, 0x63, 0x29, 0x20,
        //                                   0x4c, 0x45, 0x47, 0x4f, 0x20, 0x32, 0x30, 0x31,
        //                                   0x34, 0xf7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        //                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        // unsigned char wake_payload[HID_EP_SZ] = {0x55, 0x0f, 0xb0, 0x01, 0x28, 0x63, 0x29, 0x20,
        //                                          0x4c, 0x45, 0x47, 0x4f, 0x20, 0x32, 0x30, 0x31,
        //                                          0x34, 0xf7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        //                                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        unsigned char wake_payload[HID_EP_SZ] = {0x55, 0x19, 0x01, 0x00, 0x2f, 0x02, 0x01, 0x02,
                                                 0x02, 0x04, 0x02, 0xf5, 0x00, 0x19, 0x8b, 0x54,
                                                 0x4d, 0xb4, 0xcd, 0xae, 0x45, 0x24, 0x80, 0x0e,
                                                 0x00, 0xf0, 0x25, 0x20, 0x00, 0x00, 0x00, 0x00};

        // furi_delay_ms(50);

        usbd_ep_write(dev, HID_EP_IN, wake_payload, sizeof(wake_payload));

        // response.payload_len = sizeof(wake_payload);
        // memcpy(response.payload, wake_payload, response.payload_len);

        break;
    case CMD_READ:
        // handle_cmd_read(req + 1, res, &res_size);
        sprintf(debug_text, "CMD_READ");
        break;
    case CMD_MODEL:
        // handle_cmd_model(req + 1, res, &res_size);
        sprintf(debug_text, "CMD_MODEL");
        break;
    case CMD_SEED:
        sprintf(debug_text, "CMD_SEED");

        // decrypt the request.payload with the TEA
        tea_decrypt(request.payload, emulator->tea_key, request.payload);

        uint32_t seed = readUInt32LE(request.payload, 0);

        conf = readUInt32BE(request.payload, 4);

        burtle_init(burtle, seed);

        memset(response.payload, 0, 8); // Fill the payload with 0 with a length of 8
        writeUInt32BE(response.payload, conf); // Write the conf to the payload

        // encrypt the request.payload with the TEA
        tea_encrypt(response.payload, emulator->tea_key, response.payload);

        response.payload_len = 8;

        break;
    case CMD_WRITE:
        sprintf(debug_text, "CMD_WRITE");
        break;
    case CMD_CHAL:
        sprintf(debug_text, "CMD_CHAL");

        // decrypt the request.payload with the TEA
        tea_decrypt(request.payload, emulator->tea_key, request.payload);

        // get conf
        conf = readUInt32BE(request.payload, 0);

        // make a new buffer for the response of 8
        memset(response.payload, 0, 8);

        // get a rand from the burtle
        uint32_t rand = burtle_rand(burtle);

        // write the rand to the response payload as Int32LE
        writeUInt32LE(response.payload, rand);

        // write the conf to the response payload as Int32BE
        writeUInt32BE(response.payload + 4, conf);

        // encrypt the response.payload with the TEA
        tea_encrypt(response.payload, emulator->tea_key, response.payload);

        response.payload_len = 8;

        break;
    case CMD_COL:
        sprintf(debug_text, "CMD_COL");
        break;
    case CMD_GETCOL:
        sprintf(debug_text, "CMD_GETCOL");
        break;
    case CMD_FADE:
        sprintf(debug_text, "CMD_FADE");
        break;
    case CMD_FLASH:
        sprintf(debug_text, "CMD_FLASH");
        break;
    case CMD_FADRD:
        sprintf(debug_text, "CMD_FADRD");
        break;
    case CMD_FADAL:
        sprintf(debug_text, "CMD_FADAL");
        break;
    case CMD_FLSAL:
        sprintf(debug_text, "CMD_FLSAL");
        break;
    case CMD_COLAL:
        sprintf(debug_text, "CMD_COLAL");
        break;
    case CMD_TGLST:
        sprintf(debug_text, "CMD_TGLST");
        break;
    case CMD_PWD:
        sprintf(debug_text, "CMD_PWD");
        break;
    case CMD_ACTIVE:
        sprintf(debug_text, "CMD_ACTIVE");
        break;
    case CMD_LEDSQ:
        sprintf(debug_text, "CMD_LEDSQ");
        break;
    default:
        // snprintf(debug_text, HID_EP_SZ, "ERR: %02X", request.cmd);
        sprintf(debug_text, "Not a valid command");
        return;
    }

    // check if the response is empty
    if(sizeof(response.payload) == 0) {
        // sprintf(debug_text, "Empty payload_len");
        return;
    }
    if(response.payload_len > HID_EP_SZ) {
        sprintf(debug_text, "Payload too big");
        return;
    }

    // Make the response
    unsigned char res_buf[HID_EP_SZ];

    build_response(&response, res_buf);
    int res_len = build_frame(&response.frame, res_buf);

    if(res_len <= 0) {
        sprintf(debug_text, "res_len is 0");
        return;
    }

    // Send the response
    usbd_ep_write(dev, HID_EP_IN, res_buf, sizeof(res_buf));
}

// function to write a buffer to the HID EP_IN
void write_to_ep_in(unsigned char* buffer) {
    sprintf(debug_text, "Writing to EP_IN");
    usbd_ep_write(usb_dev, HID_EP_IN, buffer, sizeof(buffer));
}

/* Configure endpoints */
static usbd_respond hid_ep_config(usbd_device* dev, uint8_t cfg) {
    switch(cfg) {
    case 0:
        /* deconfiguring device */
        usbd_ep_deconfig(dev, HID_EP_IN);
        usbd_ep_deconfig(dev, HID_EP_OUT);

        usbd_reg_endpoint(dev, HID_EP_IN, 0);
        usbd_reg_endpoint(dev, HID_EP_OUT, 0);
        return usbd_ack;
    case 1:
        /* configuring device */
        usbd_ep_config(dev, HID_EP_IN, USB_EPTYPE_INTERRUPT, HID_EP_SZ);
        usbd_reg_endpoint(dev, HID_EP_IN, hid_in_callback);
        usbd_ep_config(dev, HID_EP_OUT, USB_EPTYPE_INTERRUPT, HID_EP_SZ);
        usbd_reg_endpoint(dev, HID_EP_OUT, hid_out_callback);
        // usbd_ep_write(dev, HID_EP_IN, 0, 0);
        // int8_t initPacket[32] = {0x55, 0x0f, 0xb0, 0x01, 0x28, 0x63, 0x29, 0x20, 0x4c, 0x45, 0x47,
        //                          0x4f, 0x20, 0x32, 0x30, 0x31, 0x34, 0xf7, 0x00, 0x00, 0x00, 0x00,
        //                          0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        // usbd_ep_write(usb_dev, HID_EP_OUT, initPacket, sizeof(initPacket));
        // usbd_ep_write(usb_dev, HID_EP_IN, initPacket, sizeof(initPacket));
        boot_protocol = false; /* BIOS will SET_PROTOCOL if it wants this */
        return usbd_ack;
    default:
        return usbd_fail;
    }
}

/* Control requests handler */
static usbd_respond hid_control(usbd_device* dev, usbd_ctlreq* req, usbd_rqc_callback* callback) {
    UNUSED(callback);
    /* HID control requests */
    if(((USB_REQ_RECIPIENT | USB_REQ_TYPE) & req->bmRequestType) ==
           (USB_REQ_INTERFACE | USB_REQ_CLASS) &&
       req->wIndex == 0) {
        switch(req->bRequest) {
        case USB_HID_SETIDLE:
            return usbd_ack;
        // case USB_HID_GETREPORT:
        //     if(boot_protocol == true) {
        //         dev->status.data_ptr = &hid_report.keyboard.boot;
        //         dev->status.data_count = sizeof(hid_report.keyboard.boot);
        //     } else {
        //         dev->status.data_ptr = &hid_report;
        //         dev->status.data_count = sizeof(hid_report);
        //     }
        //     return usbd_ack;
        case USB_HID_SETPROTOCOL:
            if(req->wValue == 0)
                boot_protocol = true;
            else if(req->wValue == 1)
                boot_protocol = false;
            else
                return usbd_fail;
            return usbd_ack;
        default:
            return usbd_fail;
        }
    }
    if(((USB_REQ_RECIPIENT | USB_REQ_TYPE) & req->bmRequestType) ==
           (USB_REQ_INTERFACE | USB_REQ_STANDARD) &&
       req->wIndex == 0 && req->bRequest == USB_STD_GET_DESCRIPTOR) {
        switch(req->wValue >> 8) {
        case USB_DTYPE_HID:
            dev->status.data_ptr = (uint8_t*)&(hid_cfg_desc.intf_0.hid_desc);
            dev->status.data_count = sizeof(hid_cfg_desc.intf_0.hid_desc);
            return usbd_ack;
        case USB_DTYPE_HID_REPORT:
            boot_protocol = false; /* BIOS does not read this */
            dev->status.data_ptr = (uint8_t*)hid_report_desc;
            dev->status.data_count = sizeof(hid_report_desc);
            return usbd_ack;
        default:
            return usbd_fail;
        }
    }
    return usbd_fail;
}

//         int8_t data[32] = {0x55, 0x0f, 0xb0, 0x01, 0x28, 0x63, 0x29, 0x20, 0x4c, 0x45, 0x47,
//                            0x4f, 0x20, 0x32, 0x30, 0x31, 0x34, 0xf7, 0x00, 0x00, 0x00, 0x00,
//                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//         int32_t length = usbd_ep_write(usb_dev, HID_EP_IN, data, sizeof(data));
