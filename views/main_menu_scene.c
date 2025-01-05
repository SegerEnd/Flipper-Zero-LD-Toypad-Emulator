#include "main_menu_scene.h"

typedef struct {
    bool settings_selected;
    bool emulate_selected;
    bool about_selected;
} MainMenu;

MainMenu* main_menu;

// draw callback
void ldtoypad_main_menu_scene_draw(Canvas* canvas, void* context) {
    UNUSED(context);

    if(main_menu == NULL) {
        main_menu = malloc(sizeof(MainMenu));
        main_menu->settings_selected = false;
        main_menu->emulate_selected = true;
        main_menu->about_selected = false;
    }

    canvas_draw_icon(canvas, -3, 0, &I_decoration);

    canvas_draw_icon(canvas, 3, 47, &I_usb);

    if(main_menu->emulate_selected) {
        canvas_draw_icon(canvas, 3, 14, &I_emulate_clicked);
    } else {
        canvas_draw_icon(canvas, 3, 14, &I_emulate);
    }

    if(main_menu->settings_selected) {
        canvas_draw_icon(canvas, 43, 38, &I_settings_clicked);
    } else {
        canvas_draw_icon(canvas, 43, 38, &I_settings);
    }

    if(main_menu->about_selected) {
        canvas_draw_icon(canvas, 83, 14, &I_about_clicked);
    } else {
        canvas_draw_icon(canvas, 83, 14, &I_about);
    }

    // canvas_draw_xbm(canvas, 3, 32, 14, 16, &I_bluetooth); // maybe later an website connection with bluetooth
}

bool ldtoypad_main_menu_scene_input_callback(InputEvent* event, void* context) {
    UNUSED(context);
    // update the MainMenu struct with the current selection, switch through the different selections

    bool consumed = false;

    if(event->key == InputKeyLeft) {
        if(main_menu->settings_selected) {
            main_menu->settings_selected = false;
            main_menu->emulate_selected = true;
        } else if(main_menu->emulate_selected) {
            main_menu->emulate_selected = false;
            main_menu->about_selected = true;
        } else if(main_menu->about_selected) {
            main_menu->about_selected = false;
            main_menu->settings_selected = true;
        }
        consumed = true;
    } else if(event->key == InputKeyRight) {
        if(main_menu->settings_selected) {
            main_menu->settings_selected = false;
            main_menu->about_selected = true;
        } else if(main_menu->about_selected) {
            main_menu->about_selected = false;
            main_menu->emulate_selected = true;
        } else if(main_menu->emulate_selected) {
            main_menu->emulate_selected = false;
            main_menu->settings_selected = true;
        }
        consumed = true;
    } else if(event->key == InputKeyOk) {
        if(main_menu->settings_selected) {
            // view_dispatcher_switch_to_view(app->view_dispatcher, ViewConfigure);
        } else if(main_menu->emulate_selected) {
            // view_dispatcher_switch_to_view(app->view_dispatcher, ViewEmulate);
        } else if(main_menu->about_selected) {
            // view_dispatcher_switch_to_view(app->view_dispatcher, ViewAbout);
        }
    }

    return consumed;
}
