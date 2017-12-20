#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <graphx.h>
#include <fileioc.h>

#include "qrcode/qrcode.h"
#include "base58/base58.h"
#include "hash/all.h"
#include "micro-ecc/uECC.h"


// The lower byte of Timer3 is used to collect entropy
#define timer_3_Counter (*(volatile uint32_t*)0xF20020)

#define BITCOIN_ADDRESS_VERSION 0x00
#define BITCOIN_PRIVKEY_VERSION 0x80

const char *appvar_name = "BTCWALL";

typedef struct {
    uint8_t key[32];
    char wif[53];
    char addr[35];
    uint8_t compressed;
} wallet_t;


int base58check_encode(char *encoded, uint8_t *payload, size_t payload_size) {
    int ret;
    uint8_t *inner, buf[32];

    if (!(inner = malloc(payload_size + 4)))
        return -1;

    // Calculate the payload checksum
    sha256d(payload, payload_size, buf);

    // Append the checksum to the payload
    memcpy(inner, payload, payload_size);
    memcpy(inner + payload_size, buf, 4);

    // Encode
    ret = base58_encode(inner, payload_size + 4, encoded);

    free(inner);
    return ret;
}

int make_wif_privkey(char *wif, uint8_t *privkey, int compressed) {
    uint8_t payload[34];

    payload[0] = BITCOIN_PRIVKEY_VERSION;
    payload[33] = 0x01;

    memcpy(payload + 1, privkey, 32);

    return base58check_encode(wif, payload, compressed?34:33);
}

int make_address(char* addr, uint8_t* privkey, int compressed) {
    uint8_t pubkey[64], pubkey_full[65], pubkey_hash[32], payload[21];
    struct uECC_Curve_t *curve = uECC_secp256k1();

    // Derive the public key and compute the public key hash
    if (!compressed) {
        pubkey_full[0] = 0x04;

        if (!uECC_compute_public_key(privkey, pubkey_full+1, curve))
            return -1;

        sha256(pubkey_full, 65, pubkey_hash);
    } else {
        if (!uECC_compute_public_key(privkey, pubkey, curve))
            return -1;

        uECC_compress(pubkey, pubkey_full, curve);

        sha256(pubkey_full, 33, pubkey_hash);
    }

    ripemd160(pubkey_hash, 32, pubkey_hash);

    // Make the address payload
    payload[0] = BITCOIN_ADDRESS_VERSION;
    memcpy(payload + 1, pubkey_hash, 20);

    return base58check_encode(addr, payload, 21);
}

int draw_qr(char *text, uint8_t version, uint8_t ecc, uint8_t x, uint8_t y, uint8_t module_size) {
    QRCode qr;
    uint8_t *qrData;
    uint8_t xx, yy;

    // If the attempt to allocate memory has failed
    if (!(qrData = malloc(qrcode_getBufferSize(version))))
        return -1;

    // If the generation was unsuccessful
    if (qrcode_initText(&qr, qrData, version, ecc, text) < 0) {
        free(qrData);
        return -1;
    }

    // Clean the area where the QR code will be drawn
    gfx_SetColor(255);
    gfx_FillRectangle(x, y, qr.size * module_size, qr.size * module_size);

    // Draw the modules
    gfx_SetColor(0);

    for (yy = 0; yy < qr.size; yy++) {
        for (xx = 0; xx < qr.size; xx++) {
            if (qrcode_getModule(&qr, xx, yy)) {
                gfx_FillRectangle(x + xx * module_size, y + yy * module_size, module_size, module_size);
            }
        }
    }

    free(qrData);
    return 0;
}

