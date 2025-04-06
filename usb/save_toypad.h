#pragma once

#include "../ldtoypad.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FAVORITES       100
#define FILE_NAME_FAVORITES "favorites.bin"

extern int favorite_ids[MAX_FAVORITES];
extern int num_favorites;

void load_favorites(void);
bool save_favorites(void);
bool save_favorite(int id, LDToyPadApp* app);
bool is_favorite(int id);

#ifdef __cplusplus
}
#endif
