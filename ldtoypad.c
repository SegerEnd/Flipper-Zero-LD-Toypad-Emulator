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
static bool setting_show_debug_text_values[] = {false, true};
static char* setting_show_debug_text_names[] = {"No", "Yes"};
static void ldtoypad_setting_setting_show_debug_text_index_change(VariableItem* item) {
    LDToyPadApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, setting_show_debug_text_names[index]);
    LDToyPadSceneEmulateModel* model =
        view_get_model(ldtoypad_scene_emulate_get_view(app->view_scene_emulate));
    model->show_debug_text_index = index;
}

static const char* setting_show_icons_names_config_label = "Show letter or icon";
static bool setting_show_icons_names_values[] = {true, false};
static char* setting_show_icons_names_names[] = {"Letter", "Icon"};

static void ldtoypad_setting_setting_show_icons_names_index_change(VariableItem* item) {
    LDToyPadApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, setting_show_icons_names_names[index]);
    LDToyPadSceneEmulateModel* model =
        view_get_model(ldtoypad_scene_emulate_get_view(app->view_scene_emulate));
    model->show_icons_index = index;
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

    // app->text_input = text_input_alloc();
    // view_dispatcher_add_view(
    //     app->view_dispatcher, ViewTextInput, text_input_get_view(app->text_input));
    // app->temp_buffer_size = 32;
    // app->temp_buffer = (char*)malloc(app->temp_buffer_size);

    app->variable_item_list_config = variable_item_list_alloc();
    variable_item_list_reset(app->variable_item_list_config);
    VariableItem* item = variable_item_list_add(
        app->variable_item_list_config,
        setting_show_debug_text_config_label,
        COUNT_OF(setting_show_debug_text_values),
        ldtoypad_setting_setting_show_debug_text_index_change,
        app);

    bool setting_show_debug_text_index = false;
    variable_item_set_current_value_index(item, setting_show_debug_text_index);
    variable_item_set_current_value_text(
        item, setting_show_debug_text_names[setting_show_debug_text_index]);

    // setting 2 show icons or first letter of minifig name
    item = variable_item_list_add(
        app->variable_item_list_config,
        setting_show_icons_names_config_label,
        COUNT_OF(setting_show_icons_names_values),
        ldtoypad_setting_setting_show_icons_names_index_change,
        app);

    bool setting_show_icons_names_index = false;
    variable_item_set_current_value_index(item, setting_show_icons_names_index);
    variable_item_set_current_value_text(
        item, setting_show_icons_names_names[setting_show_icons_names_index]);

    view_set_previous_callback(
        variable_item_list_get_view(app->variable_item_list_config),
        ldtoypad_navigation_submenu_callback);
    view_dispatcher_add_view(
        app->view_dispatcher,
        ViewConfigure,
        variable_item_list_get_view(app->variable_item_list_config));

    app->view_scene_emulate = ldtoypad_scene_emulate_alloc(app);

    LDToyPadSceneEmulateModel* model =
        view_get_model(ldtoypad_scene_emulate_get_view(app->view_scene_emulate));

    model->show_debug_text_index = setting_show_debug_text_index;
    model->show_icons_index = setting_show_icons_names_index;

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
        "This is a educational project to learn how to interact and reverse engineer a Toy Pad with a Flipper Zero.\n\nhttps://github.com/SegerEnd/Flipper-Zero-LD-Toypad-Emulator \n\nCredits: \n- Berny23 for the JavaScript Toy Pad Emulator for the Raspberry Pi\n- AlinaNova21 for the Node-LD project (Node.js Lego Dimensions Library)\n- woodenphone for the analysis of the Lego Dimensions Protocol");
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
