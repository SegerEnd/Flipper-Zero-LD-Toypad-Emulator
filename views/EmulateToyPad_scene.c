#include "EmulateToyPad_scene.h"

#include "../ldtoypad.h"

#include <furi.h>
#include <furi_hal.h>

/* generated by fbt from .png files in images folder */
#include <ldtoypad_icons.h>

#include <gui/elements.h>
// #include <gui/icon_i.h> // Not yet needed apparently

#include "dolphin/dolphin.h"

#include "minifigures.h"

#include "usb/save_toypad.h"

#define numBoxes 7 // the number of boxes (7 boxes always)

#define TOKEN_DELAY_TIME 400 // delay time for the token to be placed on the pad in ms

LDToyPadApp* app;

LDToyPadSceneEmulate* toypadscene_instance;

FuriHalUsbInterface* usb_mode_prev = NULL;

// Selection box icon
const uint8_t I_selectionBox[] = {0xf8, 0xff, 0x00, 0x06, 0x00, 0x01, 0x03, 0x00, 0x02, 0x03, 0x00,
                                  0x02, 0x03, 0x00, 0x02, 0x03, 0x00, 0x02, 0x03, 0x00, 0x02, 0x03,
                                  0x00, 0x02, 0x03, 0x00, 0x02, 0x03, 0x00, 0x02, 0x03, 0x00, 0x02,
                                  0x03, 0x00, 0x02, 0x03, 0x00, 0x02, 0x03, 0x00, 0x02, 0x03, 0x00,
                                  0x02, 0x07, 0x00, 0x03, 0xfe, 0xff, 0x01, 0xfc, 0xff, 0x00};

//  Selection circle icon
const uint8_t I_selectionCircle[] = {0x80, 0x7f, 0x00, 0xf0, 0xff, 0x03, 0xf8, 0xc0, 0x07,
                                     0x3c, 0x00, 0x0f, 0x0c, 0x00, 0x0c, 0x06, 0x00, 0x18,
                                     0x07, 0x00, 0x38, 0x07, 0x00, 0x38, 0x03, 0x00, 0x30,
                                     0x03, 0x00, 0x30, 0x07, 0x00, 0x38, 0x06, 0x00, 0x18,
                                     0x06, 0x00, 0x18, 0x3e, 0x00, 0x1f, 0xf8, 0xc0, 0x07,
                                     0xf0, 0xff, 0x03, 0x80, 0x7f, 0x00};

// Define box information (coordinates and dimensions) for each box (7 boxes total)

struct BoxInfo {
    const uint8_t x; // X-coordinate
    const uint8_t y; // Y-coordinate
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

    FuriTimer* timer; // Timer for redrawing the screen
};

// The selected pad on the toypad
uint8_t selectedBox = 1; // Variable to keep track of which toypad box is selected

// function get uid from index
// uint8_t* get_uid_from_index(int index) {
//     if(index < 0 || index >= MAX_TOKENS) {
//         return NULL; // Invalid index
//     }
//     return emulator->tokens[index]->uid;
// }

// int get_id_from_index(int index) {
//     if(index < 0 || index >= MAX_TOKENS) {
//         return 0; // Invalid index
//     }

//     // when the token is a vehicle get the id from the token payload
//     if(!emulator->tokens[index]->id) {
//         int id = emulator->tokens[index]->token[0x24 * 4] |
//                  (emulator->tokens[index]->token[0x25 * 4] << 8);
//         // convert the id to little endian
//         id = (id & 0xFF00) >> 8 | (id & 0x00FF) << 8;
//         return id;
//     } else {
//         return emulator->tokens[index]->id;
//     }

//     if(emulator->tokens[index]->id) {
//         return emulator->tokens[index]->id;
//     } else {
//         return 0;
//     }
// }

// int get_id_from_token(Token* token) {
//     if(token->id) {
//         return token->id;
//     } else {
//         // when the token is a vehicle get the id from the token payload
//         int id = token->token[0x24 * 4] | (token->token[0x25 * 4] << 8);
//         // convert the id to little endian
//         id = (id & 0xFF00) >> 8 | (id & 0x00FF) << 8;
//         return id;
//     }
// }

