#ifndef HASH_ALL_H
#define HASH_ALL_H

void sha256(uint8_t *message, uint32_t length, uint8_t *digest);
void sha256d(uint8_t *message, uint32_t length, uint8_t *digest);
void ripemd160(uint8_t *message, uint32_t length, uint8_t *digest);

#endif
