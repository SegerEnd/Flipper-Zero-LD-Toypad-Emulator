#pragma once

#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>

typedef struct LDToyPadSceneEmulate LDToyPadSceneEmulate;

// LDToyPadEmulateView* usb_hid_dirpad_alloc(); // This is the original old function name from: https://github.com/huuck/FlipperZeroUSBKeyboard/blob/main/views/usb_hid_dirpad.h
LDToyPadSceneEmulate* ldtoypad_scene_emulate_alloc();

void ldtoypad_scene_emulate_free(LDToyPadSceneEmulate* ldtoypad_emulate);

View* ldtoypad_scene_emulate_get_view(LDToyPadSceneEmulate* ldtoypad_scene_emulate);

// View* ldtoypad_emulate_get_view(LDToyPadEmulateView* ldtoypad_emulate);

// void ldtoypad_emulate_set_connected_status(LDToyPadEmulateView* ldtoypad_emulate, bool connected);

typedef struct {
    int id;
    const char* name;
    // const char* world;
    // const char* abilities;
} Minifigure;

typedef struct {
    bool left_pressed;
    bool up_pressed;
    bool right_pressed;
    bool down_pressed;
    bool ok_pressed;
    bool back_pressed;
    bool connected;

    // uint8_t selectedBox = 0;

    uint32_t setting_1_index; // The team color setting index
    FuriString* setting_2_name; // The name setting
    uint8_t x; // The x coordinate
} LDToyPadSceneEmulateModel;
