#pragma once

#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>

typedef struct LDToyPadEmulateView LDToyPadEmulateView;

// LDToyPadEmulateView* usb_hid_dirpad_alloc(); // This is the original old function name from: https://github.com/huuck/FlipperZeroUSBKeyboard/blob/main/views/usb_hid_dirpad.h
LDToyPadEmulateView* ldtoypad_emulate_alloc(ViewDispatcher* view_dispatcher);

void ldtoypad_emulate_free(LDToyPadEmulateView* ldtoypad_emulate);

View* ldtoypad_emulate_get_view(LDToyPadEmulateView* ldtoypad_emulate);

// void ldtoypad_emulate_set_connected_status(LDToyPadEmulateView* ldtoypad_emulate, bool connected);

typedef struct {
    int id;
    const char* name;
    // const char* world;
    // const char* abilities;
} Minifigure;