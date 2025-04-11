#include <furi.h>
#include <storage/storage.h>
#include <gui/modules/submenu.h>

#include "save_toypad.h"

#include "../views/EmulateToyPad_scene.h"

#include "../minifigures.h"

#define TAG "LDToyPad"

#define FILEPATH_SIZE 128

#define FILE_NAME_LEN_MAX 256

#define TOKEN_FILE_EXTENSION ".toy"
#define TOKENS_DIR           "tokens"

int favorite_ids[MAX_FAVORITES]; // In-memory array for favorites
int num_favorites = 0; // Number of favorites in memory

bool open_file(File* file, char* filename, bool is_write_mode) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    if(!storage) {
        return false;
    }

    if(is_write_mode) {
        if(!storage_file_open(file, filename, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
            furi_record_close(RECORD_STORAGE);
            return false;
        }
    } else {
        if(!storage_file_open(file, filename, FSAM_READ, FSOM_OPEN_EXISTING)) {
            furi_record_close(RECORD_STORAGE);
            return false;
        }
    }

    furi_record_close(RECORD_STORAGE);
    return true;
}

void file_close_and_free(File* file) {
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

void load_favorites(void) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    char filename[FILEPATH_SIZE];
    snprintf(filename, sizeof(filename), "%s", APP_DATA_PATH(FILE_NAME_FAVORITES));

    num_favorites = 0; // Initialize to 0
    if(!open_file(file, filename, false)) {
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return;
    }

    if(!storage_file_read(file, &num_favorites, sizeof(int))) {
        FURI_LOG_E(TAG, "Failed to read favorites count");
    } else if(num_favorites > MAX_FAVORITES) {
        num_favorites = MAX_FAVORITES;
    } else if(
        num_favorites > 0 && !storage_file_read(file, favorite_ids, sizeof(int) * num_favorites)) {
    }

    file_close_and_free(file);
}

bool save_favorites(void) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    char filename[FILEPATH_SIZE];
    snprintf(filename, sizeof(filename), "%s", APP_DATA_PATH(FILE_NAME_FAVORITES));

    if(!open_file(file, filename, true)) {
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }

    if(!storage_file_write(file, &num_favorites, sizeof(int))) {
        file_close_and_free(file);
        return false;
    }

    if(num_favorites > 0 && !storage_file_write(file, favorite_ids, sizeof(int) * num_favorites)) {
        file_close_and_free(file);
        return false;
    }

    file_close_and_free(file);
    return true;
}

void fill_favorites_submenu(LDToyPadApp* app) {
    // Clear the submenu before filling it with favorites
    submenu_reset(app->submenu_favorites_selection);

    // Load favorites from file
    load_favorites();

    // Fill the submenu with favorite items
    for(int i = 0; i < num_favorites; i++) {
        if(favorite_ids[i] != 0) {
            submenu_add_item(
                app->submenu_favorites_selection,
                get_minifigure_name(favorite_ids[i]),
                favorite_ids[i],
                minifigures_submenu_callback,
                app);
        } else {
            break;
        }
    }

    submenu_set_header(app->submenu_favorites_selection, "Select a favorite");
}

bool favorite(int id, LDToyPadApp* app) {
    if(num_favorites >= MAX_FAVORITES) {
        return false;
    }

    favorite_ids[num_favorites++] = id;

    submenu_add_item(
        app->submenu_favorites_selection,
        get_minifigure_name(id),
        id,
        minifigures_submenu_callback,
        app);

    return save_favorites();
}

bool unfavorite(int id, LDToyPadApp* app) {
    for(int i = 0; i < num_favorites; i++) {
        if(favorite_ids[i] == id) {
            // Shift the remaining favorites down
            for(int j = i; j < num_favorites - 1; j++) {
                favorite_ids[j] = favorite_ids[j + 1];
            }
            num_favorites--;
            save_favorites();

            fill_favorites_submenu(app); // Refresh the submenu

            return true;
        }
    }
    return false;
}

bool is_favorite(int id) {
    for(int i = 0; i < num_favorites; i++) {
        if(favorite_ids[i] == id) {
            return true;
        }
    }
    return false;
}

bool make_token_dir(Storage* storage) {
    if(!storage_simply_mkdir(storage, APP_DATA_PATH(TOKENS_DIR))) {
        set_debug_text("Failed to mkdir tokens");
        return false;
    }
    return true;
}