void gen_key(uint8_t *key) {
    uint16_t keypress = 0;
    uint8_t k, keypresses[256];

    // Obtain a truly random 256-bit key

    gfx_SetColor(255);
    gfx_FillScreen(255);
    gfx_PrintStringXY("Need to collect entropy, mash random keys...", 0, 0);
    gfx_PrintStringXY("Keypresses remaining:", 0, 8);

    while (keypress < 256) {
        gfx_FillRectangle(0, 16, 8*3, 8);
        gfx_SetTextXY(0, 16);
        gfx_PrintUInt(256-keypress, 3);

        // Wait for a keypress
        while (!(k = os_GetCSC()));

        keypresses[keypress++] = k ^ (uint8_t)timer_3_Counter;
    }

    // Combine the entropy into a 32-bit key
    sha256(keypresses, 256, key);
    memset(keypresses, 0, 256);

    gfx_FillScreen(255);
}

void main(void) {
    char buf[40];
    wallet_t wallet;
    ti_var_t wallet_var;
    int keycode, exit = 0;

    // Close all previously open files
    ti_CloseAll();

    gfx_Begin();
    gfx_SetTextFGColor(0);
    gfx_SetTextBGColor(255);

    if (wallet_var = ti_Open(appvar_name, "r")) {
        gfx_PrintStringXY("Opened an existing wallet file.", 0, 0);
        if (!ti_Read(&wallet, sizeof(wallet), 1, wallet_var)) {
            gfx_PrintStringXY("Couldn't load the wallet.", 0, 8);
        } else {
            gfx_PrintStringXY("Wallet loaded.", 0, 8);
            gfx_PrintStringXY("Press 1 to view the address.", 0, 16);
            gfx_PrintStringXY("Press 2 to view the private key.", 0, 24);
            gfx_PrintStringXY("Press 3 to delete the wallet file.", 0, 32);
            gfx_PrintStringXY("Press 4 to exit.", 0, 40);

            while (!exit) {
                switch (os_GetCSC()) {
                    case sk_1:
                        // Draw a QR code of the address
                        draw_qr(wallet.addr, 3, ECC_LOW, 80, 56, 5);
                        gfx_PrintStringXY(wallet.addr, 0, 216);

                        while(!os_GetCSC());

                        exit = 1;
                        break;
                    case sk_2:
                        // Draw a QR code of the private key
                        draw_qr(wallet.wif, 3, ECC_LOW, 80, 56, 5);

                        // Split the private key to 2 chunks (to fit on lines)
                        strncpy(buf, wallet.wif, 40);
                        gfx_PrintStringXY(buf, 0, 216);
                        strncpy(buf, wallet.wif+40, 12);
                        gfx_PrintStringXY(buf, 0, 224);

                        while(!os_GetCSC());

                        exit = 1;
                        break;
                    case sk_3:
                        gfx_PrintStringXY("Press Enter to confirm (any other key to exit).", 0, 48);

                        // Wait for a keypress
                        while (!(keycode = os_GetCSC()));

                        if (keycode == sk_Enter) {
                            if (ti_Delete(appvar_name)) {
                                gfx_PrintStringXY("The wallet has been deleted.", 0, 56);
                            } else {
                                gfx_PrintStringXY("Couldn't delete the wallet file.", 0, 56);
                            }

                            while(!os_GetCSC());
                        }

                        exit = 1;
                        break;
                    case sk_4:
                        exit = 1;
                        break;
                    default:
                        break;
                }
            }
        }
    } else {
        gen_key(wallet.key);
        make_wif_privkey(wallet.wif, wallet.key, 1);

        gfx_PrintStringXY("Generating the address...", 0, 0);
        make_address(wallet.addr, wallet.key, 1);

        gfx_PrintStringXY("Saving the wallet...", 0, 8);

        if (wallet_var = ti_Open(appvar_name, "w")) {
            if (ti_Write(&wallet, sizeof(wallet), 1, wallet_var)) {
                gfx_PrintStringXY("Wallet saved successfully.", 0, 16);
            } else {
                gfx_PrintStringXY("Couldn't save the wallet.", 0, 16);
            }
        } else {
            gfx_PrintStringXY("Couldn't open the AppVar for writing.", 0, 16);
        }

        while(!os_GetCSC());
    }

    // Close the wallet file
    ti_CloseAll();
    gfx_End();
}