Token* get_token_from_index(int index) {
    if(index < 0 || index >= MAX_TOKENS) {
        return NULL; // Invalid index
    }
    return emulator->tokens[index];
}

bool ldtoypad_scene_emulate_input_callback(InputEvent* event, void* context) {
    LDToyPadSceneEmulate* instance = context;
    furi_assert(instance);

    bool consumed = false;

    static const Views submenu_selection_views[] = {
        ViewMinifigureSelection, ViewVehicleSelection, ViewFavoritesSelection, ViewSavedSelection};

    with_view_model(
        instance->view,
        LDToyPadSceneEmulateModel * model,
        {
            if(model->show_placement_selection_screen && event->type == InputTypePress) {
                if(event->key == InputKeyLeft) {
                    model->sub_screen_box_selected =
                        (model->sub_screen_box_selected + SelectionCount - 1) % SelectionCount;
                } else if(event->key == InputKeyRight) {
                    model->sub_screen_box_selected =
                        (model->sub_screen_box_selected + 1) % SelectionCount;
                } else if(event->key == InputKeyOk) {
                    view_dispatcher_switch_to_view(
                        app->view_dispatcher,
                        submenu_selection_views[model->sub_screen_box_selected]);
                }
            } else {
                // when the OK button is pressed, we want to switch to the minifigure selection screen for the selected box
                if(event->key == InputKeyOk) {
                    if(event->type == InputTypePress) {
                        model->ok_pressed = true;
                    }
                    if(event->type == InputTypeShort && model->show_mini_menu_selected) {
                        bool isVehicle = get_token_from_index(boxInfo[selectedBox].index)->id == 0;

                        switch(model->mini_option_selected) {
                        case MiniSelectionFavorite:
                            // Save the token to favorites
                            if(!isVehicle) {
                                int id = get_token_from_index(boxInfo[selectedBox].index)->id;
                                if(id) {
                                    // check if the minifigure is already a favorite then unfavorite it
                                    if(is_favorite(id)) {
                                        unfavorite(id, app);
                                    } else {
                                        // save the minifigure to favorites
                                        favorite(id, app);
                                    }
                                }
                            }
                            // else {
                            //     // TODO: Implement vehicle favorites
                            // }
                            break;
                        case MiniSelectionSave:
                            Token* token = get_token_from_index(boxInfo[selectedBox].index);
                            if(!token->id) {
                                save_token(token);

                                fill_saved_submenu(app);
                            }
                            break;
                        default:
                            break;
                        }

                        model->show_mini_menu_selected = false;
                        model->ok_pressed = false;
                        return true;
                    } else if(
                        event->type == InputTypeLong && model->connected &&
                        boxInfo[selectedBox].isFilled) {
                        model->show_mini_menu_selected = !model->show_mini_menu_selected;
                    } else if(event->type == InputTypeShort && model->connected) {
                        if(boxInfo[selectedBox].isFilled) {
                            // if the box is filled, we want to remove the minifigure from the selected box
                            int i = boxInfo[selectedBox].index;
                            if(i >= 0 && ToyPadEmu_remove(i)) {
                                boxInfo[selectedBox].isFilled = false;
                                boxInfo[selectedBox].index = -1; // Reset index
                                set_debug_text("Going to remove minifig from toypad");
                                consumed = true;
                            }
                            return consumed;
                        } else {
                            // if the current selected box is not filled, we want to switch to the minifigure selection screen

                            // set current view to minifigure / vehicle selection screen
                            model->show_placement_selection_screen = true;

                            if(model->minifig_only_mode) {
                                view_dispatcher_switch_to_view(
                                    app->view_dispatcher, ViewMinifigureSelection);
                            }
                        }
                    } else if(event->type == InputTypeRelease) {
                        model->ok_pressed = false;
                    }
                }
                if((event->key == InputKeyLeft || event->key == InputKeyRight) &&
                   model->show_mini_menu_selected) {
                    model->show_mini_menu_selected = false;
                }

                if(model->show_mini_menu_selected && event->type == InputTypePress) {
                    if(event->key == InputKeyUp) {
                        model->mini_option_selected =
                            (model->mini_option_selected + MiniSelectionCount - 1) %
                            MiniSelectionCount;
                        return true;
                    } else if(event->key == InputKeyDown) {
                        model->mini_option_selected =
                            (model->mini_option_selected + 1) % MiniSelectionCount;
                        return true;
                    }
                }

                // make user loop through boxes with InputKeyLeft, InputKeyRight, InputKeyUp, InputKeyDown
                switch(event->key) {
                case InputKeyLeft:
                    if(event->type == InputTypePress) {
                        model->left_pressed = true;
                        if(selectedBox == 0) {
                            selectedBox = numBoxes;
                        }
                        selectedBox--;
                    } else if(event->type == InputTypeRelease) {
                        model->left_pressed = false;
                    }
                    break;
                case InputKeyRight:
                    if(event->type == InputTypePress) {
                        model->right_pressed = true;
                        selectedBox++;
                        if(selectedBox >= numBoxes) {
                            selectedBox = 0;
                        }
                    } else if(event->type == InputTypeRelease) {
                        model->right_pressed = false;
                    }
                    break;
                case InputKeyUp:
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
                    break;
                case InputKeyDown:
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
                    break;
                default:
                    break;
                }
            }
        },
        true);

    return consumed;
}

