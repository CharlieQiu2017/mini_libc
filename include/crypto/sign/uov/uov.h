#ifndef UOV_H
#define UOV_H

#include <stdint.h>

uint8_t uov_sign (const uint8_t * seed_sk, const uint8_t * o, const uint8_t * p1, const uint8_t * s, const uint8_t * msg, size_t len, const uint8_t * salt, uint8_t * out);

uint8_t uov_verify (const uint8_t * p1, const uint8_t * p2, const uint8_t * p3, const uint8_t * msg, size_t len, const uint8_t * salt, const uint8_t * sig);

#endif