// function to fill the saved submenu with the saved tokens from all .toy filess
void fill_saved_submenu(LDToyPadApp* app) {
    submenu_reset(app->submenu_saved_selection);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* dir = storage_file_alloc(storage);

    uint8_t token_count = 0;

    make_token_dir(storage);

    if(!storage_dir_open(dir, APP_DATA_PATH(TOKENS_DIR))) {
        storage_dir_close(dir);
        set_debug_text("Failed open dir Tokens");
        storage_file_free(dir);
        furi_record_close(RECORD_STORAGE);
        return;
    }

    FileInfo file_info;
    char file_name[FILE_NAME_LEN_MAX / 2];

    while(storage_dir_read(dir, &file_info, file_name, sizeof(file_name))) {
        if(file_info.flags & FSF_DIRECTORY) {
            continue;
        }

        char* file_ext = strstr(file_name, TOKEN_FILE_EXTENSION);
        if((file_ext == NULL) || (strcmp(file_ext, TOKEN_FILE_EXTENSION) != 0)) {
            continue;
        }

        File* file = storage_file_alloc(storage);

        // Construct the full file path
        char file_path[FILE_NAME_LEN_MAX];
        snprintf(file_path, sizeof(file_path), "%s/%s", APP_DATA_PATH(TOKENS_DIR), file_name);

        if(!storage_file_open(file, file_path, FSAM_READ, FSOM_OPEN_EXISTING)) {
            set_debug_text("Failed to open token file for reading");
            storage_file_free(file);
            continue;
        }

        // From the file content get the Token struct
        Token* token = (Token*)malloc(sizeof(Token));

        if(!storage_file_read(file, token, sizeof(Token))) {
            set_debug_text("Failed to read token data");
            free(token);
            storage_file_close(file);
            storage_file_free(file);
            continue;
        }

        // convert the file_path to a furi string
        FuriString* furi_filepath;

        if(app->saved_token_count < MAX_SAVED_TOKENS) {
            furi_filepath = furi_string_alloc();
            furi_string_printf(furi_filepath, "%s", file_path);
            app->saved_token_paths[app->saved_token_count++] = furi_filepath;
        } else {
            // fallback in case array is full
            storage_file_close(file);
            storage_file_free(file);
            continue;
        }

        // Add the token to the submenu
        submenu_add_item(
            app->submenu_saved_selection,
            token->name,
            0,
            saved_token_submenu_callback,
            furi_filepath);

        storage_file_close(file);
        storage_file_free(file);

        token_count++;

        free(token);
    }

    storage_dir_close(dir);
    storage_file_close(dir);

    storage_file_free(dir);

    furi_record_close(RECORD_STORAGE);

    if(token_count > 0) {
        submenu_set_header(app->submenu_saved_selection, "Select a saved vehicle");
    } else {
        submenu_set_header(app->submenu_saved_selection, "No saved vehicles");
    }
}

// save token function, save like uid.token. save the complete token struct to the file

bool save_token(Token* token) {
    if(token == NULL) {
        return false;
    }
    char uid[13] = {0};

    // convert the uid to a string of numbers
    snprintf(
        uid,
        sizeof(uid),
        "%02x%02x%02x%02x%02x%02x",
        (uint8_t)token->uid[0],
        (uint8_t)token->uid[1],
        (uint8_t)token->uid[2],
        (uint8_t)token->uid[3],
        (uint8_t)token->uid[4],
        (uint8_t)token->uid[5]);

    char name[16] = {0};

    // remove spaces from name
    for(unsigned int i = 0; i < sizeof(token->name); i++) {
        if(token->name[i] != ' ' && token->name[i] != '*') {
            name[i] = token->name[i];
        }
    }

    FuriString* temp_str = furi_string_alloc();

    furi_string_printf(
        temp_str, "%s/%s-%s%s", APP_DATA_PATH(TOKENS_DIR), name, uid, TOKEN_FILE_EXTENSION);

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    if(!make_token_dir(storage)) {
        furi_string_free(temp_str);
        file_close_and_free(file);
        return false;
    }

    if(!storage_file_open(file, furi_string_get_cstr(temp_str), FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        furi_string_free(temp_str);
        set_debug_text("Failed to open token to file");
        return false;
    }
    if(!storage_file_write(file, token, sizeof(Token))) {
        furi_string_free(temp_str);
        file_close_and_free(file);
        set_debug_text("Failed to write token to file");
        return false;
    }

    furi_string_free(temp_str);
    file_close_and_free(file);

    set_debug_text("Saved token to file");

    return true;
}

Token* load_saved_token(char* filepath) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    char filename[FILEPATH_SIZE];
    snprintf(filename, sizeof(filename), "%s", filepath);

    if(!open_file(file, filename, false)) {
        file_close_and_free(file);
        set_debug_text("Failed to open token file for reading");
        set_debug_text(filepath);
        return NULL;
    }

    Token* token = (Token*)malloc(sizeof(Token));

    if(!storage_file_read(file, token, sizeof(Token))) {
        free(token);
        file_close_and_free(file);
        set_debug_text("Failed to read token data");
        return NULL;
    }

    file_close_and_free(file);

    furi_record_close(RECORD_STORAGE);

    set_debug_text("Loaded token from file");

    return token;
}
