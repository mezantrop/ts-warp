#include <stdio.h>
#include <string.h>
#include <stdlib.h>


static const char base64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


int base64_strenc(char **enc, char *dec) {
    /* Source string pointed by *dec, results in **enc */
    /* Works with null-terminated strings only! */
    /* NB! free() **enc after usage! */

    int dec_len = 0;
    int tptn = 0;
    char *e = NULL;
    char ot0, ot1, ot2;


    dec_len = strlen(dec);
    *enc = e = (char *)malloc(dec_len * 4 / 3 + 4);
    for (tptn = 0; tptn < dec_len; tptn += 3) {
        /* Input octets */
        ot0 = dec[tptn];
        ot1 = tptn + 1 <= dec_len ? dec[tptn + 1] : 0;
        ot2 = tptn + 2 <= dec_len ? dec[tptn + 2] : 0;
        /* Output sextets */
        *e++ = base64_alphabet[ot0 >> 2];
        *e++ = base64_alphabet[(ot0 & 0x3) << 4 | ot1 >> 4];
        *e++ = !ot1 ? '=' : base64_alphabet[(ot1 & 0xF) << 2 | ot2 >> 6];
        *e++ = !ot2 ? '=' : base64_alphabet[ot2 & 0x3F];
    }
    *e = 0;
    return strlen(*enc);
}


int base64_strdec(char **dec, char *enc) {
/*
    int dec_len = 0, enc_len = 0;

    enc_len = strlen(enc);
    dec_len = ((enc_len + 3 - 1) / 3) * 4;
*/
    return 0;
}