unsigned char generate_checksum_for_command(const unsigned char* command, size_t len) {
    unsigned char result = 0;

    // Add bytes, wrapping naturally with unsigned char overflow
    for(size_t i = 0; i < len; ++i) {
        result += command[i];
    }

    return result;
}

void selectedBox_to_pad(Token* token, int selectedBox) {
    // Convert / map the boxes to pads there are 3 pads and 7 boxes
    // TODO: This needs to be looked at, as I don't know the correct order yet
    switch(selectedBox) {
    case 0:
        token->pad = 2;
        break;
    case 1:
        token->pad = 1; // Circle
        break;
    case 2:
        token->pad = 3;
        break;
    case 3:
        token->pad = 2;
        break;
    case 4:
        token->pad = 2;
        break;
    case 5:
        token->pad = 3;
        break;
    case 6:
        token->pad = 3;
        break;
    default:
        furi_crash("Selected pad is invalid"); // It should never reach this.
        break;
    }
}

bool place_token(Token* token, int selectedBox) {
    // check if the selected token is already placed on the toypad check by uid if the token is already placed then remove the old one and place the new one
    for(int i = 0; i < MAX_TOKENS; i++) {
        if(emulator->tokens[i] != NULL) {
            if(memcmp(emulator->tokens[i]->uid, token->uid, 7) == 0) {
                // remove the old token
                if(ToyPadEmu_remove(i)) {
                    // get the box of the old token and set it to not filled
                    for(int j = 0; j < numBoxes; j++) {
                        if(boxInfo[j].index == i) {
                            boxInfo[j].isFilled = false;
                            boxInfo[j].index = -1; // Reset index
                            break;
                        }
                    }
                }
                furi_delay_ms(TOKEN_DELAY_TIME); // wait for the token to be removed
                break;
            }
        }
    }

    unsigned char buffer[32] = {0};

    boxInfo[selectedBox].isFilled = true;
    selectedBox_to_pad(token, selectedBox);

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
        free(token);
        return false;
    }

    token->index = new_index;
    emulator->tokens[new_index] = token;
    boxInfo[selectedBox].index = new_index;

    // Send placement command
    buffer[0] = FRAME_TYPE_REQUEST;
    buffer[1] = 0x0b; // Size always 11
    buffer[2] = token->pad;
    buffer[3] = 0x00;
    buffer[4] = token->index;
    buffer[5] = 0x00;
    memcpy(&buffer[6], token->uid, 7);
    buffer[13] = generate_checksum_for_command(buffer, 13);

    usbd_ep_write(get_usb_device(), HID_EP_IN, buffer, sizeof(buffer));

    // Give dolphin some xp for placing a minifigure, vehicle
    dolphin_deed(DolphinDeedNfcReadSuccess);

    return true;
}

