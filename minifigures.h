#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int id;
    const char* name;
    // const char* world;
    // const char* abilities;
} Minifigure;

extern Minifigure minifigures[];

/**
 * @brief      Get the character name by its id
 * @param      id    The id of the character
 * @return     The character name
*/
const char* get_minifigure_name(int id);

/**
 * @brief      Get the character id by its name
 * @param      name  The name of the character
 * @return     The character id
*/
// int get_minifigure_id(const char* name);

#ifdef __cplusplus
}
#endif
