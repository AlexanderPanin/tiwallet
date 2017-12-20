#include <stdlib.h>

#include <stdio.h>
#include <string.h>

#include "base58.h"

// Credit: this code was adapted from bitcoinj.

int base58_encode(uint8_t *input, size_t inputSize, char *encoded) {
    int inputStart, outputStart;
    signed int zeros;
    uint8_t *buf, *inputCopy;

    if (inputSize == 0) {
        return -1;
    }

    // Count leading zeros.
    zeros = 0;
    while (zeros < inputSize && input[zeros] == 0) {
        ++zeros;
    }

    // Convert base-256 digits to base-58 digits (plus conversion to ASCII characters)
    if (!(inputCopy = malloc(inputSize)))
        return -1;

    if (!(buf = malloc(inputSize*2))) {
        free(inputCopy);
        return -1;
    }

    // Copy the old contents of input into inputCopy
    memcpy(inputCopy, input, inputSize);
    input = inputCopy;

    outputStart = inputSize*2;

    for (inputStart = zeros; inputStart < inputSize; ) {
        buf[--outputStart] = ALPHABET[base58_divmod(input, inputSize, inputStart, 256, 58)];

        if (input[inputStart] == 0) {
            ++inputStart; // optimization - skip leading zeros
        }
    }

    // Preserve exactly as many leading encoded zeros in output as there were leading zeros in input.
    while (outputStart < inputSize*2 && buf[outputStart] == ENCODED_ZERO) {
        ++outputStart;
    }

    while (--zeros >= 0) {
        buf[--outputStart] = ENCODED_ZERO;
    }

    // Return encoded string (including encoded leading zeros).
    strncpy(encoded, (char*)(buf + outputStart), inputSize * 2 - outputStart);

    // Terminate the encoded string
    encoded[inputSize * 2 - outputStart] = 0;

    free(buf);
    free(inputCopy);
    return 0;
}

uint8_t base58_divmod(uint8_t *number, size_t numberSize, int firstDigit, int base, int divisor) {
    // this is just long division which accounts for the base of the input digits

    int i, digit, temp, remainder = 0;
    for (i = firstDigit; i < numberSize; i++) {
        digit = (int) number[i] & 0xFF;
        temp = remainder * base + digit;
        number[i] = (uint8_t) (temp / divisor);
        remainder = temp % divisor;
    }

    return remainder;
}
