#pragma once

#include <furi.h>

#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>

#include "../usb/usb_toypad.h"

typedef struct LDToyPadSceneEmulate LDToyPadSceneEmulate;

LDToyPadSceneEmulate* ldtoypad_scene_emulate_alloc();

void ldtoypad_scene_emulate_free(LDToyPadSceneEmulate* ldtoypad_emulate);

View* ldtoypad_scene_emulate_get_view(LDToyPadSceneEmulate* ldtoypad_scene_emulate);

unsigned char generate_checksum_for_command(const unsigned char* command, size_t len);

void selectedBox_to_pad(Token* new_character, int selectedBox);

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

    usbd_device* usbDevice;

    // setting indexes below
    bool show_debug_text_index;
} LDToyPadSceneEmulateModel;

void minifigures_submenu_callback(void* context, uint32_t index);
