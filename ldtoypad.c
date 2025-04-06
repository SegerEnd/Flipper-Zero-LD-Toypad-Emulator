#include "ldtoypad.h"

/* generated by fbt from .png files in images folder */
#include <ldtoypad_icons.h>

#include "minifigures.h"

#include "usb/save_toypad.h"

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

static bool setting_bool_values[] = {false, true};
static char* setting_no_yes[] = {"No", "Yes"};

/**
 * First setting is the show debug text setting. This setting has 2 options: yes or no. Default is no.
*/
static const char* setting_show_debug_text_config_label = "Show Debug texts";
static void ldtoypad_setting_setting_show_debug_text_index_change(VariableItem* item) {
    LDToyPadApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, setting_no_yes[index]);
    LDToyPadSceneEmulateModel* model =
        view_get_model(ldtoypad_scene_emulate_get_view(app->view_scene_emulate));
    model->show_debug_text_index = index;
}

static const char* setting_show_icons_names_config_label = "Show letter or icon";
static char* setting_show_icons_names_names[] = {"Letter", "Icon"};

static void ldtoypad_setting_setting_show_icons_names_index_change(VariableItem* item) {
    LDToyPadApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, setting_show_icons_names_names[index]);
    LDToyPadSceneEmulateModel* model =
        view_get_model(ldtoypad_scene_emulate_get_view(app->view_scene_emulate));
    model->show_icons_index = index;
}

static void ldtoypad_setting_minifig_only_mode_change(VariableItem* item) {
    LDToyPadApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, setting_no_yes[index]);
    LDToyPadSceneEmulateModel* model =
        view_get_model(ldtoypad_scene_emulate_get_view(app->view_scene_emulate));
    model->minifig_only_mode = index;
}

static uint32_t minifigures_submenu_previous_callback(void* context) {
    UNUSED(context);
    return ViewEmulate;
}

ViewDispatcher* view_dispatcher;

ViewDispatcher* get_view_dispatcher() {
    return view_dispatcher;
}

