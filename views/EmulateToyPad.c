#include "EmulateToyPad.h"

#include "../ldtoypad.h"

#include "../usb/usb_toypad.h"

#include <furi.h>
#include <furi_hal.h>

/* generated by fbt from .png files in images folder */
#include <ldtoypad_icons.h>

#include <gui/elements.h>
// #include <gui/icon_i.h> // Not yet needed apparently

struct LDToyPadEmulateView {
    View* view;
};

FuriHalUsbInterface* usb_mode_prev = NULL;

#define numBoxes 7 // the number of boxes (7 boxes always)

uint8_t selectedBox = 0; // Variable to keep track of which toypad box is selected

Submenu* selectionMenu; // The submenu to select minifigures and vehicles for each selection box

typedef struct {
    bool left_pressed;
    bool up_pressed;
    bool right_pressed;
    bool down_pressed;
    bool ok_pressed;
    bool back_pressed;
    bool connected;
} LDToyPadEmulateViewModel;

ViewDispatcher* ldtoypad_view_dispatcher;

Minifigure minifigures[] = {
    {1, "Batman"},
    {2, "Gandalf"},
    {3, "Wyldstyle"},
    {4, "Aquaman"},
    {5, "Bad Cop"},
    {6, "Bane"},
    {7, "Bart Simpson"},
    {8, "Benny"}};

void selectionMenu_callback(void* context, uint32_t index) {
    UNUSED(context);
    UNUSED(index);

    view_dispatcher_switch_to_view(ldtoypad_view_dispatcher, LDToyPadView_EmulateToyPad);
}

static void ldtoypad_process(LDToyPadEmulateView* ldtoypad_emulate_view, InputEvent* event) {
    with_view_model(
        ldtoypad_emulate_view->view,
        LDToyPadEmulateViewModel * model,
        {
            if(event->key == InputKeyOk) {
                if(event->type == InputTypePress) {
                    model->ok_pressed = true;
                    // selectedBox++;
                    // if(selectedBox > numBoxes) {
                    //     selectedBox = 0;
                    // }
                    submenu_reset(selectionMenu);

                    submenu_set_header(selectionMenu, "Select minifig/vehicle");

                    for(int i = 0; minifigures[i].name != NULL; i++) {
                        submenu_add_item(
                            selectionMenu,
                            minifigures[i].name,
                            minifigures[i].id,
                            selectionMenu_callback,
                            minifigures);
                    }
                    view_dispatcher_switch_to_view(
                        ldtoypad_view_dispatcher, LDToyPadView_SelectionMenu);
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
}

static bool ldtoypad_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    LDToyPadEmulateView* ldtoypad_emulate_view = context;
    bool consumed = false;

    if(event->type == InputTypeLong && event->key == InputKeyBack) {
        // furi_hal_hid_kb_release_all();
    } else {
        ldtoypad_process(ldtoypad_emulate_view, event);
        consumed = true;
    }

    return consumed;
}

// Create selection box icon
uint8_t I_selectionBox[] = {0xf8, 0xff, 0x00, 0x06, 0x00, 0x01, 0x03, 0x00, 0x02, 0x03, 0x00,
                            0x02, 0x03, 0x00, 0x02, 0x03, 0x00, 0x02, 0x03, 0x00, 0x02, 0x03,
                            0x00, 0x02, 0x03, 0x00, 0x02, 0x03, 0x00, 0x02, 0x03, 0x00, 0x02,
                            0x03, 0x00, 0x02, 0x03, 0x00, 0x02, 0x03, 0x00, 0x02, 0x03, 0x00,
                            0x02, 0x07, 0x00, 0x03, 0xfe, 0xff, 0x01, 0xfc, 0xff, 0x00};
// Create selection circle icon
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
    // Add more boxes as needed
};

static void ldtoypad_draw_callback(Canvas* canvas, void* context) {
    furi_assert(context);
    LDToyPadEmulateViewModel* model = context;

    // Draw the toypad layout on the background
    canvas_draw_icon(canvas, 10, 13, &I_toypad);

    // Test drawing selection box icon at (x: 2, y: 2)
    // canvas_draw_xbm(canvas, 2, 2, 18, 18, I_selectionBox);

    // Display selected box
    // uint8_t y = boxInfo[selectedBox][1];
    // canvas_draw_xbm(canvas, x, y, 18, 18, I_selectionBox); // Draw highlighted box

    // Get position for the selected box
    uint8_t x = boxInfo[selectedBox][0];
    uint8_t y = boxInfo[selectedBox][1];
    // Check if the selectedBox is 1 (circle) and draw the circle, This is hardcoded for now.
    if(selectedBox == 1) {
        canvas_draw_xbm(canvas, x, y, 22, 17, I_selectionCircle); // Draw highlighted circle
    } else {
        canvas_draw_xbm(canvas, x, y, 18, 18, I_selectionBox); // Draw highlighted box
    }

    // canvas_draw_xbm(canvas, 50, 20, 22, 17, I_selectionCircle);

    canvas_set_font(canvas, FontPrimary);

    // Displaying the connected USB status
    if(model->connected) {
        // elements_multiline_text_aligned(canvas, 1, 1, AlignLeft, AlignTop, "USB Connected");
        elements_multiline_text_aligned(
            canvas, 1, 1, AlignLeft, AlignTop, "Await wakeup from game");
    } else {
        elements_multiline_text_aligned(canvas, 1, 1, AlignLeft, AlignTop, "USB Not Connected");
        if(furi_hal_usb_get_config() == &usb_hid_ldtoypad) {
            model->connected = true;
        }
    }

    // Testing pressing buttons
    // if(model->ok_pressed) {
    //     canvas_set_color(canvas, ColorWhite);
    //     canvas_draw_box(canvas, 43, 28, 64, 16);
    //     canvas_set_color(canvas, ColorBlack);
    //     elements_multiline_text_aligned(canvas, 45, 30, AlignLeft, AlignTop, "OK pressed");
    //     // hid_toypad_send();
    // }

    // Draw a box behind the text hold to exit
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 78, 53, 75, 16);
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontSecondary);
    elements_multiline_text_aligned(canvas, 80, 55, AlignLeft, AlignTop, "Hold to exit");

    // Testing reading the USB.
    // canvas_set_color(canvas, ColorWhite);
    // canvas_draw_box(canvas, 0, 0, 40, 16);
    // canvas_set_color(canvas, ColorBlack);
    // canvas_set_font(canvas, FontPrimary);
    // int32_t value = hid_toypad_read_OUT();
    // // Convert the integer value to a string
    // char valueStr[40]; // Adjust the array size as needed
    // snprintf(valueStr, sizeof(valueStr), "%ld", (long)value);
    // elements_multiline_text_aligned(canvas, 1, 1, AlignLeft, AlignTop, valueStr);

    // Now for USB info also but below the other one
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 16, 120, 16);
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontPrimary);
    // int32_t value2 = hid_toypad_read_IN();
    // // Convert the integer value to a string
    // char value2Str[40]; // Adjust the array size as needed
    // snprintf(value2Str, sizeof(value2Str), "%ld", (long)value2);
    // elements_multiline_text_aligned(canvas, 1, 17, AlignLeft, AlignTop, value2Str);

    // from get_debug_text() function display the text
    elements_multiline_text_aligned(canvas, 1, 17, AlignLeft, AlignTop, "ep_in: ");
    elements_multiline_text_aligned(canvas, 40, 17, AlignLeft, AlignTop, get_debug_text_ep_in());

    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 0, 32, 120, 16);
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontPrimary);
    // say ep_out before the text
    elements_multiline_text_aligned(canvas, 1, 33, AlignLeft, AlignTop, "ep_out: ");
    elements_multiline_text_aligned(canvas, 40, 33, AlignLeft, AlignTop, get_debug_text_ep_out());
}

