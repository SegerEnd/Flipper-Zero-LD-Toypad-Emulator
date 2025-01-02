#include "EmulateToyPad_scene.h"

#include "../ldtoypad.h"

#include <furi.h>
#include <furi_hal.h>

/* generated by fbt from .png files in images folder */
#include <ldtoypad_icons.h>

#include <gui/elements.h>
// #include <gui/icon_i.h> // Not yet needed apparently

#define numBoxes 7 // the number of boxes (7 boxes always)

LDToyPadApp* app;

FuriHalUsbInterface* usb_mode_prev = NULL;

// Selection box icon
uint8_t I_selectionBox[] = {0xf8, 0xff, 0x00, 0x06, 0x00, 0x01, 0x03, 0x00, 0x02, 0x03, 0x00,
                            0x02, 0x03, 0x00, 0x02, 0x03, 0x00, 0x02, 0x03, 0x00, 0x02, 0x03,
                            0x00, 0x02, 0x03, 0x00, 0x02, 0x03, 0x00, 0x02, 0x03, 0x00, 0x02,
                            0x03, 0x00, 0x02, 0x03, 0x00, 0x02, 0x03, 0x00, 0x02, 0x03, 0x00,
                            0x02, 0x07, 0x00, 0x03, 0xfe, 0xff, 0x01, 0xfc, 0xff, 0x00};

//  Selection circle icon
uint8_t I_selectionCircle[] = {0x80, 0x7f, 0x00, 0xf0, 0xff, 0x03, 0xf8, 0xc0, 0x07, 0x3c, 0x00,
                               0x0f, 0x0c, 0x00, 0x0c, 0x06, 0x00, 0x18, 0x07, 0x00, 0x38, 0x07,
                               0x00, 0x38, 0x03, 0x00, 0x30, 0x03, 0x00, 0x30, 0x07, 0x00, 0x38,
                               0x06, 0x00, 0x18, 0x06, 0x00, 0x18, 0x3e, 0x00, 0x1f, 0xf8, 0xc0,
                               0x07, 0xf0, 0xff, 0x03, 0x80, 0x7f, 0x00};

// Define box information (coordinates and dimensions) for each box (7 boxes total)
int boxInfo[numBoxes][2] = {
    {18, 26}, // Selection 0 (box)
    {50, 20}, // Selection 1 (circle)
    {85, 27}, // Selection 2 (box)
    {16, 40}, // Selection 3 (box)
    {35, 41}, // Selection 4 (box)
    {70, 41}, // Selection 5 (box)
    {86, 40} // Selection 6 (box)
};

struct LDToyPadSceneEmulate {
    View* view;
    // LDToyPadSceneEmulateCallback callback;
    // void* context;

    // timer
    FuriTimer* timer; // Timer for redrawing the screen
};

// The selected pad on the toypad
uint8_t selectedBox = 0; // Variable to keep track of which toypad box is selected

// Submenu* selectionMenu; // The submenu to select minifigures and vehicles for each selection box

// ViewDispatcher* ldtoypad_view_dispatcher;

// void selectionMenu_callback(void* context, uint32_t index) {
//     UNUSED(context);
//     UNUSED(index);

//     view_dispatcher_switch_to_view(ldtoypad_view_dispatcher, LDToyPadView_EmulateToyPad);
// }

void send_minifigure(uint32_t minifigure_index) {
    // create a minifigure from the selected minifigure
    char uid[6];
    ToyPadEmu_randomUID(uid);

    set_debug_text(uid);

    // place the minifigure on the selected box
    ToyPadEmu_place(get_emulator(), selectedBox, minifigure_index, uid);
}

