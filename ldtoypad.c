#include "ldtoypad.h"

/* generated by fbt from .png files in images folder */
#include <ldtoypad_icons.h>

#include "minifigures.h"

// Our application menu has 3 items.  You can add more items if you want.
typedef enum {
    EmulateToyPadSubmenuIndex,
    SettingsSubmenuIndex,
    AboutSubmenuIndex,
} AppSubmenuIndex;

/**
 * @brief      Callback for exiting the application.
 * @details    This function is called when user press back button.  We return VIEW_NONE to
 *            indicate that we want to exit the application.
 * @param      _context  The context - unused
 * @return     next view id
*/
static uint32_t ldtoypad_navigation_exit_callback(void* _context) {
    UNUSED(_context);
    return VIEW_NONE;
}

/**
 * @brief      Callback for returning to submenu.
 * @details    This function is called when user press back button.  We return VIEW_NONE to
 *            indicate that we want to navigate to the submenu.
 * @param      _context  The context - unused
 * @return     next view id
*/
static uint32_t ldtoypad_navigation_submenu_callback(void* _context) {
    UNUSED(_context);
    return ViewSubmenu;
}

/**
 * @brief      Callback for returning to configure screen.
 * @details    This function is called when user press back button.  We return VIEW_NONE to
 *            indicate that we want to navigate to the configure screen.
 * @param      _context  The context - unused
 * @return     next view id
*/
static uint32_t ldtoypad_navigation_configure_callback(void* _context) {
    UNUSED(_context);
    return ViewConfigure;
}

/**
 * @brief      Handle submenu item selection.
 * @details    This function is called when user selects an item from the submenu.
 * @param      context  The context - LDToyPadApp object.
 * @param      index     The AppSubmenuIndex item that was clicked.
*/
static void ldtoypad_submenu_callback(void* context, uint32_t index) {
    LDToyPadApp* app = (LDToyPadApp*)context;
    switch(index) {
    case EmulateToyPadSubmenuIndex:
        view_dispatcher_switch_to_view(app->view_dispatcher, ViewEmulate);
        break;
    case SettingsSubmenuIndex:
        view_dispatcher_switch_to_view(app->view_dispatcher, ViewConfigure);
        break;
    case AboutSubmenuIndex:
        view_dispatcher_switch_to_view(app->view_dispatcher, ViewAbout);
        break;
    default:
        break;
    }
}

/**
 * First setting is the show debug text setting. This setting has 2 options: yes or no. Default is no.
*/
static const char* setting_show_debug_text_config_label = "Show Debug text";
static uint8_t setting_show_debug_text_values[] = {false, true};
static char* setting_show_debug_text_names[] = {"No", "Yes"};
static void ldtoypad_setting_setting_show_debug_text_index_change(VariableItem* item) {
    LDToyPadApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, setting_show_debug_text_names[index]);
    LDToyPadSceneEmulateModel* model =
        view_get_model(ldtoypad_scene_emulate_get_view(app->view_scene_emulate));
    model->show_debug_text_index = index;
}

/**
 * Our 2nd sample setting is a text field.  When the user clicks OK on the configuration 
 * setting we use a text input screen to allow the user to enter a name.  This function is
 * called when the user clicks OK on the text input screen.
*/
static const char* setting_2_config_label = "Name";
static const char* setting_2_entry_text = "Enter name";
static const char* setting_2_default_value = "Bob";
static void ldtoypad_setting_2_text_updated(void* context) {
    LDToyPadApp* app = (LDToyPadApp*)context;
    bool redraw = true;
    with_view_model(
        ldtoypad_scene_emulate_get_view(app->view_scene_emulate),
        LDToyPadSceneEmulateModel * model,
        {
            furi_string_set(model->setting_2_name, app->temp_buffer);
            variable_item_set_current_value_text(
                app->setting_2_item, furi_string_get_cstr(model->setting_2_name));
        },
        redraw);
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewConfigure);
}

