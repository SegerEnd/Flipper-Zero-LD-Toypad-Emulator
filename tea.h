#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Function to flip bytes in a buffer (from little-endian to big-endian)
void flipBytes(uint8_t* buf, size_t length, uint8_t* out);

// TEA encryption function
// Encrypts 64-bit data (v) using a 128-bit key (k)
// Output is stored in out
void tea_encrypt(const uint8_t* buffer, const uint8_t* key, uint8_t* out);

// TEA decryption function
// Decrypts 64-bit data (v) using a 128-bit key (k)
// Output is stored in out
void tea_decrypt(const uint8_t* buffer, const uint8_t* key, uint8_t* out);

#ifdef __cplusplus
}
#endif
