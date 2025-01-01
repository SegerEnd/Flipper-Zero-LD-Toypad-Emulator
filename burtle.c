#include <stdint.h>
#include "burtle.h"

// Rotate left function, equivalent to the `rot` function in JavaScript
uint32_t rotate(uint32_t value, uint32_t bits) {
    return ((value << bits) | (value >> (32 - bits))) & 0xFFFFFFFF;
}

// Random number generation function
uint32_t burtle_rand(Burtle* burtle) {
    uint32_t e = burtle->a - rotate(burtle->b, 21);
    burtle->a = (burtle->b ^ rotate(burtle->c, 19)) & 0xFFFFFFFF;
    burtle->b = (burtle->c + rotate(burtle->d, 6)) & 0xFFFFFFFF;
    burtle->c = (burtle->d + e) & 0xFFFFFFFF;
    burtle->d = (e + burtle->a) & 0xFFFFFFFF;

    return burtle->d;
}

// Initialize the Burtle structure with a seed
void burtle_init(Burtle* burtle, uint32_t seed) {
    burtle->a = 0xf1ea5eed;
    burtle->b = seed;
    burtle->c = seed;
    burtle->d = seed;

    // Run the rand function 42 times to initialize the state
    for(int i = 0; i < 42; ++i) {
        burtle_rand(burtle);
    }
}
