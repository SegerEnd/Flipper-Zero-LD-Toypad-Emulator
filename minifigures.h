#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const int id;
    const char* name;
    // const char* world;
    // const char* abilities;
} Minifigure;

extern const Minifigure minifigures[];

typedef struct {
    const int id;
    const char* name;

    // upgrades, not yet implemented
} Vehicle;

extern const Vehicle vehicles[];

// count of minifigures
extern const int minifigures_count;

// count of vehicles

extern const int vehicles_count;

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

/**
 * @brief      Get the vehicle name by its id
 * @param      id    The id of the vehicle
 * @return     The vehicle name
*/
const char* get_vehicle_name(int id);

#ifdef __cplusplus
}
#endif