static const char* all_mini_menu_labels[] = {"Add favorite", "Save vehicle"};

static void ldtoypad_scene_emulate_draw_render_callback(Canvas* canvas, void* context) {
    LDToyPadSceneEmulateModel* model = context;

    if(model->show_placement_selection_screen) {
        draw_placement_selection_screen(canvas, model->sub_screen_box_selected);
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
        model->sub_screen_box_selected =
            SelectionMinifigure; // Set the minifigure box selected as this is the most commonnly used at start of the app.

        // reset the filled boxes
        for(int i = 0; i < numBoxes; i++) {
            boxInfo[i].isFilled = false;
            boxInfo[i].index = -1;
        }

        // Give dolphin some xp for connecting the toypad
        dolphin_deed(DolphinDeedPluginStart);

        if(toypadscene_instance->timer != NULL) {
            furi_timer_stop(toypadscene_instance->timer);
            furi_timer_start(toypadscene_instance->timer, furi_ms_to_ticks(5000));
        }
    } else if(model->connected) {
        model->connection_status = "USB Connected";
    } else if(!model->connected) {
        model->connection_status = "Trying to connect USB";
    }

    if((model->selected_minifigure_index > 0 || model->selected_vehicle_index > 0) &&
       model->connected) {
        int id = model->selected_minifigure_index > 0 ? model->selected_minifigure_index :
                                                        model->selected_vehicle_index;

        bool is_vehicle = (model->selected_vehicle_index > 0);

        if(is_vehicle) {
            model->selected_vehicle_index = 0;
        } else {
            model->selected_minifigure_index = 0;
        }

        if(!is_vehicle && id < 1) id = 1;
        if(is_vehicle && id < 1000) id = 1000;

        Token* token = is_vehicle ? createVehicle(id, (uint32_t[]){0, 0}) : createCharacter(id);
        if(!token) return; // Handle allocation failure

        place_token(token, selectedBox);
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

    int token_selected = 0;

    // when the box is filled, draw the minifigure icon
    for(int i = 0; i < numBoxes; i++) {
        if(boxInfo[i].isFilled) {
            Token* token = emulator->tokens[boxInfo[i].index];
            if(model->show_icons_index) {
                if(token->id) { // Only minifigures have an id, vehicles have no id, but is stored in the "token[180]"
                    // Draw the minifigure icon
                    canvas_draw_icon(canvas, boxInfo[i].x + 4, boxInfo[i].y + 3, &I_head);
                } else {
                    // Draw the vehicle icon
                    canvas_draw_icon(canvas, boxInfo[i].x + 4, boxInfo[i].y + 3, &I_car);
                }
            } else {
                // Draw the first letter of the minifigure name
                char letter[1] = {0};

                // Find the first letter that is not '*' or space
                for(char* p = token->name; *p; p++) {
                    if(*p != '*' && *p != ' ') {
                        letter[0] = *p;
                        break;
                    }
                }

                canvas_draw_str(canvas, boxInfo[i].x + 6, boxInfo[i].y + 12, letter);
            }

            // Set the connection status text to the currently connected minifigure name
            if(selectedBox == i) {
                model->connection_status = token->name;

                if(token->id) {
                    token_selected = 1;
                } else {
                    // vehicles dont have an id stored in token->id, but in token[180]
                    token_selected = 2;
                }
            }
        }
    }

    if(token_selected == 1) {
        canvas_draw_icon(canvas, 0, 0, &I_head);
    } else if(token_selected == 2) {
        canvas_draw_icon(canvas, 0, 0, &I_car);
    }
    if(token_selected) {
        elements_multiline_text_aligned(
            canvas, 15, 1, AlignLeft, AlignTop, model->connection_status);
    } else {
        elements_multiline_text_aligned(
            canvas, 1, 1, AlignLeft, AlignTop, model->connection_status);
    }

    if(model->show_debug_text_index) {
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

    if(model->show_mini_menu_selected && model->connected && token_selected) {
        // Adjust position depending on selectedBox
        // TODO: Needs to be looked at if positions are good for all boxes
        if(selectedBox == 2 || selectedBox == 6) {
            x -= 75;
            y += 10;
        } else if(selectedBox == 1) {
            x -= 5;
            y += 5;
        } else {
            x += 20;
            y += 10;
        }

        // Determine which menu items to show
        const char* visible_labels[2];
        int visible_count = 0;

        if(token_selected == 1) {
            // Only minifig, show Add favorite
            visible_labels[visible_count++] = all_mini_menu_labels[0];

            // change add favorite to remove favorite if the minifigure is already a favorite
            if(is_favorite(get_token_from_index(boxInfo[selectedBox].index)->id)) {
                visible_labels[0] = "Remove favorite";
            }
        } else if(token_selected == 2) {
            // Only vehicle, show only save vehicle
            visible_labels[visible_count++] = all_mini_menu_labels[1];
            model->mini_option_selected = MiniSelectionSave;
        }

        // Draw visible menu
        for(int i = 0; i < visible_count; i++) {
            // if(i == model->mini_option_selected) {
            //     canvas_set_font(canvas, FontPrimary);
            // } else {
            //     canvas_set_font(canvas, FontSecondary);
            // }

            // Currebtly only one label is shown, so no need to change font depending on selection
            canvas_set_font(canvas, FontPrimary);

            elements_multiline_text_framed(
                canvas,
                x,
                y + i * 12, // vertical spacing
                visible_labels[i]);
        }

        canvas_set_font(canvas, FontPrimary);
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
            if(model->show_placement_selection_screen) {
                model->show_placement_selection_screen = false;
                return ViewEmulate;
            }

            if(model->show_mini_menu_selected) {
                model->show_mini_menu_selected = false;
                return ViewEmulate;
            }
        },
        true);

    return ViewSubmenu;
}

void ldtoypad_scene_emulate_view_game_timer_callback(void* context) {
    UNUSED(context);
    view_dispatcher_send_custom_event(get_view_dispatcher(), 0);
}

void ldtoypad_scene_emulate_enter_callback(void* context) {
    uint32_t period = furi_ms_to_ticks(200);
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
    for(int i = 0; i < MAX_TOKENS; i++) {
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

void saved_token_submenu_callback(void* context, uint32_t index) {
    UNUSED(index);
    // get file path from the context char file_path
    FuriString* filepath = (FuriString*)context;

    if(furi_string_utf8_length(filepath) == 0) {
        set_debug_text("Not good filepath");
        return;
    }

    set_debug_text((char*)furi_string_get_cstr(filepath));

    // Load the token from the file
    Token* token = load_saved_token((char*)furi_string_get_cstr(filepath));
    if(token == NULL) {
        return;
    }

    with_view_model(
        ldtoypad_scene_emulate_get_view(app->view_scene_emulate),
        LDToyPadSceneEmulateModel * model,
        {
            model->selected_minifigure_index = 0;
            model->selected_vehicle_index = 0;
            if(model->connected) {
                // set the token to the selected index
                Token* token = load_saved_token((char*)furi_string_get_cstr(filepath));
                if(token != NULL) {
                    place_token(token, selectedBox);
                    set_debug_text(token->name);
                }
            }
            model->show_placement_selection_screen = false;
        },
        true);

    view_dispatcher_switch_to_view(app->view_dispatcher, ViewEmulate);
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
            model->show_placement_selection_screen = false;
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
            model->show_placement_selection_screen = false;
        },
        true);

    view_dispatcher_switch_to_view(app->view_dispatcher, ViewEmulate);
}

int get_token_count_of_specific_id(unsigned int id) {
    // Get the number of tokens with the same ID

    with_view_model(
        ldtoypad_scene_emulate_get_view(app->view_scene_emulate),
        LDToyPadSceneEmulateModel * model,
        {
            // if quick swap is enabled, we want to return 0
            if(model->quick_swap) {
                return 0;
            }
        },
        false);

    int count = 0;

    for(int i = 0; i < MAX_TOKENS; i++) {
        if(emulator->tokens[i] != NULL) {
            if(emulator->tokens[i]->id == id) {
                count++;
            }
        } else {
            break;
        }
    }
    return count;
}
