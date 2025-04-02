#include "EmulateToyPad_scene.h"

#include "../ldtoypad.h"
#include "sub_screens.h"

#include <furi.h>
#include <furi_hal.h>

/* generated by fbt from .png files in images folder */
#include <ldtoypad_icons.h>

#include <gui/elements.h>
// #include <gui/icon_i.h> // Not yet needed apparently

#include "dolphin/dolphin.h"

#include "minifigures.h"

#define numBoxes 7 // the number of boxes (7 boxes always)

LDToyPadApp* app;

LDToyPadSceneEmulate* toypadscene_instance;

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

struct BoxInfo {
    int x; // X-coordinate
    int y; // Y-coordinate
    bool isFilled; // Indicates if the box is filled with a Token (minifig / vehicle)
    int index; // The index of the token in the box
};

struct BoxInfo boxInfo[] = {
    {18, 26, false, -1}, // Selection 0 (box)
    {50, 20, false, -1}, // Selection 1 (circle)
    {85, 27, false, -1}, // Selection 2 (box)
    {16, 40, false, -1}, // Selection 3 (box)
    {35, 41, false, -1}, // Selection 4 (box)
    {70, 41, false, -1}, // Selection 5 (box)
    {86, 40, false, -1} // Selection 6 (box)
};

struct LDToyPadSceneEmulate {
    View* view;
    // LDToyPadSceneEmulateCallback callback;
    // void* context;

    // timer
    FuriTimer* timer; // Timer for redrawing the screen
};

// The selected pad on the toypad
uint8_t selectedBox = 1; // Variable to keep track of which toypad box is selected

