#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <furi.h>
#include <furi_hal.h>

#include <gui/gui.h>
#include <gui/view.h>

#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/widget.h>

// #include <notification/notification.h>
// #include <notification/notification_messages.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/text_input.h>
#include <gui/modules/variable_item_list.h>

// Include the used views
// #include <gui/modules/submenu.h>
// #include <gui/modules/dialog_ex.h>
// #include "views/EmulateToyPad.h"
// #include "views/Settings.h"

#include "views/EmulateToyPad_scene.h"

typedef struct {
    ViewDispatcher* view_dispatcher; // Switches between our views
    // NotificationApp* notifications; // Used for controlling the backlight
    Submenu* submenu; // The application menu
    TextInput* text_input; // The text input screen
    VariableItemList* variable_item_list_config; // The configuration screen

    // View* view_game; // The main screen
    LDToyPadSceneEmulate* view_scene_emulate; // The emulator screen

    Widget* widget_about; // The about screen

    VariableItem* setting_2_item; // The name setting item (so we can update the text)
    char* temp_buffer; // Temporary buffer for text input
    uint32_t temp_buffer_size; // Size of temporary buffer

    FuriTimer* timer; // Timer for redrawing the screen
} LDToyPadApp;

// Each view is a screen we show the user.
typedef enum {
    ViewSubmenu, // The menu when the app starts
    ViewTextInput, // Input for configuring text settings
    ViewConfigure, // The configuration screen
    ViewEmulate, // The main screen
    ViewAbout, // The about screen with directions, link to social channel, etc.
} Views;

#ifdef __cplusplus
}
#endif
