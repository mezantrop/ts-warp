/* ------------------------------------------------------------------------------------------------------------------ */
/* TS-Warp - Transparent Socks proxy server and traffic Wrapper                                                       */
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


/* -- HTTP proxy (CONNECT method) implementation -------------------------------------------------------------------- */
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>

#include "http.h"
#include "logfile.h"
#include "utility.h"


/* ------------------------------------------------------------------------------------------------------------------ */
char *http_server_request(int socket, struct sockaddr_storage *daddr, char *dname) {
    char buf[64 * BUF_SIZE_1KB];
    int rcount;
    char *method = NULL, *host = NULL, *proto = NULL, *port = NULL, *query = NULL;


    if ((rcount = recv(socket, &buf, sizeof buf, 0)) == -1) {
        /* Quit immediately; no reply to the client */
        printl(LOG_WARN, "Unable to receive a request from the HTTP client");
        return NULL;
    }

    /* Parse HTTP request */
    buf[rcount] = '\0';
    method = strtok(buf,  " \t\r\n");
    host = strtok(NULL, ": \t");
    port = strtok(NULL, " \t");
    proto = strtok(NULL, " \t\r\n");
    if ((query = strchr(host, '?'))) *query = '\0';                   /* Cut Query part from URI */

    printl(LOG_VERB, "HTTP REQUEST: METHOD: [%s], HOST: [%s], PORT: [%s], PROTO: [%s], QUERY: [%s]",
        method, host, port, proto, query);

    return NULL;
}
