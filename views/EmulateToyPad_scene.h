#pragma once

#include <furi.h>

#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>

#include "../usb/usb_toypad.h"

typedef struct LDToyPadSceneEmulate LDToyPadSceneEmulate;

// LDToyPadEmulateView* usb_hid_dirpad_alloc(); // This is the original old function name from: https://github.com/huuck/FlipperZeroUSBKeyboard/blob/main/views/usb_hid_dirpad.h
LDToyPadSceneEmulate* ldtoypad_scene_emulate_alloc();

void ldtoypad_scene_emulate_free(LDToyPadSceneEmulate* ldtoypad_emulate);

View* ldtoypad_scene_emulate_get_view(LDToyPadSceneEmulate* ldtoypad_scene_emulate);

// View* ldtoypad_emulate_get_view(LDToyPadEmulateView* ldtoypad_emulate);

// void ldtoypad_emulate_set_connected_status(LDToyPadEmulateView* ldtoypad_emulate, bool connected);

typedef struct {
    bool left_pressed;
    bool up_pressed;
    bool right_pressed;
    bool down_pressed;
    bool ok_pressed;
    bool back_pressed;
    bool connected;
    char* connection_status;
    bool minifigure_submenu;

    uint32_t selected_minifigure_index;

    // uint8_t selectedBox = 0;

    usbd_device* usbDevice;

    uint32_t setting_1_index; // The team color setting index
    FuriString* setting_2_name; // The name setting
    uint8_t x; // The x coordinate
} LDToyPadSceneEmulateModel;

void minifigures_submenu_callback(void* context, uint32_t index);
