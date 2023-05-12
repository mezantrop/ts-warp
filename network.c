/* ------------------------------------------------------------------------------------------------------------------ */
/* TS-Warp - Transparent SOCKS proxy Wrapper                                                                          */
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


/* -- Network functions --------------------------------------------------------------------------------------------- */
#include <stdlib.h>
#include <string.h>

#include "network.h"
#include "logfile.h"
#include "pidfile.h"
#include "utility.h"

 
/* ------------------------------------------------------------------------------------------------------------------ */
int connect_desnation(struct sockaddr dest) {
    /* Establish TCP connetion with a det address */

    int sock;

    if ((sock = socket(dest.sa_family, SOCK_STREAM, 0)) < 0) {
        printl(LOG_CRIT, "Error creating a socket for the destination address");
        return sock;
    }

    #if (WITH_TCP_NODELAY)
        int tpc_ndelay = 1;
        if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &tpc_ndelay, sizeof(int)) == -1)
            printl(LOG_WARN, "Error setting TCP_NODELAY socket option for outgoing connections");
    #endif

    int keepalive = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(int)) == -1)
        printl(LOG_WARN, "Error setting SO_KEEPALIVE socket option for outgoing connections");

    #if !defined(__APPLE__) && !defined(__OpenBSD__)
        int keepidle = TCP_KEEPIDLE_S;
        if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepidle, sizeof(int)))
            printl(LOG_WARN, "Error setting TCP_KEEPIDLE socket option for outgoing connections");
    #endif

    /* Timeout for a new not yet established connections on FreeBSD/Darwin, 
    or a number of retries on Linux, or nothing on OpenBSD */
    #if defined(__FreeBSD__)
        int keepinit = TCP_KEEPINIT_S;
        if (setsockopt(sock, IPPROTO_TCP, TCP_KEEPINIT, &keepinit, sizeof(int)) == -1)
            printl(LOG_WARN, "Error setting TCP_KEEPINIT socket option for outgoing connections");
    #endif

    #if defined(__APPLE__)
        int keepinit = TCP_CONNECTIONTIMEOUT_S;
        if (setsockopt(sock, IPPROTO_TCP, TCP_CONNECTIONTIMEOUT, &keepinit, sizeof(int)) == -1)
            printl(LOG_WARN, "Error setting TCP_CONNECTIONTIMEOUT socket option for outgoing connections");
    #endif

    #if defined(linux)
        int syncnt = TCP_SYNCNT_N;
        if (setsockopt(sock, IPPROTO_TCP, TCP_SYNCNT, &syncnt, sizeof(int)) == -1)
            printl(LOG_WARN, "Error setting TCP_SYNCNT socket option for outgoing connections");
    #endif

    printl(LOG_VERB, "Socket to connect with destination address created");

    if ((connect(sock, &dest, sizeof dest)) < 0) {
        printl(LOG_CRIT, "Unable to connect with destination address");
        return -1;
    }

    return sock;
}

/* ------------------------------------------------------------------------------------------------------------------ */
char *inet2str(struct sockaddr_storage *ai_addr, char *str_addr) {
    /* inet_ntop() wrapper. If str_add is NULL, memory is auto-allocated,
     don't forget to free it after usage! */

    char buf[INET_ADDRPORTSTRLEN];

    if (!str_addr) str_addr = (char *)malloc(INET_ADDRPORTSTRLEN);

    memset(str_addr, 0, INET_ADDRPORTSTRLEN);
    memset(&buf, 0, INET_ADDRPORTSTRLEN);

    switch (ai_addr->ss_family) {
        case AF_INET:
            inet_ntop(AF_INET, &SIN4_ADDR(*ai_addr), buf, INET_ADDRSTRLEN);
            sprintf(str_addr, "%s:%d", buf, ntohs(SIN4_PORT(*ai_addr)));
            break;

        case AF_INET6:
            inet_ntop(AF_INET6, &SIN6_ADDR(*ai_addr), buf, INET6_ADDRSTRLEN);
            sprintf(str_addr, "%s:%d", buf, ntohs(SIN6_PORT(*ai_addr)));
            break;

        default:
            printl(LOG_WARN, "Unrecognized address family: %d", ai_addr->ss_family);
            return NULL;
    }
    return str_addr;
}

/* ------------------------------------------------------------------------------------------------------------------ */
struct sockaddr_storage str2inet(char *str_addr, char *str_port) {
    struct addrinfo hints, *res = NULL;
    struct sockaddr_storage a_ret;
    int ret;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((ret = getaddrinfo(str_addr, str_port, &hints, &res)) > 0) {
        printl(LOG_CRIT, "Error resolving address [%s]:[%s]: [%s]", str_addr, str_port, gai_strerror(ret));
        /* Return INADDR_NONE when failing to resolve */
        memset(&a_ret, 0, sizeof(struct sockaddr_storage));
        SA_FAMILY(a_ret) = AF_INET;
        S4_ADDR(a_ret) = INADDR_NONE;
    } else
        a_ret = *(struct sockaddr_storage *)res->ai_addr;

    freeaddrinfo(res);
    return a_ret;
}