bool ldtoypad_scene_emulate_input_callback(InputEvent* event, void* context) {
    LDToyPadSceneEmulate* instance = context;
    furi_assert(instance);

    bool consumed = false;

    with_view_model(
        instance->view,
        LDToyPadSceneEmulateModel * model,
        {
            if(model->selected_minifigure_index != 0) {
                send_minifigure(model->selected_minifigure_index);
                model->selected_minifigure_index = 0;
            }

            // when the OK button is pressed, we want to switch to the minifigure selection screen for the selected box
            if(event->key == InputKeyOk) {
                if(event->type == InputTypePress) {
                    model->ok_pressed = true;

                    // set current view to minifigure selection screen
                    view_dispatcher_switch_to_view(app->view_dispatcher, ViewMinifigureSelection);

                    // submenu_reset(selectionMenu);

                    // for(int i = 0; minifigures[i].name != NULL; i++) {
                    //     submenu_add_item(
                    //         selectionMenu,
                    //         minifigures[i].name,
                    //         minifigures[i].id,
                    //         selectionMenu_callback,
                    //         minifigures);
                    // }
                    // view_dispatcher_switch_to_view(
                    //     ldtoypad_view_dispatcher, LDToyPadView_SelectionMenu);

                    consumed = true;
                    return consumed;

                } else if(event->type == InputTypeRelease) {
                    model->ok_pressed = false;
                }
            }
            // make user loop through boxes with InputKeyLeft, InputKeyRight, InputKeyUp, InputKeyDown
            if(event->key == InputKeyLeft) {
                if(event->type == InputTypePress) {
                    model->left_pressed = true;
                    if(selectedBox == 0) {
                        selectedBox = numBoxes;
                    }
                    selectedBox--;
                } else if(event->type == InputTypeRelease) {
                    model->left_pressed = false;
                }
            }
            if(event->key == InputKeyRight) {
                if(event->type == InputTypePress) {
                    model->right_pressed = true;
                    selectedBox++;
                    if(selectedBox >= numBoxes) {
                        selectedBox = 0;
                    }
                } else if(event->type == InputTypeRelease) {
                    model->right_pressed = false;
                }
            }
            if(event->key == InputKeyUp) {
                if(event->type == InputTypePress) {
                    model->up_pressed = true;
                    if(selectedBox == 0) {
                        selectedBox = 3;
                    } else if(selectedBox >= 4) {
                        selectedBox -= 4;
                    } else {
                        selectedBox = (numBoxes - 3) + selectedBox;
                    }
                    if(selectedBox >= numBoxes) {
                        selectedBox = 0;
                    }
                } else if(event->type == InputTypeRelease) {
                    model->up_pressed = false;
                }
            }

            if(event->key == InputKeyDown) {
                if(event->type == InputTypePress) {
                    model->down_pressed = true;
                    if(selectedBox == 2) {
                        selectedBox = 6;
                    } else if(selectedBox == 3) {
                        selectedBox = 0;
                    } else if(selectedBox == 5) {
                        selectedBox = 2;
                    } else if(selectedBox < (numBoxes - 3)) {
                        selectedBox += 3;
                    } else {
                        selectedBox = selectedBox - (numBoxes - 3);
                    }
                } else if(event->type == InputTypeRelease) {
                    model->down_pressed = false;
                }
            }
        },
        true);

    return consumed;
}

