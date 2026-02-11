/* ------------------------------------------------------------------------------------------------------------------ */
/* TS-Warp - Transparent proxy server and traffic wrapper                                                             */
/* ------------------------------------------------------------------------------------------------------------------ */

/*
* Copyright (c) 2021-2026, Mikhail Zakharov <zmey20000@yahoo.com>
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
* following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
*    disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
*    the following disclaimer in the documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


/* -- Base64 encoding ----------------------------------------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "base64.h"

/* ------------------------------------------------------------------------------------------------------------------ */
static const char base64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* ------------------------------------------------------------------------------------------------------------------ */
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
	/* Source string in *enc, results in **dec  */
	/* Works with null-terminated strings only! */
	/* NB! free() **dec after use! */

	int enc_len = 0;
	int qttn = 0;
	char *d = NULL;
	unsigned char st0, st1, st2, st3;


	enc_len = strlen(enc);
	*dec = d = (char *)malloc(((enc_len + 3 - 1) / 3) * 4);
	for (qttn = 0; qttn < enc_len; qttn += 4) {
		/* Input sextets */
		st0 = strchr(base64_alphabet, enc[qttn]) - base64_alphabet;
		st1 = strchr(base64_alphabet, enc[qttn + 1]) - base64_alphabet;
		st2 = strchr(base64_alphabet, enc[qttn + 2]) - base64_alphabet;
		st3 = strchr(base64_alphabet, enc[qttn + 3]) - base64_alphabet;
		/* Output octets */
		*d++ = st0 << 2 | st1 >> 4;
		if (enc[qttn + 2] != '=') {
			*d++ = (st1 & 0xF) << 4 | st2 >> 2;
			if (enc[qttn + 3] != '=')
				*d++ = (st2 & 0x3) << 6 | (st3 & 0x3F);
		}
	}
	*d = 0;

	return strlen(*dec);
}
