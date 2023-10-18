/* ------------------------------------------------------------------------------------------------------------------ */
/* TS-Warp - Transparent proxy server and traffic wrapper                                                             */
/* ------------------------------------------------------------------------------------------------------------------ */

/*
* Copyright (c) 2021-2023, Mikhail Zakharov <zmey20000@yahoo.com>
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


/* ------------------------------------------------------------------------------------------------------------------ */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "xedec.h"


/* ------------------------------------------------------------------------------------------------------------------ */
char *init_xcrypt(int xkey_len) {
    char *xkey = NULL;
    char *urnd_name = URANDOM;
    int urnd_fd = 0; int i = 0;

    xkey = (char *)malloc(xkey_len + 1);

    urnd_fd = open(urnd_name, O_RDONLY);
    if (read(urnd_fd, xkey, xkey_len) == -1) {}             /* Wrong encryption hash version! */
    xkey[xkey_len] = '\0';
    for (i = 0; i < xkey_len; i++) {
        xkey[i] &= ~(1 << 7);
        xkey[i] |= (1 << 0);
    }

    return xkey;
}

/* ------------------------------------------------------------------------------------------------------------------ */
char *xencrypt(char *xkey, char *prefix, char *text) {
    /* 0xHASH: klen|xkey|pref|text */

    char *int_hash = NULL, *hex_hash = NULL, *s = NULL, *d = NULL;
    int xkey_len = 0, hash_len = 0, b = 0;

    if (!xkey || !prefix || !text) return NULL;

    xkey_len = strlen(xkey);
    hash_len = sizeof(char) + xkey_len + strlen(prefix) + strlen(text);

    int_hash = (char *)malloc(hash_len + 1);
    hex_hash = (char *)malloc(hash_len * 2 + 1);

    int_hash[0] = xkey_len;
    snprintf(int_hash + 1, hash_len, "%s%s%s", xkey, prefix, text);

    for (s = int_hash + sizeof(char) + xkey_len; s <= int_hash + hash_len - 1; *s++ ^= xkey[b++ % xkey_len]) ;
    for (s = int_hash, d = hex_hash; s < int_hash + hash_len; sprintf(d, "%02X", *s++), d+=2) ;

    free(int_hash);
    return hex_hash;
}

/* ------------------------------------------------------------------------------------------------------------------ */
char *xdecrypt(char *hex_hash, char *prefix) {
    char *int_hash = NULL, *xkey = NULL, *pref = NULL, *text = NULL, *s = NULL;
    int hash_len = 0, pref_len = 0;
    char ch[3] = {0};                                                   /* ch[2] is for NULL-terminated string */
    int xkey_len = 0, b = 0;

    if (!hex_hash) return NULL;

    hash_len = strlen(hex_hash);
    int_hash = (char *)malloc(hash_len / 2 + 1);

    for (b = 0, s = int_hash; b + 1 <= hash_len; b += 2) {
        memcpy(&ch, hex_hash + b, 2);
        *s ++= (char)strtol(ch, NULL, 16);
    }

    xkey_len = int_hash[0];
    xkey = int_hash + sizeof(char);
    pref = xkey + xkey_len;
    pref_len = strlen(prefix);

    for (s = pref, b = 0; s < int_hash + (hash_len / 2); *s++ ^= xkey[b++ % xkey_len]) ;
    *s ='\0';

    if (strncmp(prefix, pref, pref_len)) return NULL;                   /* Wrong encryption hash version! */

    text = strdup(pref + pref_len);

    free(int_hash);
    return text;
}
