#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
typedef uint8_t *sdnv_t;

int sdnv_encode(uint32_t val, uint8_t* bp, size_t len);
size_t sdnv_encoding_len(uint32_t val);
int sdnv_decode(const uint8_t* bp, size_t len, uint32_t* val);
size_t sdnv_len(const uint8_t* bp);
