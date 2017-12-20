#include "hash.h"
#include "common.h"

#include "sha256.h"
#include "ripemd160.h"

void sha256(uint8_t *message, uint32_t length, uint8_t *digest)
{
    uint32_t i;
    HashState hs;

    sha256Begin(&hs);
    for (i = 0; i < length; i++)
    {
        sha256WriteByte(&hs, message[i]);
    }
    sha256Finish(&hs);

    writeHashToByteArray(digest, &hs, true);
}

void sha256d(uint8_t *message, uint32_t length, uint8_t *digest)
{
    uint32_t i;
    HashState hs;

    sha256Begin(&hs);
    for (i = 0; i < length; i++)
    {
        sha256WriteByte(&hs, message[i]);
    }
    sha256FinishDouble(&hs);

    writeHashToByteArray(digest, &hs, true);
}

void ripemd160(uint8_t *message, uint32_t length, uint8_t *digest)
{
    uint32_t i;
    HashState hs;

    ripemd160Begin(&hs);
    for (i = 0; i < length; i++)
    {
        ripemd160WriteByte(&hs, message[i]);
    }
    ripemd160Finish(&hs);

    writeHashToByteArray(digest, &hs, true);
}