void ldtoypad_scene_emulate_draw_callback(Canvas* canvas, void* _model) {
    // furi_assert(_model);
    LDToyPadSceneEmulateModel* model = _model;

    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    canvas_draw_str(canvas, 2, 7, "Emulate ToyPad");

    // // Draw the toypad layout on the background
    // canvas_draw_icon(canvas, 10, 13, &I_toypad);

    // // Get position for the selected box
    // uint8_t x = boxInfo[selectedBox][0];
    // uint8_t y = boxInfo[selectedBox][1];
    // // Check if the selectedBox is 1 (circle) and draw the circle, This is hardcoded for now.
    // if(selectedBox == 1) {
    //     canvas_draw_xbm(canvas, x, y, 22, 17, I_selectionCircle); // Draw highlighted circle
    // } else {
    //     canvas_draw_xbm(canvas, x, y, 18, 18, I_selectionBox); // Draw highlighted box
    // }

    // canvas_set_font(canvas, FontPrimary);

    // // Displaying the connected USB status
    if(model->connected) {
        // elements_multiline_text_aligned(canvas, 1, 1, AlignLeft, AlignTop, "USB Connected");
        elements_multiline_text_aligned(canvas, 1, 1, AlignLeft, AlignTop, "Awaiting");
    } else {
        elements_multiline_text_aligned(canvas, 1, 1, AlignLeft, AlignTop, "USB Not Connected");
        // if(furi_hal_usb_get_config() == &usb_hid_ldtoypad) {
        //     model->connected = true;
        // }
    }

    // // Testing pressing buttons
    // // if(model->ok_pressed) {
    // //     canvas_set_color(canvas, ColorWhite);
    // //     canvas_draw_box(canvas, 43, 28, 64, 16);
    // //     canvas_set_color(canvas, ColorBlack);
    // //     elements_multiline_text_aligned(canvas, 45, 30, AlignLeft, AlignTop, "OK pressed");
    // // }

    // // Draw a box behind the text hold to exit
    // canvas_set_color(canvas, ColorWhite);
    // canvas_draw_box(canvas, 78, 53, 75, 16);
    // canvas_set_color(canvas, ColorBlack);
    // canvas_set_font(canvas, FontSecondary);
    // elements_multiline_text_aligned(canvas, 80, 55, AlignLeft, AlignTop, "Hold to exit");
    // canvas_set_color(canvas, ColorWhite);
    // canvas_draw_box(canvas, 0, 16, 120, 16);
    // canvas_set_color(canvas, ColorBlack);
    // canvas_set_font(canvas, FontPrimary);

    // // from get_debug_text() function display the text
    // elements_multiline_text_aligned(canvas, 1, 17, AlignLeft, AlignTop, "ep_in: ");
    // elements_multiline_text_aligned(canvas, 40, 17, AlignLeft, AlignTop, get_debug_text_ep_in());

    // canvas_set_color(canvas, ColorWhite);
    // canvas_draw_box(canvas, 0, 32, 120, 16);
    // canvas_set_color(canvas, ColorBlack);
    // canvas_set_font(canvas, FontPrimary);

    // // say ep_out before the text
    // elements_multiline_text_aligned(canvas, 1, 33, AlignLeft, AlignTop, "ep_out: ");
    // elements_multiline_text_aligned(canvas, 40, 33, AlignLeft, AlignTop, get_debug_text_ep_out());
    // // Debugging text for watching the USB endpoints

    // // Now for USB info also but below the other one
}

// void ldtoypad_scene_emulate_enter_callback(void* context) {
//     UNUSED(context);
// }

// void ldtoypad_scene_emulate_exit_callback(void* context) {
//     UNUSED(context);
// }

// uint32_t selectionMenu_prev_callback(void* context) {
//     UNUSED(context);
//     return LDToyPadView_EmulateToyPad;
// }