bool ldtoypad_scene_emulate_input_callback(InputEvent* event, void* context) {
    LDToyPadSceneEmulate* instance = context;
    furi_assert(instance);

    bool consumed = false;

    with_view_model(
        instance->view,
        LDToyPadSceneEmulateModel * model,
        {
            if(model->show_screen_minfig_vehicle) {
                if(event->type == InputTypePress) {
                    if(event->key == InputKeyLeft || event->key == InputKeyRight) {
                        model->screen_minfig_vehicle_minfig_box_selected =
                            !model->screen_minfig_vehicle_minfig_box_selected;
                    }
                    // when ok is pressed and minifigure box is selected, we want to switch to the minifigure submenu else show vehicle screen
                    if(event->key == InputKeyOk) {
                        if(model->screen_minfig_vehicle_minfig_box_selected) {
                            view_dispatcher_switch_to_view(
                                app->view_dispatcher, ViewMinifigureSelection);
                        } else {
                            // model->show_screen_minfig_vehicle = false;
                            view_dispatcher_switch_to_view(
                                app->view_dispatcher, ViewVehicleSelection);
                        }
                    }
                }
            } else {
                // when the OK button is pressed, we want to switch to the minifigure selection screen for the selected box
                if(event->key == InputKeyOk) {
                    if(event->type == InputTypePress) {
                        model->ok_pressed = true;

                        // if the current selected box is not filled, we want to switch to the minifigure selection screen
                        if(!boxInfo[selectedBox].isFilled && model->connected) {
                            // set current view to minifigure / vehicle selection screen
                            model->show_screen_minfig_vehicle = true;

                            if(model->minifig_only_mode) {
                                view_dispatcher_switch_to_view(
                                    app->view_dispatcher, ViewMinifigureSelection);
                            }

                        } else if(boxInfo[selectedBox].isFilled) {
                            // if the box is filled, we want to remove the minifigure from the selected box
                            int i = boxInfo[selectedBox].index;
                            if(i >= 0 && ToyPadEmu_remove(i, selectedBox)) {
                                boxInfo[selectedBox].isFilled = false;
                                boxInfo[selectedBox].index = -1; // Reset index
                                set_debug_text("Removed minifigure from toypad");
                                consumed = true;
                            }
                            return consumed;
                        }
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
            }
        },
        true);

    return consumed;
}

unsigned char generate_checksum_for_command(const unsigned char* command, size_t len) {
    // Assert that the length of the command is less than or equal to 31
    // assert(len <= 31);

    unsigned char result = 0;

    // Add bytes, wrapping naturally with unsigned char overflow
    for(size_t i = 0; i < len; ++i) {
        result += command[i];
    }

    return result;
}

void selectedBox_to_pad(Token* new_character, int selectedBox) {
    // Convert / map the boxes to pads there are 3 pads and 7 boxes
    // TODO: This needs to be looked at, as I don't know the correct order yet
    switch(selectedBox) {
    case 0:
        new_character->pad = 2;
        break;
    case 1:
        new_character->pad = 1; // Circle
        break;
    case 2:
        new_character->pad = 3;
        break;
    case 3:
        new_character->pad = 2;
        break;
    case 4:
        new_character->pad = 2;
        break;
    case 5:
        new_character->pad = 3;
        break;
    case 6:
        new_character->pad = 3;
        break;
    default:
        furi_crash("Selected pad is invalid"); // It should never reach this.
        break;
    }
}

static void ldtoypad_scene_emulate_draw_render_callback(Canvas* canvas, void* context) {
    // UNUSED(context);
    LDToyPadSceneEmulateModel* model = context;

    if(model->show_screen_minfig_vehicle) {
        draw_minifigure_vehicle_screen(canvas, model->screen_minfig_vehicle_minfig_box_selected);
        return;
    }

    // when the usb device is not set in modek, set it
    if(model->usbDevice == NULL) {
        model->usbDevice = get_usb_device();
        model->connection_status = "USB device setting...";
    }

    if(get_connected_status() == 2) {
        model->connected = true;
        set_connected_status(
            1); // Set the connected status to 1 (connected) and not 2 (re-connecting)
        model->connection_status = "USB Awoken";
        model->screen_minfig_vehicle_minfig_box_selected =
            true; // Set the minifigure box selected as this is the most commonnly used at start of the app.

        // reset the filled boxes
        for(int i = 0; i < numBoxes; i++) {
            boxInfo[i].isFilled = false;
        }

        // Give dolphin some xp for connecting the toypad
        dolphin_deed(DolphinDeedPluginStart);

        if(toypadscene_instance->timer != NULL) {
            furi_timer_stop(toypadscene_instance->timer);
            furi_timer_start(toypadscene_instance->timer, furi_ms_to_ticks(5000));
        }
    } else if(model->connected) {
        model->connection_status = "USB Connected";
    } else if(model->usbDevice == NULL) {
        model->connection_status = "USB not yet connected";
    } else if(!model->connected) {
        model->connection_status = "Trying to connect USB";
    }

    if(model->selected_minifigure_index > 0 && model->connected) {
        int id = (int)model->selected_minifigure_index;
        model->selected_minifigure_index = 0;

        unsigned char buffer[32] = {0};
        if(id < 1) id = 1;

        Token* character = createCharacter(id);
        boxInfo[selectedBox].isFilled = true;
        selectedBox_to_pad(character, selectedBox);

        // Find an empty slot or use the next available index
        int new_index = -1;
        for(int i = 0; i < MAX_TOKENS; i++) {
            if(emulator->tokens[i] == NULL) {
                new_index = i;
                break;
            }
        }
        if(new_index == -1 && emulator->token_count < MAX_TOKENS) {
            new_index = emulator->token_count++;
        } else if(new_index == -1) {
            set_debug_text("Max tokens reached!");
            free(character);
            return; // No space available
        }

        character->index = new_index;
        emulator->tokens[new_index] = character;
        boxInfo[selectedBox].index = new_index;

        // Send placement command
        buffer[0] = 0x56;
        buffer[1] = 0x0b;
        buffer[2] = character->pad;
        buffer[3] = 0x00;
        buffer[4] = character->index;
        buffer[5] = 0x00; // Tag placed
        memcpy(&buffer[6], character->uid, 7);
        buffer[13] = generate_checksum_for_command(buffer, 13);

        usbd_ep_write(model->usbDevice, HID_EP_IN, buffer, sizeof(buffer));

        dolphin_deed(DolphinDeedNfcReadSuccess);
    } else if(model->selected_vehicle_index > 0 && model->connected) {
        int id = (int)model->selected_vehicle_index;
        model->selected_vehicle_index = 0;
        set_debug_text("Render vehicle");
        unsigned char buffer[32];
        memset(buffer, 0, sizeof(buffer));
        if(id < 1000) {
            id = 1000;
        }
        uint32_t upgrades[2] = {0, 0};
        Token* vehicle = createVehicle(id, upgrades); // Use `id` instead of 1030
        boxInfo[selectedBox].isFilled = true;
        selectedBox_to_pad(vehicle, selectedBox);
        vehicle->index = emulator->token_count;
        emulator->tokens[vehicle->index] = vehicle;
        emulator->token_count++;
        boxInfo[selectedBox].index = vehicle->index;
        buffer[0] = 0x56;
        buffer[1] = 0x0b;
        buffer[2] = vehicle->pad;
        buffer[3] = 0x00;
        buffer[4] = vehicle->index;
        buffer[5] = 0x00;
        memcpy(&buffer[6], vehicle->uid, 7);
        buffer[13] = generate_checksum_for_command(buffer, 13);
        usbd_ep_write(model->usbDevice, HID_EP_IN, buffer, sizeof(buffer));
    }

    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);

    // Draw the big toypad image as decoration for the background of the screen
    canvas_draw_icon(canvas, 10, 13, &I_toypad);

    // Get position for the selected box
    uint8_t x = boxInfo[selectedBox].x;
    uint8_t y = boxInfo[selectedBox].y;
    // Check if the selectedBox is 1 (circle) and draw the circle, This is hardcoded for now.
    if(selectedBox == 1) {
        canvas_draw_xbm(canvas, x, y, 22, 17, I_selectionCircle); // Draw highlighted circle
    } else {
        canvas_draw_xbm(canvas, x, y, 18, 18, I_selectionBox); // Draw highlighted box
    }

    // when the box is filled, draw the minifigure icon
    for(int i = 0; i < numBoxes; i++) {
        if(boxInfo[i].isFilled) {
            Token* character = emulator->tokens[boxInfo[i].index];
            if(model->show_icons_index) {
                // Draw the minifigure icon
                canvas_draw_icon(canvas, boxInfo[i].x + 4, boxInfo[i].y + 3, &I_head);
            } else {
                // Draw the first letter of the minifigure name

                // get the first letter of the minifigure name
                char letter[1];
                letter[0] = character->name[0];

                canvas_draw_str(canvas, boxInfo[i].x + 6, boxInfo[i].y + 12, letter);
            }

            // Set the connection status text to the currently connected minifigure name
            if(selectedBox == i) {
                model->connection_status = character->name;
            }
        }
    }

    elements_multiline_text_aligned(canvas, 1, 1, AlignLeft, AlignTop, model->connection_status);

    if(model->show_debug_text_index) {
        // elements_button_left(canvas, "Prev");
        // elements_button_center(canvas, "OK");
        // elements_button_right(canvas, "Next");

        if(get_debug_text_ep_in() != NULL && strcmp(get_debug_text_ep_in(), "nothing") != 0) {
            canvas_set_color(canvas, ColorWhite);
            canvas_clear(canvas);
            canvas_set_color(canvas, ColorBlack);

            elements_multiline_text_aligned(
                canvas, 1, 1, AlignLeft, AlignTop, get_debug_text_ep_in());
        }

        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, 0, 16, 120, 20);
        canvas_set_color(canvas, ColorBlack);

        elements_multiline_text_aligned(canvas, 1, 17, AlignLeft, AlignTop, "Debug: ");
        elements_multiline_text_aligned(canvas, 40, 17, AlignLeft, AlignTop, get_debug_text());
    }

    if(!model->connected && model->ok_pressed) {
        // when boxes at the right side of the screen are pressed, show message at the left side
        if(selectedBox == 2 || selectedBox == 6) {
            x -= 75;
            y += 10;
        } else if(selectedBox == 1) {
            x -= 30;
            y += 22;
        } else {
            x += 20;
            y += 10;
        }

        elements_multiline_text_framed(canvas, x, y, "Connect Game");
    }
}

