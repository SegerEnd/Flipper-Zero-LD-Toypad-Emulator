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

#ifdef __cplusplus
}
#endif
