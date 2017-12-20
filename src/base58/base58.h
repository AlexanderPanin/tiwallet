#ifndef BASE58_H
#define BASE58_H

#include <stdio.h>
#include <stdint.h>

static const char* ALPHABET = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
static const char ENCODED_ZERO = '1';

int base58_encode(uint8_t *input, size_t inputSize, char *encoded);
uint8_t base58_divmod(uint8_t *number, size_t numberSize, int firstDigit, int base, int divisor);

#endif