static uint32_t ldtoypad_scene_emulate_navigation_submenu_callback(void* context) {
    UNUSED(context);

    with_view_model(
        ldtoypad_scene_emulate_get_view(toypadscene_instance),
        LDToyPadSceneEmulateModel * model,
        {
            if(model->show_screen_minfig_vehicle) {
                model->show_screen_minfig_vehicle = false;
                return ViewEmulate;
            }
        },
        false);

    return ViewSubmenu;
}

void ldtoypad_scene_emulate_view_game_timer_callback(void* context) {
    UNUSED(context);
    // LDToyPadSceneEmulate* app = (LDToyPadSceneEmulate*)context;
    view_dispatcher_send_custom_event(get_view_dispatcher(), 0);
}

void ldtoypad_scene_emulate_enter_callback(void* context) {
    uint32_t period = furi_ms_to_ticks(200); // 5 seconds
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

static bool ldtoypad_scene_emulate_custom_event_callback(uint32_t event, void* context) {
    LDToyPadSceneEmulate* scene = (LDToyPadSceneEmulate*)context;
    switch(event) {
    case 0:
        // Redraw screen by passing true to last parameter of with_view_model.
        {
            bool redraw = true;
            with_view_model(
                ldtoypad_scene_emulate_get_view(scene),
                LDToyPadSceneEmulateModel * _model,
                { UNUSED(_model); },
                redraw);
            return true;
        }
    default:
        return false;
    }
}

LDToyPadSceneEmulate* ldtoypad_scene_emulate_alloc(LDToyPadApp* new_app) {
    furi_assert(new_app);
    app = new_app;

    if(emulator == NULL) emulator = malloc(sizeof(ToyPadEmu));
    emulator->token_count = 0;
    memset(emulator->tokens, 0, sizeof(emulator->tokens));

    LDToyPadSceneEmulate* instance = malloc(sizeof(LDToyPadSceneEmulate));
    instance->view = view_alloc();

    toypadscene_instance = instance;

    // ldtoypad_view_dispatcher = view_dispatcher;

    usb_mode_prev = furi_hal_usb_get_config();

    furi_hal_usb_unlock();
    furi_check(furi_hal_usb_set_config(&usb_hid_ldtoypad, NULL) == true);

    view_set_context(instance->view, instance);
    view_allocate_model(instance->view, ViewModelTypeLockFree, sizeof(LDToyPadSceneEmulateModel));
    // view_set_draw_callback(instance->view, ldtoypad_scene_emulate_draw_callback);
    view_set_draw_callback(instance->view, ldtoypad_scene_emulate_draw_render_callback);
    view_set_input_callback(instance->view, ldtoypad_scene_emulate_input_callback);

    view_set_previous_callback(instance->view, ldtoypad_scene_emulate_navigation_submenu_callback);

    view_set_enter_callback(instance->view, ldtoypad_scene_emulate_enter_callback);
    view_set_exit_callback(instance->view, ldtoypad_scene_emulate_exit_callback);

    view_set_custom_callback(instance->view, ldtoypad_scene_emulate_custom_event_callback);

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

    // free all the tokens ( needs a better solution later )
    for(int i = 0; i < 8; i++) {
        if(emulator->tokens[i] != NULL) {
            free(emulator->tokens[i]);
        }
    }
    free(emulator);
}

View* ldtoypad_scene_emulate_get_view(LDToyPadSceneEmulate* instance) {
    furi_assert(instance);
    return instance->view;
}

void minifigures_submenu_callback(void* context, uint32_t index) {
    LDToyPadApp* app = (LDToyPadApp*)context;

    // set current view to minifigure number to the selected index
    with_view_model(
        ldtoypad_scene_emulate_get_view(app->view_scene_emulate),
        LDToyPadSceneEmulateModel * model,
        {
            model->selected_vehicle_index = 0;
            if(model->connected) {
                model->selected_minifigure_index = index;
            }
            model->show_screen_minfig_vehicle = false;
        },
        true);

    view_dispatcher_switch_to_view(app->view_dispatcher, ViewEmulate);
}

void vehicles_submenu_callback(void* context, uint32_t index) {
    LDToyPadApp* app = (LDToyPadApp*)context;

    // set current view to minifigure number to the selected index
    with_view_model(
        ldtoypad_scene_emulate_get_view(app->view_scene_emulate),
        LDToyPadSceneEmulateModel * model,
        {
            model->selected_minifigure_index = 0;
            if(model->connected) {
                model->selected_vehicle_index = index;
            }
            model->show_screen_minfig_vehicle = false;
        },
        true);

    view_dispatcher_switch_to_view(app->view_dispatcher, ViewEmulate);
}
