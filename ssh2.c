/* ------------------------------------------------------------------------------------------------------------------ */
/* TS-Warp - Transparent proxy server and traffic wrapper                                                             */
/* ------------------------------------------------------------------------------------------------------------------ */

/*
* Copyright (c) 2024, Mikhail Zakharov <zmey20000@yahoo.com>
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

char *gpassword = NULL;

/* ------------------------------------------------------------------------------------------------------------------ */
static void kbd_callback(const char *name, int name_len, const char *instruction, int instruction_len, int num_prompts,
    const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts, LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses, void **abstract) {

    (void)name;
    (void)name_len;
    (void)instruction;
    (void)instruction_len;
    if (num_prompts == 1) {
        responses[0].text = gpassword ? strdup(gpassword) : NULL;
        responses[0].length = gpassword ? strlen(gpassword) : 0;
    }
    (void)prompts;
    (void)abstract;
}

/* ------------------------------------------------------------------------------------------------------------------ */
LIBSSH2_CHANNEL *ssh2_client_request(int socket, LIBSSH2_SESSION *session, struct uvaddr *daddr,
    char *user, char *password, char *priv_key, char *priv_key_passphrase) {

    LIBSSH2_CHANNEL *channel = NULL;
    LIBSSH2_AGENT *agent = NULL;
    struct libssh2_agent_publickey *apubkey = NULL, *apubkey_prev = NULL;
    const char *fingerprint = NULL;
    char *userauthlist = NULL;
    int port = 0;
    int auth_pw = 0;
    char buf[61];
    int rc = 0;


    if (libssh2_session_handshake(session, socket)) {
        printl(LOG_WARN, "Unable to perform SSH2 handshake");
        return NULL;
    }

    fingerprint = libssh2_hostkey_hash(session, LIBSSH2_HOSTKEY_HASH_SHA1);
    for(int i = 0; i < 20; i++)
        sprintf(buf + i * 3, "%02X:", (unsigned char)fingerprint[i]);
    buf[60] = '\0';
    printl(LOG_INFO, "SSH2 Fingerprint: [%s]", buf);

    /* check what authentication methods are available */
    userauthlist = libssh2_userauth_list(session, user, strlen(user));
    printl(LOG_VERB, "Authentication methods: [%s]", userauthlist);
    if (userauthlist) {
        if (strstr(userauthlist, "publickey")) auth_pw |= 1;
        if (strstr(userauthlist, "password")) auth_pw |= 2;
        if (strstr(userauthlist, "keyboard-interactive")) auth_pw |= 4;

        /* Try SSH Agent first */
        agent = libssh2_agent_init(session);
        if (!agent) {
            printl(LOG_WARN, "Authentication by SSH-agent is not available!");
            goto manualauth;
        }

        if (libssh2_agent_connect(agent)) {
            printl(LOG_WARN, "Unable to connect with SSH-agent!");
            goto manualauth;
        }

        if (libssh2_agent_list_identities(agent)) {
            printl(LOG_WARN, "Unable to request identities from SSH-agent!");
            goto manualauth;
        }

        while (1) {
            rc = libssh2_agent_get_identity(agent, &apubkey, apubkey_prev);

            if (rc == 1) break;

            if (rc < 0) {
                printl(LOG_WARN, "Unable to obtain identity from SSH-agent!");
                goto manualauth;
            }

            if (libssh2_agent_userauth(agent, user, apubkey))
                printl(LOG_WARN, "Authentication with username [%s] and public key [%s] failed!",
                    user, apubkey->comment);
            else {
                printl(LOG_VERB, "Authentication with username [%s] and public key [%s] succeeded",
                    user, apubkey->comment);
                break;
            }

            apubkey_prev = apubkey;
        }

        if (rc) {
            printl(LOG_WARN, "Giving up agent authentication");
            goto manualauth;
        }

        manualauth:

        if (agent) {
            libssh2_agent_disconnect(agent);
            libssh2_agent_free(agent);
        }

        /* Failback to manual authentication */
        if (auth_pw & 1) {
            /*  We could authenticate by public key */
            if (libssh2_userauth_publickey_fromfile(session, user, NULL, priv_key, priv_key_passphrase))
                printl(LOG_WARN, "Authentication by public key failed!");
            else {
                printl(LOG_VERB, "Authentication by public key succeeded.");
                goto getchannel;
            }
        }

        if (auth_pw & 2) {
            /* Or via password */
            if (libssh2_userauth_password(session, user, password))
                printl(LOG_WARN, "Authentication by password failed!");
            else {
                printl(LOG_VERB,"Authentication by password succeeded.");
                goto getchannel;
            }
        }

        if (auth_pw & 4) {
            /* Or via keyboard-interactive */
            gpassword = password;
            if (libssh2_userauth_keyboard_interactive(session, user, &kbd_callback))
                printl(LOG_WARN, "Authentication by keyboard-interactive failed!");
            else {
                printl(LOG_VERB, "Authentication by keyboard-interactive succeeded.");
                goto getchannel;
            }
        }

        printl(LOG_WARN, "No supported authentication methods found!");
        return NULL;
    }

    getchannel:

    printl(LOG_VERB, "SSH2 Getting a Channel");

    switch (daddr->ip_addr.ss_family) {
        case AF_INET:
            if (!daddr->name[0])
                inet_ntop(AF_INET, &SIN4_ADDR(daddr->ip_addr), daddr->name, INET_ADDRSTRLEN);
            port = ntohs(SIN4_PORT(daddr->ip_addr));
        break;

        case AF_INET6:
            if (!daddr->name[0])
                inet_ntop(AF_INET6, &SIN6_ADDR(daddr->ip_addr), daddr->name, INET6_ADDRSTRLEN);
            port = ntohs(SIN6_PORT(daddr->ip_addr));
        break;

        default:
            printl(LOG_WARN, "Unrecognized address family: %d", daddr->ip_addr.ss_family);
            return NULL;
    }

    printl(LOG_VERB, "Destination SSH2 address: [%s]:[%d]", daddr->name, port);
    channel = libssh2_channel_direct_tcpip(session, daddr->name, port);        /* Return channel or NULL */
    if (channel)
        libssh2_session_set_blocking(session, 0);

    return channel;
}

#endif                /* WITH_LIBSSH2 */