/**
 * @brief      Callback when item in configuration screen is clicked.
 * @details    This function is called when user clicks OK on an item in the configuration screen.
 *            If the item clicked is our text field then we switch to the text input screen.
 * @param      context  The context - LDToyPadApp object.
 * @param      index - The index of the item that was clicked.
*/
static void ldtoypad_setting_item_clicked(void* context, uint32_t index) {
    LDToyPadApp* app = (LDToyPadApp*)context;
    index++; // The index starts at zero, but we want to start at 1.

    // Our configuration UI has the 2nd item as a text field.
    if(index == 2) {
        // Header to display on the text input screen.
        text_input_set_header_text(app->text_input, setting_2_entry_text);

        // Copy the current name into the temporary buffer.
        bool redraw = false;
        with_view_model(
            ldtoypad_scene_emulate_get_view(app->view_scene_emulate),
            LDToyPadSceneEmulateModel * model,
            {
                strncpy(
                    app->temp_buffer,
                    furi_string_get_cstr(model->setting_2_name),
                    app->temp_buffer_size);
            },
            redraw);

        // Configure the text input.  When user enters text and clicks OK, ldtoypad_setting_text_updated be called.
        bool clear_previous_text = false;
        text_input_set_result_callback(
            app->text_input,
            ldtoypad_setting_2_text_updated,
            app,
            app->temp_buffer,
            app->temp_buffer_size,
            clear_previous_text);

        // Pressing the BACK button will reload the configure screen.
        view_set_previous_callback(
            text_input_get_view(app->text_input), ldtoypad_navigation_configure_callback);

        // Show text input dialog.
        view_dispatcher_switch_to_view(app->view_dispatcher, ViewTextInput);
    }
}

static uint32_t minifigures_submenu_previous_callback(void* context) {
    UNUSED(context);
    return ViewEmulate;
}

ViewDispatcher* view_dispatcher;

ViewDispatcher* get_view_dispatcher() {
    return view_dispatcher;
}
/**
 * @brief      Allocate the ldtoypad application. Set up the views and resources.
 * @details    This function allocates the ldtoypad application resources.
 * @return     LDToyPadApp object.
*/
static LDToyPadApp* ldtoypad_app_alloc() {
    LDToyPadApp* app = (LDToyPadApp*)malloc(sizeof(LDToyPadApp));

    Gui* gui = furi_record_open(RECORD_GUI);

    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher = app->view_dispatcher;
    view_dispatcher_attach_to_gui(app->view_dispatcher, gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);

    app->submenu = submenu_alloc();
    submenu_add_item(
        app->submenu, "Emulate", EmulateToyPadSubmenuIndex, ldtoypad_submenu_callback, app);
    submenu_add_item(app->submenu, "Config", SettingsSubmenuIndex, ldtoypad_submenu_callback, app);
    submenu_add_item(app->submenu, "About", AboutSubmenuIndex, ldtoypad_submenu_callback, app);

    view_set_previous_callback(submenu_get_view(app->submenu), ldtoypad_navigation_exit_callback);
    view_dispatcher_add_view(app->view_dispatcher, ViewSubmenu, submenu_get_view(app->submenu));
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewSubmenu);

    app->text_input = text_input_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, ViewTextInput, text_input_get_view(app->text_input));
    app->temp_buffer_size = 32;
    app->temp_buffer = (char*)malloc(app->temp_buffer_size);

    app->variable_item_list_config = variable_item_list_alloc();
    variable_item_list_reset(app->variable_item_list_config);
    VariableItem* item = variable_item_list_add(
        app->variable_item_list_config,
        setting_show_debug_text_config_label,
        COUNT_OF(setting_show_debug_text_values),
        ldtoypad_setting_setting_show_debug_text_index_change,
        app);

    bool setting_show_debug_text_index = 0;
    variable_item_set_current_value_index(item, setting_show_debug_text_index);
    variable_item_set_current_value_text(
        item, setting_show_debug_text_names[setting_show_debug_text_index]);

    FuriString* setting_2_name = furi_string_alloc();
    furi_string_set_str(setting_2_name, setting_2_default_value);
    app->setting_2_item = variable_item_list_add(
        app->variable_item_list_config, setting_2_config_label, 1, NULL, NULL);
    variable_item_set_current_value_text(
        app->setting_2_item, furi_string_get_cstr(setting_2_name));
    variable_item_list_set_enter_callback(
        app->variable_item_list_config, ldtoypad_setting_item_clicked, app);

    view_set_previous_callback(
        variable_item_list_get_view(app->variable_item_list_config),
        ldtoypad_navigation_submenu_callback);
    view_dispatcher_add_view(
        app->view_dispatcher,
        ViewConfigure,
        variable_item_list_get_view(app->variable_item_list_config));

    app->view_scene_emulate = ldtoypad_scene_emulate_alloc(app);

    // This is allready happening in ldtoypad_scene_emulate_alloc

    // view_set_draw_callback(app->view_scene_emulate->view, ldtoypad_view_game_draw_callback);
    // view_set_input_callback(app->view_scene_emulate->view, ldtoypad_view_game_input_callback);
    // view_set_previous_callback(app->view_scene_emulate->view, ldtoypad_navigation_submenu_callback);
    // view_set_context(app->view_scene_emulate->view, app);
    // view_set_custom_callback(app->view_game, ldtoypad_view_game_custom_event_callback);
    // view_allocate_model(app->view_game, ViewModelTypeLockFree, sizeof(AppGameModel));

    LDToyPadSceneEmulateModel* model =
        view_get_model(ldtoypad_scene_emulate_get_view(app->view_scene_emulate));

    model->show_debug_text_index = setting_show_debug_text_index;
    model->setting_2_name = setting_2_name;
    model->x = 0;
    // view_dispatcher_add_view(app->view_dispatcher, ViewEmulate, app->view_game);
    view_dispatcher_add_view(
        app->view_dispatcher,
        ViewEmulate,
        ldtoypad_scene_emulate_get_view(app->view_scene_emulate));

    app->widget_about = widget_alloc();
    widget_add_text_scroll_element(
        app->widget_about,
        0,
        0,
        128,
        64,
        "This is a WIP application.\n---\nauthor: Seger\nhttps://github.com/SegerEnd");
    view_set_previous_callback(
        widget_get_view(app->widget_about), ldtoypad_navigation_submenu_callback);

    view_dispatcher_add_view(app->view_dispatcher, ViewAbout, widget_get_view(app->widget_about));

    // create a minifigure selection screen
    app->submenu_minifigure_selection = submenu_alloc();
    // get the minifigures from minifigures.h
    for(int i = 0; minifigures[i].name != NULL; i++) {
        submenu_add_item(
            app->submenu_minifigure_selection,
            minifigures[i].name,
            minifigures[i].id,
            minifigures_submenu_callback,
            app);
    }
    view_set_previous_callback(
        submenu_get_view(app->submenu_minifigure_selection),
        minifigures_submenu_previous_callback);

    view_dispatcher_add_view(
        app->view_dispatcher,
        ViewMinifigureSelection,
        submenu_get_view(app->submenu_minifigure_selection));

    submenu_set_header(app->submenu_minifigure_selection, "Select minifigure");

    return app;
}