static void ldtoypad_scene_emulate_draw_render_callback(Canvas* canvas, void* context) {
    // UNUSED(context);
    LDToyPadSceneEmulateModel* model = context;

    // when the usb device is not set in modek, set it
    if(model->usbDevice == NULL) {
        model->usbDevice = get_usb_device();
        model->connection_status = "USB device setting...";
    }
    if(model->usbDevice == NULL) {
        model->connection_status = "USB not yet connected";
    } else if(!model->connected) {
        model->connection_status = "Trying to connect USB";
    }

    // poll the USB status here
    // usbd_poll(model->usbDevice);

    canvas_clear(canvas);
    canvas_set_font(canvas, FontSecondary);

    // canvas_draw_str_aligned(canvas, (128 - 40), 5, AlignCenter, AlignTop, "Select here");
    // canvas_draw_str(canvas, 2, 7, "Emulate ToyPad");

    canvas_draw_icon(canvas, 10, 13, &I_toypad);

    // Get position for the selected box
    uint8_t x = boxInfo[selectedBox][0];
    uint8_t y = boxInfo[selectedBox][1];
    // Check if the selectedBox is 1 (circle) and draw the circle, This is hardcoded for now.
    if(selectedBox == 1) {
        canvas_draw_xbm(canvas, x, y, 22, 17, I_selectionCircle); // Draw highlighted circle
    } else {
        canvas_draw_xbm(canvas, x, y, 18, 18, I_selectionBox); // Draw highlighted box
    }

    canvas_set_font(canvas, FontPrimary);
    // elements_button_left(canvas, "Prev");
    // elements_button_center(canvas, "OK");
    // elements_button_right(canvas, "Next");

    if(model->connected) {
        // elements_multiline_text_aligned(canvas, 1, 1, AlignLeft, AlignTop, "USB Connected");
        elements_multiline_text_aligned(canvas, 1, 1, AlignLeft, AlignTop, "Awaiting");
    } else {
        elements_multiline_text_aligned(
            canvas, 1, 1, AlignLeft, AlignTop, model->connection_status);
        // if(furi_hal_usb_get_config() == &usb_hid_ldtoypad) {
        //     model->connected = true;
        // }
    }

    // Testing pressing buttons
    if(model->ok_pressed) {
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, 43, 28, 64, 16);
        canvas_set_color(canvas, ColorBlack);
        elements_multiline_text_aligned(canvas, 45, 30, AlignLeft, AlignTop, "OK pressed");
    }

    // canvas_set_color(canvas, ColorWhite);
    // canvas_draw_box(canvas, 0, 16, 120, 16);
    // canvas_set_color(canvas, ColorBlack);

    // elements_multiline_text_aligned(canvas, 1, 17, AlignLeft, AlignTop, "ep_in: ");
    // elements_multiline_text_aligned(canvas, 40, 17, AlignLeft, AlignTop, get_debug_text_ep_in());

    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 16, 120, 16);
    canvas_set_color(canvas, ColorBlack);

    elements_multiline_text_aligned(canvas, 1, 17, AlignLeft, AlignTop, "Debug: ");
    elements_multiline_text_aligned(canvas, 40, 17, AlignLeft, AlignTop, get_debug_text());

    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 32, 120, 16);
    canvas_set_color(canvas, ColorBlack);

    elements_multiline_text_aligned(canvas, 1, 33, AlignLeft, AlignTop, "ep_out: ");
    elements_multiline_text_aligned(canvas, 40, 33, AlignLeft, AlignTop, get_debug_text_ep_out());

    // ep in
    // canvas_set_color(canvas, ColorWhite);
    // canvas_draw_box(canvas, 0, 48, 120, 16);
    // canvas_set_color(canvas, ColorBlack);

    // elements_multiline_text_aligned(canvas, 1, 49, AlignLeft, AlignTop, "ep_in: ");
    // elements_multiline_text_aligned(canvas, 40, 49, AlignLeft, AlignTop, get_debug_text_ep_in());
}

static uint32_t ldtoypad_scene_emulate_navigation_submenu_callback(void* context) {
    UNUSED(context);

    // if(usb_mode_prev != NULL) {
    //     furi_hal_usb_set_config(usb_mode_prev, NULL);
    //     free(usb_mode_prev);
    // }

    return ViewSubmenu;
}

void ldtoypad_scene_emulate_view_game_timer_callback(void* context) {
    UNUSED(context);
    // LDToyPadSceneEmulate* app = (LDToyPadSceneEmulate*)context;
    view_dispatcher_send_custom_event(get_view_dispatcher(), 0);
}

