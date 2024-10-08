/* ------------------------------------------------------------------------------------------------------------------ */
/* TS-Warp - Transparent proxy server and traffic wrapper                                                             */
/* ------------------------------------------------------------------------------------------------------------------ */

/*
* Copyright (c) 2021-2024, Mikhail Zakharov <zmey20000@yahoo.com>
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

#include "utility.h"
#include "network.h"
#include "base64.h"
#include "http.h"
#include "logfile.h"

/* ------------------------------------------------------------------------------------------------------------------ */
extern char *pfile_name;

/* ------------------------------------------------------------------------------------------------------------------ */
int http_server_request(int socket, struct uvaddr *daddr) {
    char buf[64 * BUF_SIZE_1KB] = {0};
    char rbuf[STR_SIZE] = {0};
    int l = 0;

    int rcount = 0;
    char *method = NULL, *url = NULL, *proto = NULL;
    char host[HOST_NAME_MAX] = {0};
    uint16_t port = 80;

    if ((rcount = recv(socket, &buf, sizeof(buf), 0)) == -1) {
        /* Quit immediately; no reply to the client */
        printl(LOG_WARN, "Unable to receive a request from the HTTP client");
        return 1;
    }

    if (memcmp(HTTP_REQUEST_METHOD_CONNECT, &buf, strlen(HTTP_REQUEST_METHOD_CONNECT))) {
        printl(LOG_WARN, "Incorrect HTTP method in the request");
        return 1;
    }

    /* Parse HTTP request */
    buf[rcount] = '\0';
    method = strtok(buf,  " \t\r\n");
    url = strtok(NULL, " \t\r\n");
    proto = strtok(NULL, " \t\r\n");

    /* printl(LOG_VERB, "URL: [%s]", url); */
    if (sscanf(url, "https://%[a-zA-Z0-9.-]/", host) != 1)
        if (sscanf(url, "http://%[a-zA-Z0-9.-]/", host) != 1)
            if (sscanf(url, "%[a-zA-Z0-9.-]:%hu/", host, &port) != 2)
                if (sscanf(url, "%[a-zA-Z0-9.-]:%hu", host, &port) != 2)
                    if (sscanf(url, "%[a-zA-Z0-9.-]", host) != 1) {
                        printl(LOG_WARN, "Unable to discover target host in the URL-part: [%s] of HTTP request", url);
                        return 1;
                    }

    /* TODO: Validate the request */
    strcpy(daddr->name, host);
    daddr->ip_addr = str2inet(daddr->name, NULL);
    if (SA_FAMILY(daddr->ip_addr) == AF_INET) SIN4_PORT(daddr->ip_addr) = htons(port);
    else SIN6_PORT(daddr->ip_addr) = port;

    /* TODO: Check connection; Reply real status */
    l = snprintf(rbuf, sizeof(rbuf), "%s %s OK\r\nProxy-agent: %s\r\n\r\n", proto, HTTP_RESPONSE_200, PROG_NAME_FULL);
    if (l < 1 || send(socket, rbuf, l, 0) == -1) {
        printl(LOG_CRIT, "Unable to send reply to the HTTP client");
        return 1;
    }

    printl(LOG_VERB, "INTERNAL HTTP got REQUEST: URL: [%s] METHOD: [%s], HOST: [%s], PORT: [%hu], PROTO: [%s]",
        url, method, host, port, proto);

    return 0;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int http_client_request(chs cs, struct sockaddr_storage *daddr, char *user, char *password, int sdpi) {
    char r[BUF_SIZE_1KB] = {0};
    char b[HOST_NAME_MAX] = {0};
    char usr_pwd_plain[BUF_SIZE_1KB] = {0};
    char *usr_pwd_base64;
    char *proto = NULL, *status = NULL, *reason = NULL;
    int rcount = 0;
    int l = 0;

    /* Request startline: CONNECT address:port PROTOCOL */
    if (user && password) {
        sprintf(usr_pwd_plain, "%s:%s", user, password);
        base64_strenc(&usr_pwd_base64, usr_pwd_plain);
        l = snprintf(r, sizeof(r), "%s %s %s\r\n%s %s\r\n\r\n",
            HTTP_REQUEST_METHOD_CONNECT, inet2str(daddr, b), HTTP_REQEST_PROTOCOL,
            HTTP_HEADER_PROXYAUTH_BASIC, usr_pwd_base64);
    } else
        l = snprintf(r, sizeof(r), "%s %s %s\r\n\r\n",
            HTTP_REQUEST_METHOD_CONNECT, inet2str(daddr, b), HTTP_REQEST_PROTOCOL);

    printl(LOG_VERB, "Sending HTTP %s request", HTTP_REQUEST_METHOD_CONNECT);

    switch (cs.t) {
        case CHS_SOCKET:
            if (sdpi) {
                printl(LOG_VERB, "Trying to bypass Deep Packet Inspections for HTTP proxy. Fragment size: [%d]", sdpi);

                if (send(cs.s, r, sdpi, 0) == -1 || send(cs.s, r + sdpi, l - sdpi, 0) == -1) {
                    printl(LOG_CRIT, "SDPI: Unable to send a request to the HTTP server via socket");
                    return 1;
                }
            } else
                if (send(cs.s, r, l, 0) == -1) {
                    printl(LOG_CRIT, "Unable to send a request to the HTTP server via socket");
                    return 1;
                }

            printl(LOG_VERB, "Expecting HTTP reply");

            if ((rcount = recv(cs.s, &r, sizeof(r), 0)) == -1) {
                printl(LOG_CRIT, "Unable to receive a reply from the HTTP server via socket");
                return 1;
            }
        break;

        case CHS_CHANNEL:
            #if (WITH_LIBSSH2)
                if (libssh2_channel_write(cs.c, (char*)&r, l) < 0) {
                    printl(LOG_CRIT, "Unable to send a request to the HTTP server via SSH2 channel");
                    return 1;
                }

                while ((rcount = libssh2_channel_read(cs.c, (char*)&r, sizeof(r))) == LIBSSH2_ERROR_EAGAIN) ;
                if (rcount < 0) {
                    printl(LOG_CRIT, "Unable to send a request to the HTTP server via SSH2 channel");
                    return 1;
                }
            #endif
        break;

        default:
            printl(LOG_CRIT, "Error Socket / SSH2 Channel specified");
            return 1;
        break;
        }

    /* Parse HTTP reply */
    r[rcount] = '\0';
    proto = strtok(r,  " \t\r\n");
    status = strtok(NULL, " \t\r\n");
    reason = strtok(NULL, " \t\r\n");

    printl(LOG_VERB, "External HTTP send RESPONSE: PROTO: [%s] STATUS: [%s], REASON: [%s]", proto, status, reason);

    if (strcmp(status, HTTP_RESPONSE_200)) {
        printl(LOG_INFO, "Non-succesful responce [%s] from the HTTP server", status);
        return 1;
    }

    return 0;
}
