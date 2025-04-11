#pragma once

#include "../ldtoypad.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FAVORITES       100
#define FILE_NAME_FAVORITES "favs.bin"

extern int favorite_ids[MAX_FAVORITES];
extern int num_favorites;

void fill_favorites_submenu(LDToyPadApp* app);
void load_favorites(void);

bool favorite(int id, LDToyPadApp* app);
bool is_favorite(int id);
bool unfavorite(int id, LDToyPadApp* app);

bool save_token(Token* token);

void fill_saved_submenu(LDToyPadApp* app);
Token* load_saved_token(char* filepath);

#ifdef __cplusplus
}
#endif