void ldtoypad_scene_emulate_enter_callback(void* context) {
    uint32_t period = furi_ms_to_ticks(150);
    LDToyPadSceneEmulate* app = (LDToyPadSceneEmulate*)context;
    furi_assert(app->timer == NULL);
    app->timer = furi_timer_alloc(
        ldtoypad_scene_emulate_view_game_timer_callback, FuriTimerTypePeriodic, app);
    furi_timer_start(app->timer, period);
}

void ldtoypad_scene_emulate_exit_callback(void* context) {
    LDToyPadSceneEmulate* app = (LDToyPadSceneEmulate*)context;
    furi_timer_stop(app->timer);
    furi_timer_free(app->timer);
    app->timer = NULL;
}

LDToyPadSceneEmulate* ldtoypad_scene_emulate_alloc(LDToyPadApp* new_app) {
    furi_assert(new_app);
    app = new_app;

    LDToyPadSceneEmulate* instance = malloc(sizeof(LDToyPadSceneEmulate));
    instance->view = view_alloc();

    // ldtoypad_view_dispatcher = view_dispatcher;

    usb_mode_prev = furi_hal_usb_get_config();

    furi_hal_usb_unlock();
    furi_check(furi_hal_usb_set_config(&usb_hid_ldtoypad, NULL) == true);

    view_set_context(instance->view, instance);
    view_allocate_model(instance->view, ViewModelTypeLockFree, sizeof(LDToyPadSceneEmulateModel));
    // view_set_draw_callback(instance->view, ldtoypad_scene_emulate_draw_callback);
    view_set_draw_callback(instance->view, ldtoypad_scene_emulate_draw_render_callback);
    view_set_input_callback(instance->view, ldtoypad_scene_emulate_input_callback);
    // view_set_enter_callback(instance->view, ldtoypad_scene_emulate_enter_callback);
    // view_set_exit_callback(instance->view, ldtoypad_scene_emulate_exit_callback);
    view_set_previous_callback(instance->view, ldtoypad_scene_emulate_navigation_submenu_callback);

    view_set_enter_callback(instance->view, ldtoypad_scene_emulate_enter_callback);

    view_set_exit_callback(instance->view, ldtoypad_scene_emulate_exit_callback);

    // Allocate the submenu
    // selectionMenu = submenu_alloc();
    // view_set_previous_callback(submenu_get_view(selectionMenu), selectionMenu_prev_callback);
    // view_dispatcher_add_view(
    //     ldtoypad_view_dispatcher, LDToyPadView_SelectionMenu, submenu_get_view(selectionMenu));

    // Items for the submenu as characters and vehicles

    return instance;
}

void ldtoypad_scene_emulate_free(LDToyPadSceneEmulate* ldtoypad_emulate_view) {
    furi_assert(ldtoypad_emulate_view);
    view_free(ldtoypad_emulate_view->view);
    // view_free(submenu_get_view(selectionMenu));

    // // Change back profile
    if(usb_mode_prev != NULL) {
        furi_hal_usb_set_config(usb_mode_prev, NULL);
    }
    free(usb_mode_prev);

    free(ldtoypad_emulate_view);
}

View* ldtoypad_scene_emulate_get_view(LDToyPadSceneEmulate* instance) {
    furi_assert(instance);
    return instance->view;
}

void minifigures_submenu_callback(void* context, uint32_t index) {
    LDToyPadApp* app = (LDToyPadApp*)context;

    // print index of selected minifigure as debug text
    char debug_text[10];
    // convert the long unsigned int to a string
    snprintf(debug_text, 10, "%ld", index);
    // set the debug text
    set_debug_text(debug_text);

    // set current view to minifigure number to the selected index
    with_view_model(
        ldtoypad_scene_emulate_get_view(app->view_scene_emulate),
        LDToyPadSceneEmulateModel * model,
        { model->selected_minifigure_index = index + 1; },
        true);

    view_dispatcher_switch_to_view(app->view_dispatcher, ViewEmulate);
}
