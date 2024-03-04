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


/* ------------------------------------------------------------------------------------------------------------------ */
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

/* -- Useful macros ------------------------------------------------------------------------------------------------- */
/* Socket options: Keepalive, timeouts, ets */
#if !defined (WITH_TCP_NODELAY)
   #define WITH_TCP_NODELAY 1
#endif

#if (WITH_LIBSSH2)
    #include <libssh2.h>
#endif

#define TCP_KEEPIDLE_S  120         /* Wait 2 minutes in sec before sending keep_alives */
#define TCP_KEEPINTVL_S 30          /* Interval between keep_alives probes in seconds */
#define TCP_KEEPCNT_N   8           /* A number of probes before marking a session broken */

#if defined(__FreeBSD__)
    #define TCP_KEEPINIT_S  6           /* Timeout for a new not yet established connections in seconds */
#endif
#if defined(__APPLE__)
    #define TCP_CONNECTIONTIMEOUT_S 6   /* Timeout for a new not yet established connections in seconds */
#endif
#if defined(linux)
    #define TCP_SYNCNT_N    2           /* N of SYN retransmits should be sent before aborting the attempt to connect */
#endif

/* Used ports */
#define LISTEN_IPV4         "127.0.0.1"             /* We listen on this IPv4 address or */
#define LISTEN_IPV6         "::1"                   /* on this IPv6 address */
#define LISTEN_DEFAULT      LISTEN_IPV4

#define SOCKS_PORT          "1080"                  /* This is remote Socks server port */
#define SQUID_PORT          "3128"                  /* This is HTTPS proxy port */
#define SSH2_PORT           "22"                    /* This is SSH2 proxy port */

#define LISTEN_SOCKS_PORT   "7080"                  /* Our internal TCP Socks port */
#define LISTEN_HTTP_PORT    "8080"                  /* Our internal HTTP server port */
#define LISTEN_TRANS_PORT   "10800"                 /* Our transparent port */

#define INET_ADDRPORTSTRLEN INET6_ADDRSTRLEN + 6    /* MAX: ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff + ':' + '65535' */

/* -- Socket conversion macros -------------------------------------------------------------------------------------- */
#define SA_FAMILY(sa)  ((struct sockaddr *)&sa)->sa_family

/* sockaddr to sockaddr_in-> */
#define SIN4_ADDR(sa)   ((struct sockaddr_in *)&sa)->sin_addr
#define SIN4_PORT(sa)   ((struct sockaddr_in *)&sa)->sin_port
#define SIN4_FAMILY(sa) ((struct sockaddr_in *)&sa)->sin_family
#if !defined(linux)
    #define SIN4_LENGTH(sa) ((struct sockaddr_in *)&sa)->sin_len
#endif
#define S4_ADDR(sa)     ((struct sockaddr_in *)&sa)->sin_addr.s_addr

/* sockaddr to sockaddr_in6-> */
#define SIN6_ADDR(sa)   ((struct sockaddr_in6 *)&sa)->sin6_addr
#define SIN6_PORT(sa)   ((struct sockaddr_in6 *)&sa)->sin6_port
#define SIN6_FAMILY(sa) ((struct sockaddr_in6 *)&sa)->sin6_family
#if !defined(linux)
    #define SIN6_LENGTH(sa) ((struct sockaddr_in6 *)&sa)->sin6_len
#endif
#if defined(linux)
    #define S6_ADDR(sa) ((struct sockaddr_in6 *)&sa)->sin6_addr.__in6_u.__u6_addr8
#else
    #define S6_ADDR(sa) ((struct sockaddr_in6 *)&sa)->sin6_addr.__u6_addr.__u6_addr8
#endif

#define SIN_PORT(sa)    SA_FAMILY(sa) == AF_INET ? SIN4_PORT(sa) : SIN6_PORT(sa)

#ifndef HOST_NAME_MAX
    #define HOST_NAME_MAX 255
#endif


/* ------------------------------------------------------------------------------------------------------------------ */
typedef struct uvaddr {
    struct sockaddr_storage ip_addr;
    char name[HOST_NAME_MAX];
} uvaddr;

#define CHS_CHANNEL     0
#define CHS_SOCKET      1

typedef struct chs {                                            /* Channel / Socket structure */
    #if (WITH_LIBSSH2)
        LIBSSH2_CHANNEL *c;                                     /* libssh2 channel */
    #endif
    int s;                                                      /* socket */
    char t;                                                     /* type CHS_CHANNEL|CHS_SOCKET */
} chs;

#define CHS(cs)     cs.t ? (void *)(&cs.s) : (void *)cs.c       /* Return socket or SSH2 channel */


/* -- Function prototypes ------------------------------------------------------------------------------------------- */
int connect_desnation(struct sockaddr dest);
char *inet2str(struct sockaddr_storage *ai_addr, char *str_addr);
struct sockaddr_storage str2inet(char *str_addr, char *str_port);
