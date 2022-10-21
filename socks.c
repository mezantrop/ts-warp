/* ------------------------------------------------------------------------------------------------------------------ */
/* TS-Warp - Transparent SOCKS protocol Wrapper                                                                       */
/* ------------------------------------------------------------------------------------------------------------------ */

/* Copyright (c) 2021, 2022, Mikhail Zakharov <zmey20000@yahoo.com>

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the 
following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following 
   disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following 
   disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */


/* -- SOCKS protocol implementation --------------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "network.h"
#include "socks.h"
#include "logfile.h"
#include "utility.h"
#include "version.h"

extern char *pfile_name;


/* ------------------------------------------------------------------------------------------------------------------ */
int socks4_request(int socket, uint8_t cmd, struct sockaddr_in *daddr, char *user) {
    /* Send SOCKS4 request */

    s4_request req;
    s4_reply rep;
    int idlen = 0, rcount = 0;
  
    printl(LOG_CRIT, "Preparing IPv4 SOCKS4 request");
    
    /* Fill in the request */
    strcpy((char*)req.id, PROG_NAME);                       /* Sic! Some username is required by server! */
    idlen = strnlen((char *)req.id, STR_SIZE);

    req.ver = PROXY_PROTO_SOCKS_V4;
    req.cmd = cmd;
    req.dstaddr = S4_ADDR(*daddr);
    req.dstport = SIN4_PORT(*daddr);
    if (user != NULL) {
        idlen = strnlen(user, STR_SIZE);
        strncpy((char *)req.id, user, idlen);
    }

    printl(LOG_VERB, "Sending IPv4 SOCKS4 request");

    if (send(socket, &req, 8 + idlen + 1, 0) == -1) {
        printl(LOG_CRIT, "Unable to send a request to the SOCKS4 server");
        mexit(1, pfile_name);
    }

    printl(LOG_VERB, "IPv4 SOCKS4 request sent");

    if ((rcount = recv(socket, &rep, sizeof(s4_reply), 0)) == -1) {
        printl(LOG_CRIT, "Unable to receive a reply from the SOCKS4 server");
        mexit(1, pfile_name);
    }

    printl(LOG_VERB, "SOCKS4 reply: [%d][%d], Bytes [%d]", rep.nul, rep.status, rcount);

    if (rep.nul != 0) {                                             /* Reply SOCKS4 Request rejected or failed */
        printl(LOG_CRIT, "SOCKS4 server speaks unsupported protocol v:[%d]", rep.nul);
        return SOCKS4_REPLY_KO;
    }
    
    printl(LOG_VERB, "SOCKS4 server reply status: %d", rep.status);
    
    return rep.status;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int socks5_hello(int socket, unsigned int auth_method, ...) {
    /* Send SOCKS5 Hello and receive a reply. 
    
    Specify one mandatory auth_method, list the rest of auth methods as variadic arguments. Make sure the last of them
    is always AUTH_METHOD_NOACCEPT !!! */

    s5_request_hello req;
    s5_reply_hello rep;
    va_list ap;
    unsigned int am = 0;

    if (auth_method == AUTH_METHOD_NOACCEPT) {
        printl(LOG_CRIT, "socks5_hello(): auth_method must not be AUTH_METHOD_NOACCEPT");
        return AUTH_METHOD_NOACCEPT;
    }

    /* Fill into auth-methods */
    req.auth[am] = auth_method;
    printl(LOG_VERB, "Auth method: [%u], number [%d]", req.auth[am], am);
    va_start(ap, (auth_method));
    while ((req.auth[++am] = va_arg(ap, unsigned int)) != AUTH_METHOD_NOACCEPT)
        printl(LOG_VERB, "Auth method: [%u], number [%d]", req.auth[am], am);
    va_end(ap);

    /* Fill the rest fields of 'hello' request structure */
    req.ver = PROXY_PROTO_SOCKS_V5;
    req.nauth = am;

    /* Send 'hello' request */
    if (send(socket, &req, am + sizeof req.ver + sizeof req.nauth, 0) == -1) {
        printl(LOG_CRIT, "Unable to send 'hello' request to the SOCKS5 server");
        return AUTH_METHOD_NOACCEPT;
    }

    /* Receive 'hello' response from SOCKS server */
    if ((recv(socket, &rep, sizeof rep, 0)) == -1) {
        printl(LOG_CRIT, "Unable to receive 'hello' reply from the SOCKS5 server");
        return AUTH_METHOD_NOACCEPT;
    }
    
    /* Veryfy Socks version */
    if (rep.ver != PROXY_PROTO_SOCKS_V5) {
        printl(LOG_CRIT, "SOCKS5 server unsupported protocol: v[%d]", rep.ver);
        return AUTH_METHOD_NOACCEPT;
    }
    
    printl(LOG_VERB, "SOCKS5 server accepted auth-method: [%d]", rep.cauth);
    return rep.cauth;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int socks5_auth(int socket, char *user, char *password) {
    char buf[513];                                                      /* Max SOCKS5 auth request size */
    int idlen = 0, pwlen = 0;
    int rcount;
    s5_reply_auth *rep = NULL;

    idlen = strnlen(user, 255);
    pwlen = strnlen(password, 255);

    /* Fill into the auth request structure */
    buf[0] = 1;
    buf[1] = idlen;
    memcpy(buf + 2, user, idlen);
    buf[2 + idlen] = pwlen;
    memcpy(buf + 2 + idlen + 1, password, pwlen);

    if (send(socket, buf, 2 + idlen + 1 + pwlen, 0) == -1) {
        printl(LOG_CRIT, "Unable to send an auth request to the SOCKS5 server");
        mexit(1, pfile_name);
    }

    memset(buf, 0, sizeof buf);
    if ((rcount = recv(socket, &buf, sizeof buf, 0)) == -1) {
        printl(LOG_CRIT, "Unable to receive a reply from the SOCKS5 server");
        mexit(1, pfile_name);
    }

    rep = (s5_reply_auth *)buf;
    printl(LOG_VERB, "SOCKS5 reply: [%d][%d], Bytes [%d]", rep->ver, rep->status, rcount);

    return rep->status;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int socks5_request(int socket, uint8_t cmd, uint8_t atype, struct sockaddr *daddr) {
    /* Perform SOCKS5 request; IPv4/IPv6 addresses: atype 1/4, Domain name: atype 3 */

    s5_reply_short *rep;
    char buf[261];                                                      /* Max SOCKS5 reply size */
    int rcount = 0;
    int atype_len = SOCKS5_ATYPE_NAME_LEN;

    s5_request_short *req = (s5_request_short *)buf;
    req->ver = PROXY_PROTO_SOCKS_V5;
    req->cmd = cmd;
    req->rsv = 0x0;
    req->atype = atype;

    if (atype == SOCKS5_ATYPE_IPV4) {
        atype_len = SOCKS5_ATYPE_IPV4_LEN;
        printl(LOG_VERB, "Preparing IPv4 SOCKS5 request");

        s5_request_ipv4 *req = (s5_request_ipv4 *)buf;
        memcpy(req->dstaddr, &SIN4_ADDR(*daddr), sizeof req->dstaddr);
        req->dstport = SIN4_PORT(*daddr);

        printl(LOG_VERB, "Sending IPv4 SOCKS5 request");

        if (send(socket, req, sizeof(s5_request_ipv4), 0) == -1) {
            printl(LOG_CRIT, "Unable to send a request to the SOCKS5 server");
            mexit(1, pfile_name);
        }

        printl(LOG_VERB, "IPv4 SOCKS5 request sent");
    
    } else if (atype == SOCKS5_ATYPE_IPV6) {
        atype_len = SOCKS5_ATYPE_IPV6_LEN;
        printl(LOG_VERB, "Preparing IPv6 SOCKS5 request");

        s5_request_ipv6 *req = (s5_request_ipv6 *)buf;
        memcpy(req->dstaddr, &SIN6_ADDR(*daddr), sizeof req->dstaddr);
        req->dstport = SIN6_PORT(*daddr);

        printl(LOG_VERB, "Sending IPv6 SOCKS5 request");

        if (send(socket, req, 4 + sizeof(s5_request_ipv6), 0) == -1) {
            printl(LOG_CRIT, "Unable to send a request to the SOCKS server");
            mexit(1, pfile_name);
        }

        printl(LOG_VERB, "IPv6 SOCKS5 request sent");
   
    } else if (atype == SOCKS5_ATYPE_NAME) {
        char daddr_ch[HOST_NAME_MAX];

        printl(LOG_VERB, "Preparing NAME SOCKS5 request");

        getnameinfo(daddr, sizeof(*daddr), daddr_ch, sizeof(daddr_ch), NULL, 0, 0);
        atype_len = strlen(daddr_ch);
     
        printl(LOG_VERB, "The name in the NAME SOCKS5 request: [%s]", daddr_ch);

        char *name = (char *)req + sizeof(s5_request_short);
        *name++ = atype_len;
        memcpy(name, daddr_ch, atype_len);
        name += atype_len;
        *(in_port_t *)name = SA_FAMILY(*daddr) == AF_INET ? (in_port_t)SIN4_PORT(*daddr) : (in_port_t)SIN6_PORT(*daddr);

        printl(LOG_VERB, "Sending NAME SOCKS5 request");

        if (send(socket, req,  sizeof(s5_request_short) + 1 + atype_len + 2,  0) == -1) {
            printl(LOG_CRIT, "Unable to send a request to the SOCKS server");
            mexit(1, pfile_name);
        }

        printl(LOG_VERB, "NAME SOCKS5 request sent");

    } else {
        printl(LOG_CRIT, "Unsupported address types: [%d] is in the request", daddr->sa_family);
        mexit(1, pfile_name);
    }

    printl(LOG_VERB, "Expecting SOCKS5 server reply");

    /* Receive reply from the server */
    memset(buf, 0, sizeof buf);

    /* 6 + atype_len: 6 is a SOCK5 header length - variable address field */
    if ((rcount = recv(socket, &buf, 6 + atype_len, 0)) == -1) {
        printl(LOG_CRIT, "Unable to receive a reply from the SOCKS5 server");
        mexit(1, pfile_name);
    }

    rep = (s5_reply_short *)buf;
    printl(LOG_VERB, "SOCKS5 reply: [%d][%d], Bytes [%d]", rep->ver, 
        rep->status, rcount);

    if (rep->ver != PROXY_PROTO_SOCKS_V5) {                             /* Report SOCKS5 general failure */
        printl(LOG_WARN, "SOCKS5 server speaks unsupported protocol v:[%d]", rep->ver);
        return SOCKS5_REPLY_KO;
    }
    
    printl(LOG_VERB, "SOCKS5 server reply status: %d", rep->status);
    return rep->status;
}
