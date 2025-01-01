#include <furi_hal_version.h>
#include <furi_hal_usb.h>
#include <furi_hal_usb_hid.h>
#include <furi.h>
#include "usb.h"
#include "usb_hid.h"

#include "../views/EmulateToyPad_scene.h"

#define CMD_WAKE  0xB0
#define CMD_READ  0xD2
#define CMD_MODEL 0xD4
#define CMD_SEED  0xB1
#define CMD_CHAL  0xB3

#define HID_EP_IN  0x81
#define HID_EP_OUT 0x01

#define HID_INTERVAL 2

#define HID_VID_DEFAULT 0x0e6f // Logic3
#define HID_PID_DEFAULT 0x0241

#define USB_EP0_SIZE 64
PLACE_IN_SECTION("MB_MEM2") static uint32_t ubuf[0x20];

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

static void hid_init(usbd_device* dev, FuriHalUsbInterface* intf, void* ctx) {
    UNUSED(intf);
    FuriHalUsbHidConfig* cfg = (FuriHalUsbHidConfig*)ctx;
    if(hid_semaphore == NULL) hid_semaphore = furi_semaphore_alloc(1, 1);
    usb_dev = dev;

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

    int len = usbd_ep_read(dev, HID_EP_IN, debug_text_ep_in, HID_EP_SZ);
    if(len <= 0) return;
}

void hid_out_callback(usbd_device* dev, uint8_t event, uint8_t ep) {
    UNUSED(ep);
    UNUSED(event);

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

    Request request;
    request.cmd = frame.payload[0];
    request.cid = frame.payload[1];
    request.payload_len = frame.len - 2;
    memcpy(request.payload, frame.payload + 2, request.payload_len);

    Response response;
    response.cid = request.cid;
    response.payload_len = 0;

    switch(request.cmd) {
    case CMD_WAKE:
        // handle_cmd_wake(req + 1, res, &res_size);
        sprintf(debug_text, "CMD_WAKE");

        // int8_t wake_payload[HID_EP_SZ] = {0x55, 0x0f, 0xb0, 0x01, 0x28, 0x63, 0x29, 0x20,
        //                                   0x4c, 0x45, 0x47, 0x4f, 0x20, 0x32, 0x30, 0x31,
        //                                   0x34, 0xf7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        //                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

        break;
    case CMD_READ:
        // handle_cmd_read(req + 1, res, &res_size);
        sprintf(debug_text, "CMD_READ");
        break;
    case CMD_MODEL:
        // handle_cmd_model(req + 1, res, &res_size);
        sprintf(debug_text, "CMD_READ");
        break;
    default:
        snprintf(debug_text, HID_EP_SZ, "U: %02X", request.cmd);
        return;
    }

    // Make the response
    unsigned char res_buf[HID_EP_SZ];

    // check if the response is empty
    if(response.payload_len == 0) return;

    int res_len = build_response(&response, res_buf);

    if(res_len <= 0) return;

    // Send the response
    usbd_ep_write(dev, HID_EP_IN, res_buf, res_len);
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
