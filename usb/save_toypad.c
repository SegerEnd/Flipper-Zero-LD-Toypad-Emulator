#include <furi.h>
#include <storage/storage.h>
#include <gui/modules/submenu.h>

#include "save_toypad.h"

#include "../minifigures.h"

#define TAG "LDToyPad"

int favorite_ids[MAX_FAVORITES]; // In-memory array for favorites
int num_favorites = 0; // Number of favorites in memory

bool open_file(File* file, const char* filename, bool is_write_mode) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    if(!storage) {
        FURI_LOG_E(TAG, "Failed to open storage");
        return false;
    }

    if(is_write_mode) {
        if(!storage_file_open(file, filename, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
            FURI_LOG_E(TAG, "Failed to open favorites file for writing: %s", filename);
            furi_record_close(RECORD_STORAGE);
            return false;
        }
    } else {
        if(!storage_file_open(file, filename, FSAM_READ, FSOM_OPEN_EXISTING)) {
            FURI_LOG_E(TAG, "Failed to open favorites file for reading: %s", filename);
            furi_record_close(RECORD_STORAGE);
            return false;
        }
    }

    furi_record_close(RECORD_STORAGE);
    return true;
}

void load_favorites(void) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    char filename[128];
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
        FURI_LOG_E(TAG, "Too many favorites in file: %d > %d", num_favorites, MAX_FAVORITES);
        num_favorites = MAX_FAVORITES;
    } else if(
        num_favorites > 0 && !storage_file_read(file, favorite_ids, sizeof(int) * num_favorites)) {
        FURI_LOG_E(TAG, "Failed to read favorite IDs data");
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

bool save_favorites(void) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    char filename[64];
    snprintf(filename, sizeof(filename), "%s", APP_DATA_PATH(FILE_NAME_FAVORITES));

    if(!open_file(file, filename, true)) {
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }

    if(!storage_file_write(file, &num_favorites, sizeof(int))) {
        FURI_LOG_E(TAG, "Failed to write favorites count");
        storage_file_close(file);
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }

    if(num_favorites > 0 && !storage_file_write(file, favorite_ids, sizeof(int) * num_favorites)) {
        FURI_LOG_E(TAG, "Failed to write favorite IDs data");
        storage_file_close(file);
        storage_file_free(file);
        furi_record_close(RECORD_STORAGE);
        return false;
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return true;
}

bool save_favorite(int id, LDToyPadApp* app) {
    if(num_favorites >= MAX_FAVORITES) {
        FURI_LOG_E(TAG, "Favorites list is full, cannot add new favorite");
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

bool is_favorite(int id) {
    for(int i = 0; i < num_favorites; i++) {
        if(favorite_ids[i] == id) {
            return true;
        }
    }
    return false;
}
