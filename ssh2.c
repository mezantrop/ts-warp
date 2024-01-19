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

/* -- SSH2 protocol (using https://libssh2.org/ library) ------------------------------------------------------------ */
#if (WITH_LIBSSH2)

#include <libssh2.h>
#include <string.h>
#include <sys/socket.h>

#include "utility.h"
#include "network.h"

#include "ssh2.h"

#include "logfile.h"

/* ------------------------------------------------------------------------------------------------------------------ */
extern char *pfile_name;

char *password = NULL;

/* ------------------------------------------------------------------------------------------------------------------ */
static void kbd_callback(const char *name, int name_len, const char *instruction, int instruction_len, int num_prompts,
    const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts, LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses, void **abstract) {

    (void)name;
    (void)name_len;
    (void)instruction;
    (void)instruction_len;
    if (num_prompts == 1) {
            responses[0].text = strdup(password);
            responses[0].length = strlen(password);
    }
    (void)prompts;
    (void)abstract;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int ssh2_client_request(int socket, struct sockaddr_storage *daddr, char *user, char *password, char *priv_key) {
    LIBSSH2_SESSION *session = NULL;
    const char *fingerprint;
    char *userauthlist;
    int auth_pw = 0;
    char buf[61];


    if (!(session = libssh2_session_init())) {
        printl(LOG_WARN, "Unable to initialize SSH2 session");
        return 1;
    }

    if (!libssh2_session_handshake(session, socket)) {
        printl(LOG_WARN, "Unable to perform SSH2 handshake");
        return 1;
    }

    fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);
    for(int i = 0; i < 20; i++)
        sprintf(buf + i * 3, "%02X:", (unsigned char)fingerprint[i]);
    buf[60] = '\0';

    printl(LOG_WARN, "SSH2 Fingerprint: [%s]", buf);

	/* check what authentication methods are available */
	userauthlist = libssh2_userauth_list(session, user, strlen(user));
	fprintf(stderr, "Authentication methods: %s\n", userauthlist);
	if (strstr(userauthlist, "password")) auth_pw |= 1;
	if (strstr(userauthlist, "keyboard-interactive")) auth_pw |= 2;
	if (strstr(userauthlist, "publickey")) auth_pw |= 4;

	if (auth_pw & 1) {
		/* We could authenticate via password */
        	if (libssh2_userauth_password(session, user, password)) {
            		printl(LOG_WARN, "Authentication by password failed!");
		} else {
			printl(LOG_VERB,"Authentication by password succeeded.");
			goto getchannel;
		}
	}

	if (auth_pw & 2) {
		/* Or via keyboard-interactive */
		if (libssh2_userauth_keyboard_interactive(session, user, &kbd_callback) ) {
			printl(LOG_WARN, "Authentication by keyboard-interactive failed!");
		} else {
			printl(LOG_VERB, "Authentication by keyboard-interactive succeeded.");
			goto getchannel;
		}
	}

	if (auth_pw & 4) {
        	/* Or by public key */
		if (libssh2_userauth_publickey_fromfile(session, user, NULL, priv_key, password)) {
			printl(LOG_WARN, "Authentication by public key failed!");
			return 1;
		} else {
			printl(LOG_VERB, "Authentication by public key succeeded.");
			goto getchannel;
		}
	} else {
		printl(LOG_WARN, "No supported authentication methods found!");
		return 1;
	}

getchannel:

    return 0;
}

#endif