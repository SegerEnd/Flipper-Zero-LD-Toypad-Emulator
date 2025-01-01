#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define DELTA 0x9E3779B9

#include "./usb/usb_toypad.h"

// Encipher function: core TEA encryption logic
void encipher(uint32_t* v, uint32_t* k, uint32_t* result) {
    uint32_t v0 = v[0], v1 = v[1];
    uint32_t sum = 0;

    for(int i = 0; i < 32; i++) {
        sum += DELTA;
        v0 += (((v1 << 4) + k[0]) ^ (v1 + sum) ^ ((v1 >> 5) + k[1]));
        v1 += (((v0 << 4) + k[2]) ^ (v0 + sum) ^ ((v0 >> 5) + k[3]));
    }

    result[0] = v0;
    result[1] = v1;
}

// Decipher function: core TEA decryption logic
void decipher(uint32_t* v, uint32_t* k, uint32_t* result) {
    uint32_t v0 = v[0], v1 = v[1];
    uint32_t sum = 0xC6EF3720;

    for(int i = 0; i < 32; i++) {
        v1 -= (((v0 << 4) + k[2]) ^ (v0 + sum) ^ ((v0 >> 5) + k[3]));
        v0 -= (((v1 << 4) + k[0]) ^ (v1 + sum) ^ ((v1 >> 5) + k[1]));
        sum -= DELTA;
    }

    result[0] = v0;
    result[1] = v1;
}

// Load 32-bit value safely (portable)
static uint32_t bytes_to_uint32(const uint8_t* buf) {
    return (uint32_t)buf[0] | ((uint32_t)buf[1] << 8) | ((uint32_t)buf[2] << 16) |
           ((uint32_t)buf[3] << 24);
}

// Store 32-bit value safely (portable)
static void uint32_to_bytes(uint32_t value, uint8_t* buf) {
    buf[0] = (uint8_t)(value & 0xFF);
    buf[1] = (uint8_t)((value >> 8) & 0xFF);
    buf[2] = (uint8_t)((value >> 16) & 0xFF);
    buf[3] = (uint8_t)((value >> 24) & 0xFF);
}

// Encryption function
void tea_encrypt(const uint8_t* buffer, const uint8_t* key, uint8_t* out) {
    if(!buffer || !key || !out) return;

    uint32_t v[2] = {bytes_to_uint32(buffer), bytes_to_uint32(buffer + 4)};
    uint32_t k[4] = {
        bytes_to_uint32(key),
        bytes_to_uint32(key + 4),
        bytes_to_uint32(key + 8),
        bytes_to_uint32(key + 12)};

    uint32_t result[2];
    encipher(v, k, result);

    uint32_to_bytes(result[0], out);
    uint32_to_bytes(result[1], out + 4);
}

// Decryption function
void tea_decrypt(const uint8_t* buffer, const uint8_t* key, uint8_t* out) {
    if(!buffer || !key || !out) return;

    uint32_t v[2] = {bytes_to_uint32(buffer), bytes_to_uint32(buffer + 4)};
    uint32_t k[4] = {
        bytes_to_uint32(key),
        bytes_to_uint32(key + 4),
        bytes_to_uint32(key + 8),
        bytes_to_uint32(key + 12)};

    // dechiper the buffer
    uint32_t result[2];
    decipher(v, k, result);

    // if(k[0] == 0 || v[0] == 0) {
    //     return;
    // }

    if(!out || out == NULL) {
        set_debug_text("Error: output pointer is NULL");
        return;
    }
    // if(result == NULL) {
    //     set_debug_text("Error: result is NULL");
    //     return;
    // }
    if(result[0] == 0 || result[1] == 0) {
        set_debug_text("Error: result is 0");
        return;
    }
    // uint32_to_bytes(result[0], out);
    // uint32_to_bytes(result[1], out + 4);
}