static void ldtoypad_setup_dispatcher(LDToyPadApp* app, Gui* gui) {
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher = app->view_dispatcher;
    view_dispatcher_attach_to_gui(app->view_dispatcher, gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
}

static void ldtoypad_setup_main_menu(LDToyPadApp* app) {
    app->submenu = submenu_alloc();
    submenu_add_item(
        app->submenu, "Emulate", EmulateToyPadSubmenuIndex, ldtoypad_submenu_callback, app);
    submenu_add_item(app->submenu, "Config", SettingsSubmenuIndex, ldtoypad_submenu_callback, app);
    submenu_add_item(app->submenu, "About", AboutSubmenuIndex, ldtoypad_submenu_callback, app);

    view_set_previous_callback(submenu_get_view(app->submenu), ldtoypad_navigation_exit_callback);
    view_dispatcher_add_view(app->view_dispatcher, ViewSubmenu, submenu_get_view(app->submenu));
    view_dispatcher_switch_to_view(app->view_dispatcher, ViewSubmenu);
}

static void ldtoypad_setup_settings(LDToyPadApp* app) {
    app->variable_item_list_config = variable_item_list_alloc();
    variable_item_list_reset(app->variable_item_list_config);

    VariableItem* item;
    bool setting_show_debug_text_index = false;
    item = variable_item_list_add(
        app->variable_item_list_config,
        setting_show_debug_text_config_label,
        COUNT_OF(setting_bool_values),
        ldtoypad_setting_setting_show_debug_text_index_change,
        app);
    variable_item_set_current_value_index(item, setting_show_debug_text_index);
    variable_item_set_current_value_text(item, setting_no_yes[setting_show_debug_text_index]);

    bool setting_show_icons_names_index = false;
    item = variable_item_list_add(
        app->variable_item_list_config,
        setting_show_icons_names_config_label,
        COUNT_OF(setting_bool_values),
        ldtoypad_setting_setting_show_icons_names_index_change,
        app);
    variable_item_set_current_value_index(item, setting_show_icons_names_index);
    variable_item_set_current_value_text(
        item, setting_show_icons_names_names[setting_show_icons_names_index]);

    bool setting_minifig_only_mode = false;
    item = variable_item_list_add(
        app->variable_item_list_config,
        "Skip vehicle selection / Minifig only mode",
        COUNT_OF(setting_bool_values),
        ldtoypad_setting_minifig_only_mode_change,
        app);
    variable_item_set_current_value_index(item, setting_minifig_only_mode);
    variable_item_set_current_value_text(item, setting_no_yes[setting_minifig_only_mode]);

    view_set_previous_callback(
        variable_item_list_get_view(app->variable_item_list_config),
        ldtoypad_navigation_submenu_callback);

    view_dispatcher_add_view(
        app->view_dispatcher,
        ViewConfigure,
        variable_item_list_get_view(app->variable_item_list_config));
}

static void ldtoypad_setup_emulation_view(LDToyPadApp* app) {
    app->view_scene_emulate = ldtoypad_scene_emulate_alloc(app);

    LDToyPadSceneEmulateModel* model =
        view_get_model(ldtoypad_scene_emulate_get_view(app->view_scene_emulate));

    model->show_debug_text_index = false;
    model->show_icons_index = false;
    model->minifig_only_mode = false;

    view_dispatcher_add_view(
        app->view_dispatcher,
        ViewEmulate,
        ldtoypad_scene_emulate_get_view(app->view_scene_emulate));
}

static void ldtoypad_setup_about_view(LDToyPadApp* app) {
    app->widget_about = widget_alloc();
    widget_add_text_scroll_element(
        app->widget_about,
        0,
        0,
        128,
        64,
        "This is a educational project to learn how to interact and reverse engineer a Toy Pad with a Flipper Zero.\n\nhttps://github.com/SegerEnd/Flipper-Zero-LD-Toypad-Emulator \n\nCredits: \n- Berny23 for the JavaScript Toy Pad Emulator for the Raspberry Pi\n- AlinaNova21 for the Node-LD project (Node.js Lego Dimensions Library)\n- woodenphone for the analysis of the Lego Dimensions Protocol\n\nAuthor: SegerEnd");

    view_set_previous_callback(
        widget_get_view(app->widget_about), ldtoypad_navigation_submenu_callback);

    view_dispatcher_add_view(app->view_dispatcher, ViewAbout, widget_get_view(app->widget_about));
}

static void ldtoypad_setup_minifigure_menu(LDToyPadApp* app) {
    app->submenu_minifigure_selection = submenu_alloc();

    for(int i = 0; i < minifigures_count; i++) {
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
}

static void ldtoypad_setup_vehicle_menu(LDToyPadApp* app) {
    app->submenu_vehicle_selection = submenu_alloc();

    for(int i = 0; i < vehicles_count; i++) {
        submenu_add_item(
            app->submenu_vehicle_selection,
            vehicles[i].name,
            vehicles[i].id,
            vehicles_submenu_callback,
            app);
    }

    view_set_previous_callback(
        submenu_get_view(app->submenu_vehicle_selection), minifigures_submenu_previous_callback);

    view_dispatcher_add_view(
        app->view_dispatcher,
        ViewVehicleSelection,
        submenu_get_view(app->submenu_vehicle_selection));

    submenu_set_header(app->submenu_vehicle_selection, "Select vehicle");
}

static void ldtoypad_setup_favorites_menu(LDToyPadApp* app) {
    app->submenu_favorites_selection = submenu_alloc();

    fill_submenu(app);

    view_set_previous_callback(
        submenu_get_view(app->submenu_favorites_selection), minifigures_submenu_previous_callback);

    view_dispatcher_add_view(
        app->view_dispatcher,
        ViewFavoritesSelection,
        submenu_get_view(app->submenu_favorites_selection));
}

static void ldtoypad_setup_saved_menu(LDToyPadApp* app) {
    app->submenu_saved_selection = submenu_alloc();

    view_set_previous_callback(
        submenu_get_view(app->submenu_saved_selection), minifigures_submenu_previous_callback);

    view_dispatcher_add_view(
        app->view_dispatcher, ViewSavedSelection, submenu_get_view(app->submenu_saved_selection));

    submenu_set_header(app->submenu_saved_selection, "Select saved vehicle");
}

/**
 * @brief      Allocate the ldtoypad application. Set up the views, resources and settings.
 * @details    This function allocates the ldtoypad application resources.
 * @return     LDToyPadApp object.
*/
static LDToyPadApp* ldtoypad_app_alloc() {
    LDToyPadApp* app = (LDToyPadApp*)malloc(sizeof(LDToyPadApp));
    Gui* gui = furi_record_open(RECORD_GUI);

    load_favorites();

    ldtoypad_setup_dispatcher(app, gui);
    ldtoypad_setup_main_menu(app);
    ldtoypad_setup_settings(app);
    ldtoypad_setup_emulation_view(app);
    ldtoypad_setup_about_view(app);
    ldtoypad_setup_minifigure_menu(app);
    ldtoypad_setup_vehicle_menu(app);
    ldtoypad_setup_favorites_menu(app);
    ldtoypad_setup_saved_menu(app);

    return app;
}

/**
 * @brief      Free the ldtoypad application.
 * @details    This function frees the ldtoypad application resources.
 * @param      app  The ldtoypad application object.
*/
static void ldtoypad_app_free(LDToyPadApp* app) {
    view_dispatcher_remove_view(app->view_dispatcher, ViewAbout);
    widget_free(app->widget_about);

    ldtoypad_scene_emulate_free(app->view_scene_emulate);
    view_dispatcher_remove_view(app->view_dispatcher, ViewEmulate);

    free(app->view_scene_emulate);

    variable_item_list_reset(app->variable_item_list_config);
    view_dispatcher_remove_view(app->view_dispatcher, ViewConfigure);
    variable_item_list_free(app->variable_item_list_config);

    view_dispatcher_remove_view(app->view_dispatcher, ViewSubmenu);
    submenu_free(app->submenu);

    submenu_reset(app->submenu_minifigure_selection);
    view_dispatcher_remove_view(app->view_dispatcher, ViewMinifigureSelection);
    submenu_free(app->submenu_minifigure_selection);

    submenu_reset(app->submenu_vehicle_selection);
    view_dispatcher_remove_view(app->view_dispatcher, ViewVehicleSelection);
    submenu_free(app->submenu_vehicle_selection);

    submenu_reset(app->submenu_favorites_selection);
    view_dispatcher_remove_view(app->view_dispatcher, ViewFavoritesSelection);
    submenu_free(app->submenu_favorites_selection);

    submenu_reset(app->submenu_saved_selection);
    view_dispatcher_remove_view(app->view_dispatcher, ViewSavedSelection);
    submenu_free(app->submenu_saved_selection);

    furi_record_close(RECORD_GUI);

    //app->notification = NULL;

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
