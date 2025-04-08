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

int save_token(Token* token);
// Token* get_saved_token(const char* uid);

int get_token_count_of_specific_name(const char* name);

#ifdef __cplusplus
}
#endif