void ldtoypad_enter_callback(void* context) {
    UNUSED(context);
    // furi_assert(context);

    usb_mode_prev = furi_hal_usb_get_config();
    furi_hal_usb_unlock();
    furi_check(furi_hal_usb_set_config(&usb_hid_ldtoypad, NULL) == true);
}

uint32_t selectionMenu_prev_callback(void* context) {
    UNUSED(context);
    return LDToyPadView_EmulateToyPad;
}

LDToyPadEmulateView* ldtoypad_emulate_alloc(ViewDispatcher* view_dispatcher) {
    ldtoypad_view_dispatcher = view_dispatcher;
    LDToyPadEmulateView* ldtoypad_emulate_view = malloc(sizeof(LDToyPadEmulateView));
    ldtoypad_emulate_view->view = view_alloc();
    view_set_context(ldtoypad_emulate_view->view, ldtoypad_emulate_view);
    view_allocate_model(
        ldtoypad_emulate_view->view, ViewModelTypeLocking, sizeof(LDToyPadEmulateViewModel));
    view_set_draw_callback(ldtoypad_emulate_view->view, ldtoypad_draw_callback);
    view_set_input_callback(ldtoypad_emulate_view->view, ldtoypad_input_callback);
    view_set_enter_callback(ldtoypad_emulate_view->view, ldtoypad_enter_callback);

    // Allocate the submenu
    selectionMenu = submenu_alloc();
    view_set_previous_callback(submenu_get_view(selectionMenu), selectionMenu_prev_callback);
    view_dispatcher_add_view(
        ldtoypad_view_dispatcher, LDToyPadView_SelectionMenu, submenu_get_view(selectionMenu));
    // Items for the submenu as characters and vehicles

    return ldtoypad_emulate_view;
}

void ldtoypad_emulate_free(LDToyPadEmulateView* ldtoypad_emulate_view) {
    furi_assert(ldtoypad_emulate_view);
    view_free(ldtoypad_emulate_view->view);
    view_free(submenu_get_view(selectionMenu));

    // // Change back profile
    if(usb_mode_prev != NULL) {
        furi_hal_usb_set_config(usb_mode_prev, NULL);
    }

    // submenu_free(selectionMenu);

    free(ldtoypad_emulate_view);
}

View* ldtoypad_emulate_get_view(LDToyPadEmulateView* ldtoypad_emulate_view) {
    furi_assert(ldtoypad_emulate_view);
    return ldtoypad_emulate_view->view;
}