#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define DELTA 0x9E3779B9

void flipBytes(uint8_t* buf, size_t length, uint8_t* out) {
    for(size_t i = 0; i < length; i += 4) {
        uint32_t value =
            ((uint32_t)buf[i] | (uint32_t)buf[i + 1] << 8 | (uint32_t)buf[i + 2] << 16 |
             (uint32_t)buf[i + 3] << 24);
        value = (value >> 0) & 0xFFFFFFFF; // This is just to keep the value unsigned
        out[i] = (uint8_t)(value >> 24);
        out[i + 1] = (uint8_t)(value >> 16);
        out[i + 2] = (uint8_t)(value >> 8);
        out[i + 3] = (uint8_t)value;
    }
}

void encrypt(uint8_t* buffer, uint8_t* key, uint8_t* out) {
    if(!key) {
        return;
    }

    uint32_t v0 =
        ((uint32_t)buffer[0] | (uint32_t)buffer[1] << 8 | (uint32_t)buffer[2] << 16 |
         (uint32_t)buffer[3] << 24);
    uint32_t v1 =
        ((uint32_t)buffer[4] | (uint32_t)buffer[5] << 8 | (uint32_t)buffer[6] << 16 |
         (uint32_t)buffer[7] << 24);

    uint32_t k0 =
        ((uint32_t)key[0] | (uint32_t)key[1] << 8 | (uint32_t)key[2] << 16 |
         (uint32_t)key[3] << 24);
    uint32_t k1 =
        ((uint32_t)key[4] | (uint32_t)key[5] << 8 | (uint32_t)key[6] << 16 |
         (uint32_t)key[7] << 24);
    uint32_t k2 =
        ((uint32_t)key[8] | (uint32_t)key[9] << 8 | (uint32_t)key[10] << 16 |
         (uint32_t)key[11] << 24);
    uint32_t k3 =
        ((uint32_t)key[12] | (uint32_t)key[13] << 8 | (uint32_t)key[14] << 16 |
         (uint32_t)key[15] << 24);

    uint32_t sum = 0;
    for(int i = 0; i < 32; ++i) {
        sum += DELTA;
        sum &= 0xFFFFFFFF;
        v0 += (((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1)) & 0xFFFFFFFF;
        v1 += (((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3)) & 0xFFFFFFFF;
    }

    out[0] = (uint8_t)(v0 >> 24);
    out[1] = (uint8_t)(v0 >> 16);
    out[2] = (uint8_t)(v0 >> 8);
    out[3] = (uint8_t)v0;
    out[4] = (uint8_t)(v1 >> 24);
    out[5] = (uint8_t)(v1 >> 16);
    out[6] = (uint8_t)(v1 >> 8);
    out[7] = (uint8_t)v1;
}

void decrypt(uint8_t* buffer, uint8_t* key, uint8_t* out) {
    if(!key) {
        return;
    }

    uint32_t v0 =
        ((uint32_t)buffer[0] | (uint32_t)buffer[1] << 8 | (uint32_t)buffer[2] << 16 |
         (uint32_t)buffer[3] << 24);
    uint32_t v1 =
        ((uint32_t)buffer[4] | (uint32_t)buffer[5] << 8 | (uint32_t)buffer[6] << 16 |
         (uint32_t)buffer[7] << 24);

    uint32_t k0 =
        ((uint32_t)key[0] | (uint32_t)key[1] << 8 | (uint32_t)key[2] << 16 |
         (uint32_t)key[3] << 24);
    uint32_t k1 =
        ((uint32_t)key[4] | (uint32_t)key[5] << 8 | (uint32_t)key[6] << 16 |
         (uint32_t)key[7] << 24);
    uint32_t k2 =
        ((uint32_t)key[8] | (uint32_t)key[9] << 8 | (uint32_t)key[10] << 16 |
         (uint32_t)key[11] << 24);
    uint32_t k3 =
        ((uint32_t)key[12] | (uint32_t)key[13] << 8 | (uint32_t)key[14] << 16 |
         (uint32_t)key[15] << 24);

    uint32_t sum = 0xC6EF3720;
    for(int i = 0; i < 32; ++i) {
        v1 -= (((v0 << 4) + k2) ^ (v0 + sum) ^ ((v0 >> 5) + k3)) & 0xFFFFFFFF;
        v0 -= (((v1 << 4) + k0) ^ (v1 + sum) ^ ((v1 >> 5) + k1)) & 0xFFFFFFFF;
        sum -= DELTA;
        sum &= 0xFFFFFFFF;
    }

    out[0] = (uint8_t)(v0 >> 24);
    out[1] = (uint8_t)(v0 >> 16);
    out[2] = (uint8_t)(v0 >> 8);
    out[3] = (uint8_t)v0;
    out[4] = (uint8_t)(v1 >> 24);
    out[5] = (uint8_t)(v1 >> 16);
    out[6] = (uint8_t)(v1 >> 8);
    out[7] = (uint8_t)v1;
}