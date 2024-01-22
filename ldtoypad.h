#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <furi.h>

#include <gui/gui.h>
#include <gui/view.h>
#include <notification/notification.h>
#include <gui/view_dispatcher.h>

// Include the used views
#include <gui/modules/submenu.h>
#include <gui/modules/dialog_ex.h>
#include "views/EmulateToyPad.h"
#include "views/Settings.h"

typedef struct {
    Gui* gui;
    // NotificationApp* notifications;
    ViewDispatcher* view_dispatcher;
    Submenu* submenu;
    DialogEx* dialog;
    LDToyPadEmulateView* ldtoypad_emulate_view;
    uint32_t view_id;
} LDToyPadApp;

typedef enum LDToyPadView {
    LDToyPadView_Submenu,
    LDToyPadView_EmulateToyPad,
    LDToyPadView_Settings,
    LDToyPadView_ExitConfirm,
    LDToyPadView_SelectionMenu,
} LDToyPadView;

#ifdef __cplusplus
}
#endif