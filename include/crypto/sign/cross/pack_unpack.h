#ifndef CROSS_PACK_UNPACK_H
#define CROSS_PACK_UNPACK_H

#include <stdint.h>
#include <stddef.h>
#include <crypto/sign/cross/parameters.h>

void pack_syndrome (const uint16_t * in, uint8_t * out);
void pack_resp_v (const uint8_t * in, uint8_t * out);
void pack_resp_y (const uint16_t * in, uint8_t * out);

_Bool unpack_syndrome (const uint8_t * in, uint16_t * out);
_Bool unpack_resp_v (const uint8_t * in, uint8_t * out);
_Bool unpack_resp_y (const uint8_t * in, uint16_t * out);

#endif
