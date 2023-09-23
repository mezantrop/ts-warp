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


/* -- HTTP proxy (CONNECT method) implementation -------------------------------------------------------------------- */
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>

#include "utility.h"
#include "network.h"
#include "base64.h"
#include "http.h"
#include "logfile.h"


extern char *pfile_name;


/* ------------------------------------------------------------------------------------------------------------------ */
int http_server_request(int socket, struct uvaddr *daddr) {
    char buf[64 * BUF_SIZE_1KB];
    int rcount;
    char *method = NULL, *url = NULL, *proto = NULL;
    char host[HOST_NAME_MAX] = {0};
    uint16_t port = 80;


    if ((rcount = recv(socket, &buf, sizeof buf, 0)) == -1) {
        /* Quit immediately; no reply to the client */
        printl(LOG_WARN, "Unable to receive a request from the HTTP client");
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
    if (send(socket, HTTP_PROXY_REPLY_200, sizeof(HTTP_PROXY_REPLY_200) -1, 0) == -1) {
        printl(LOG_CRIT, "Unable to send reply to the HTTP client");
        return 1;
    }


    printl(LOG_VERB, "HTTP REQUEST: URL: [%s] METHOD: [%s], HOST: [%s], PORT: [%hu], PROTO: [%s]",
        url, method, host, port, proto);


    return 0;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int http_client_request(int socket, struct sockaddr_storage *daddr, char *user, char *password) {
    char r[BUF_SIZE_1KB] = {0};
    char b[HOST_NAME_MAX] = {0};
    char usr_pwd_plain[BUF_SIZE_1KB] = {0};
    char *usr_pwd_base64;
    char *proto = NULL, *status = NULL, *reason = NULL;
    int rcount = 0;


    /* Request startline: CONNECT address:port PROTOCOL */
    if (user && password) {
        sprintf(usr_pwd_plain, "%s:%s", user, password);
        base64_strenc(&usr_pwd_base64, usr_pwd_plain);
        sprintf(r, "%s %s %s\r\n%s %s\r\n\r\n",
            HTTP_REQUEST_METHOD_CONNECT, inet2str(daddr, b), HTTP_REQEST_PROTOCOL,
            HTTP_HEADER_PROXYAUTH_BASIC, usr_pwd_base64);
    } else
        sprintf(r, "%s %s %s\r\n\r\n", HTTP_REQUEST_METHOD_CONNECT, inet2str(daddr, b), HTTP_REQEST_PROTOCOL);

    printl(LOG_VERB, "Sending HTTP %s request", HTTP_REQUEST_METHOD_CONNECT);

    if (send(socket, r, strlen(r), 0) == -1) {
        printl(LOG_CRIT, "Unable to send a request to the HTTP server");
        mexit(1, pfile_name, tfile_name);
    }

    printl(LOG_VERB, "Expecting HTTP reply");

    if ((rcount = recv(socket, &r, sizeof(r), 0)) == -1) {
        printl(LOG_CRIT, "Unable to receive a reply from the HTTP server");
        mexit(1, pfile_name, tfile_name);
    }

    /* Parse HTTP reply */
    r[rcount] = '\0';
    proto = strtok(r,  " \t\r\n");
    status = strtok(NULL, " \t\r\n");
    reason = strtok(NULL, " \t\r\n");

    printl(LOG_VERB, "HTTP RESPONSE: PROTO: [%s] STATUS: [%s], REASON: [%s]", proto, status, reason);

    if (strcmp(status, HTTP_RESPONSE_200)) {
        printl(LOG_INFO, "Non-succesful responce [%s] from the HTTP server", status);
        return 1;
    }

    return 0;
}