/**
 * @brief      Free the ldtoypad application.
 * @details    This function frees the ldtoypad application resources.
 * @param      app  The ldtoypad application object.
*/
static void ldtoypad_app_free(LDToyPadApp* app) {
    view_dispatcher_remove_view(app->view_dispatcher, ViewTextInput);
    text_input_free(app->text_input);
    free(app->temp_buffer);

    view_dispatcher_remove_view(app->view_dispatcher, ViewAbout);
    widget_free(app->widget_about);

    ldtoypad_scene_emulate_free(app->view_scene_emulate);
    view_dispatcher_remove_view(app->view_dispatcher, ViewEmulate);
    // view_free(ldtoypad_scene_emulate_get_view(app->view_scene_emulate)); // This is allready happening in ldtoypad_scene_emulate_free
    free(app->view_scene_emulate);

    view_dispatcher_remove_view(app->view_dispatcher, ViewConfigure);

    variable_item_list_free(app->variable_item_list_config);

    view_dispatcher_remove_view(app->view_dispatcher, ViewSubmenu);
    submenu_free(app->submenu);

    view_dispatcher_remove_view(app->view_dispatcher, ViewMinifigureSelection);
    submenu_free(app->submenu_minifigure_selection);

    // view_dispatcher_free(app->view_dispatcher); // this is causing a crash
    furi_record_close(RECORD_GUI);

    // app->gui = NULL;
    //app->notification = NULL;

    // Remove whatever is left
    // memzero(app, sizeof(LDToyPadApp));

    free(app);
}

/**
 * @brief      Main function for ldtoypad application.
 * @details    This function is the entry point for the ldtoypad application.  It should be defined in
 *           application.fam as the entry_point setting.
 * @param      _p  Input parameter - unused
 * @return     0 - Success
*/
int32_t ldtoypad_app(void* _p) {
    UNUSED(_p);
    LDToyPadApp* app = ldtoypad_app_alloc();

    view_dispatcher_run(app->view_dispatcher);

    ldtoypad_app_free(app);

    return 0;
}
