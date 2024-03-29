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


/* -- Socks protocol implementation --------------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "utility.h"
#include "network.h"
#include "socks.h"
#include "logfile.h"
#include "version.h"


extern char *pfile_name;

const char *socks4_status[] = {
    "OK",
    "Request rejected or failed",
    "Request failed because client is not running identd",
    "Request failed because client's identd couldn't confirm user"
};

const char *socks5_status[] = {
    "OK",
    "General failure",
    "Connection not allowed by ruleset",
    "Network unreachable",
    "Host unreachable",
    "Connection refused by target host",
    "TTL expired",
    "Command unsupported / protocol error",
    "Address type is not supported"
};

/* -- Socks client functions ---------------------------------------------------------------------------------------- */
int socks4_client_request(chs cs, uint8_t cmd, struct sockaddr_in *daddr, char *user) {
    /* Send Socks4 request */

    s4_request req;
    s4_reply rep;
    int idlen = 0, rcount = 0;

    printl(LOG_CRIT, "Preparing IPv4 Socks4 request");

    /* Fill in the request */
    strcpy((char*)req.id, PROG_NAME);                       /* Sic! Some username is required by server! */
    idlen = strnlen((char *)req.id, STR_SIZE);

    req.ver = PROXY_PROTO_SOCKS_V4 - '0';
    req.cmd = cmd;
    req.dstaddr = S4_ADDR(*daddr);
    req.dstport = SIN4_PORT(*daddr);
    if (user != NULL) {
        idlen = strnlen(user, STR_SIZE);
        strncpy((char *)req.id, user, idlen);
    }

    printl(LOG_VERB, "Sending IPv4 Socks4 request");

    switch (cs.t) {
        case CHS_SOCKET:
            if (send(cs.s, &req, 8 + idlen + 1, 0) == -1) {
                printl(LOG_CRIT, "Unable to send a request to the Socks4 server via socket");
                return SOCKS4_REPLY_KO;
            }

            printl(LOG_VERB, "IPv4 Socks4 request sent");

            if ((rcount = recv(cs.s, &rep, sizeof(s4_reply), 0)) == -1) {
                printl(LOG_CRIT, "Unable to receive a reply from the Socks4 server via socket");
                return SOCKS4_REPLY_KO;
            }
        break;

        case CHS_CHANNEL:
            #if (WITH_LIBSSH2)
                if (libssh2_channel_write(cs.c, (char*)&req, 8 + idlen + 1) < 0) {
                    printl(LOG_CRIT, "Unable to send a request to the Socks4 server via SSH2 channel");
                    return SOCKS4_REPLY_KO;
                }

                while ((rcount = libssh2_channel_read(cs.c, (char*)&rep, sizeof(s4_reply))) == LIBSSH2_ERROR_EAGAIN) ;
                if (rcount < 0) {
                    printl(LOG_CRIT, "Unable to receive a reply from the Socks4 server via SSH2 channel");
                    return SOCKS4_REPLY_KO;
                }
            #endif
        break;

        default:
            printl(LOG_CRIT, "Error Socket / SSH2 Channel specified");
            return SOCKS4_REPLY_KO;
        break;
    }

    printl(LOG_VERB, "Socks4 reply: [%d][%d]:[%s], Bytes [%d]",
        rep.nul, rep.status, socks4_status[rep.status - 0x5a], rcount);

    if (rep.nul != 0) {                                             /* Reply Socks4 Request rejected or failed */
        printl(LOG_CRIT, "Socks4 server speaks unsupported protocol v:[%d]", rep.nul);
        return SOCKS4_REPLY_KO;
    }

    printl(LOG_VERB, "Socks4 server reply status: [%d]:[%s]", rep.status, socks4_status[rep.status - 0x5a]);

    return rep.status;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int socks5_client_hello(chs cs, unsigned int auth_method, ...) {
    /* Send Socks5 Hello and receive a reply.

    Specify one mandatory auth_method, list the rest of auth methods as variadic arguments. Make sure the last of them
    is always AUTH_METHOD_NOACCEPT !!! */

    s5_request_hello req;
    s5_reply_hello rep;
    va_list ap;
    unsigned int am = 0;
    #if (WITH_LIBSSH2)
        int rcount = 0;
    #endif


    if (auth_method == AUTH_METHOD_NOACCEPT) {
        printl(LOG_CRIT, "socks5_client_hello(): auth_method must not be AUTH_METHOD_NOACCEPT");
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
    req.ver = PROXY_PROTO_SOCKS_V5 - '0';
    req.nauth = am;

    switch (cs.t) {
        case CHS_SOCKET:
            /* Send 'hello' request */
            if (send(cs.s, &req, am + sizeof(req.ver) + sizeof(req.nauth), 0) == -1) {
                printl(LOG_CRIT, "Unable to send 'hello' request to the Socks5 server via socket");
                return AUTH_METHOD_NOACCEPT;
            }

            /* Receive 'hello' response from Socks5 server */
            if ((recv(cs.s, &rep, sizeof(rep), 0)) == -1) {
                printl(LOG_CRIT, "Unable to receive 'hello' reply from the Socks5 server via socket");
                return AUTH_METHOD_NOACCEPT;
            }
        break;

        case CHS_CHANNEL:
            #if (WITH_LIBSSH2)
                if (libssh2_channel_write(cs.c, (char*)&req, am + sizeof(req.ver) + sizeof(req.nauth)) < 0) {
                    printl(LOG_CRIT, "Unable to send 'hello' request to the Socks5 server via SSH2 channel");
                    return AUTH_METHOD_NOACCEPT;
                }

                while ((rcount = libssh2_channel_read(cs.c, (char*)&rep, sizeof(rep))) == LIBSSH2_ERROR_EAGAIN) ;
                if (rcount < 0) {
                    printl(LOG_CRIT, "Unable to receive 'hello' reply from the Socks5 server via SSH2 channel");
                    return AUTH_METHOD_NOACCEPT;
                }
            #endif
        break;

        default:
            printl(LOG_CRIT, "Error Socket / SSH2 Channel specified");
            return AUTH_METHOD_NOACCEPT;
        break;
    }

    /* Veryfy Socks version */
    if (rep.ver != PROXY_PROTO_SOCKS_V5 - '0') {
        printl(LOG_CRIT, "Socks5 server unsupported protocol: v[%d]", rep.ver);
        return AUTH_METHOD_NOACCEPT;
    }

    printl(LOG_VERB, "Socks5 server accepted auth-method: [%d]", rep.cauth);
    return rep.cauth;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int socks5_client_auth(chs cs, char *user, char *password) {
    char buf[513];                                                      /* Max Socks5 auth request size */
    int idlen = 0, pwlen = 0;
    int rcount = 0;
    s5_reply_auth *rep = NULL;

    idlen = strnlen(user, 255);
    pwlen = strnlen(password, 255);

    /* Fill into the auth request structure */
    buf[0] = 1;
    buf[1] = idlen;
    memcpy(buf + 2, user, idlen);
    buf[2 + idlen] = pwlen;
    memcpy(buf + 2 + idlen + 1, password, pwlen);


    switch (cs.t) {
        case CHS_SOCKET:
            if (send(cs.s, buf, 2 + idlen + 1 + pwlen, 0) == -1) {
                printl(LOG_CRIT, "Unable to send an auth request to the Socks5 server via socket");
                return SOCKS5_REPLY_KO;
            }

            memset(buf, 0, sizeof buf);
            if ((rcount = recv(cs.s, &buf, sizeof(buf), 0)) == -1) {
                printl(LOG_CRIT, "Unable to receive a reply from the Socks5 server via socket");
                return SOCKS5_REPLY_KO;
            }
        break;

        case CHS_CHANNEL:
            #if (WITH_LIBSSH2)
                if (libssh2_channel_write(cs.c, (char*)&buf, 2 + idlen + 1 + pwlen) < 0) {
                    printl(LOG_CRIT, "Unable to send auth request to the Socks5 server via SSH2 channel");
                    return SOCKS5_REPLY_KO;
                }

                while ((rcount = libssh2_channel_read(cs.c, (char*)&buf, sizeof(buf))) == LIBSSH2_ERROR_EAGAIN) ;
                if (rcount < 0) {
                    printl(LOG_CRIT, "Unable to receive auth reply from the Socks5 server via SSH2 channel");
                    return SOCKS5_REPLY_KO;
                }
            #endif
        break;

        default:
            printl(LOG_CRIT, "Error Socket / SSH2 Channel specified");
            return SOCKS5_REPLY_KO;
        break;
    }

    rep = (s5_reply_auth *)buf;
    printl(LOG_VERB, "Socks5 reply: [%d][%d]:[%s], Bytes [%d]",
        rep->ver, rep->status, !rep->status ? "OK" : "KO", rcount);

    return rep->status;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int socks5_client_request(chs cs, uint8_t cmd, struct sockaddr_storage *daddr, char *dname) {
    /* Perform Socks5 request; IPv4/IPv6 addresses: atype 1/4, Domain name: atype 3 */

    s5_reply_short *rep;
    char buf[sizeof(s5_reply)];                                   /* Max Socks5 reply size */
    int rcount = 0;
    uint8_t atype = SOCKS5_ATYPE_NONE;
    int atype_len = SOCKS5_ATYPE_NAME_LEN;


    if (dname && dname[0])
        atype = SOCKS5_ATYPE_NAME;
    else
        if (SA_FAMILY(*daddr) == AF_INET)
            atype = SOCKS5_ATYPE_IPV4;
        else
            if (SA_FAMILY(*daddr) == AF_INET6)
                atype = SOCKS5_ATYPE_IPV6;

    s5_request_short *req = (s5_request_short *)buf;
    req->ver = PROXY_PROTO_SOCKS_V5 - '0';
    req->cmd = cmd;
    req->rsv = 0x0;
    req->atype = atype;

    if (atype == SOCKS5_ATYPE_IPV4) {
        atype_len = SOCKS5_ATYPE_IPV4_LEN;

        printl(LOG_VERB, "Preparing IPv4 Socks5 request");

        s5_request_ipv4 *req = (s5_request_ipv4 *)buf;
        memcpy(req->dstaddr, &SIN4_ADDR(*daddr), sizeof req->dstaddr);
        req->dstport = SIN4_PORT(*daddr);

        printl(LOG_VERB, "Sending IPv4 Socks5 request");

        switch (cs.t) {
            case CHS_SOCKET:
                if (send(cs.s, req, sizeof(s5_request_ipv4), 0) == -1) {
                    printl(LOG_CRIT, "Unable to send a request to the Socks5 server via socket");
                    return SOCKS5_REPLY_KO;
                }
            break;

            case CHS_CHANNEL:
                #if (WITH_LIBSSH2)
                    if (libssh2_channel_write(cs.c, (char *)req, sizeof(s5_request_ipv4)) < 0) {
                        printl(LOG_CRIT, "Unable to send auth request to the Socks5 server via SSH2 channel");
                        return SOCKS5_REPLY_KO;
                    }
                #endif
            break;

            default:
                printl(LOG_CRIT, "Error Socket / SSH2 Channel specified");
                return SOCKS5_REPLY_KO;
            break;
        }

        printl(LOG_VERB, "IPv4 Socks5 request sent");

    } else if (atype == SOCKS5_ATYPE_IPV6) {
        atype_len = SOCKS5_ATYPE_IPV6_LEN;
        printl(LOG_VERB, "Preparing IPv6 Socks5 request");

        s5_request_ipv6 *req = (s5_request_ipv6 *)buf;
        memcpy(req->dstaddr, &SIN6_ADDR(*daddr), sizeof req->dstaddr);
        req->dstport = SIN6_PORT(*daddr);

        printl(LOG_VERB, "Sending IPv6 Socks5 request");

        switch (cs.t) {
            case CHS_SOCKET:
                if (send(cs.s, req, 4 + sizeof(s5_request_ipv6), 0) == -1) {
                    printl(LOG_CRIT, "Unable to send a request to the Socks5 server via socket");
                    return SOCKS5_REPLY_KO;
                }
            break;

            case CHS_CHANNEL:
                #if (WITH_LIBSSH2)
                    if (libssh2_channel_write(cs.c, (char*)&req, 4 + sizeof(s5_request_ipv6)) < 0) {
                        printl(LOG_CRIT, "Unable to send a request to the Socks5 server via SSH2 channel");
                        return SOCKS5_REPLY_KO;
                    }
                #endif
            break;

            default:
                printl(LOG_CRIT, "Error Socket / SSH2 Channel specified");
                return SOCKS5_REPLY_KO;
            break;
        }

        printl(LOG_VERB, "IPv6 Socks5 request sent");

    } else if (atype == SOCKS5_ATYPE_NAME) {
        printl(LOG_VERB, "Preparing NAME Socks5 request");

        atype_len = strlen(dname);

        printl(LOG_VERB, "The name in the NAME Socks5 request: [%s]", dname);

        char *name = (char *)req + sizeof(s5_request_short);
        *name++ = atype_len;
        memcpy(name, dname, atype_len);
        name += atype_len;
        *(in_port_t *)name = SA_FAMILY(*daddr) == AF_INET ? (in_port_t)SIN4_PORT(*daddr) : (in_port_t)SIN6_PORT(*daddr);

        printl(LOG_VERB, "Sending NAME Socks5 request");

        switch (cs.t) {
            case CHS_SOCKET:
                if (send(cs.s, req,  sizeof(s5_request_short) + 1 + atype_len + 2,  0) == -1) {
                    printl(LOG_CRIT, "Unable to send a request to the Socks5 server via socket");
                    return SOCKS5_REPLY_KO;
                }
            break;

            case CHS_CHANNEL:
                #if (WITH_LIBSSH2)
                    if (libssh2_channel_write(cs.c, (char*)&req, sizeof(s5_request_short) + 1 + atype_len + 2) < 0) {
                        printl(LOG_CRIT, "Unable to send a request to the Socks5 server via SSH2 channel");
                        return SOCKS5_REPLY_KO;
                    }
                #endif
            break;

            default:
                printl(LOG_CRIT, "Error Socket / SSH2 Channel specified");
                return SOCKS5_REPLY_KO;
            break;
        }

        printl(LOG_VERB, "NAME Socks5 request sent");

    } else {
        printl(LOG_CRIT, "Unsupported address types: [%d] is in the request", daddr->ss_family);
        return SOCKS5_REPLY_KO;
    }

    printl(LOG_VERB, "Expecting Socks5 server reply");

    /* Receive reply from the server */
    memset(buf, 0, sizeof(buf));

    switch (cs.t) {
        case CHS_SOCKET:
            /* 6 + atype_len: 6 is a SOCK5 header length - variable address field */
            if ((rcount = recv(cs.s, &buf, 6 + atype_len, 0)) == -1) {
                printl(LOG_CRIT, "Unable to receive a reply from the Socks5 server via socket");
                return SOCKS5_REPLY_KO;
            }
        break;

        case CHS_CHANNEL:
            #if (WITH_LIBSSH2)
                while ((rcount = libssh2_channel_read(cs.c, (char*)&buf, 6 + atype_len)) == LIBSSH2_ERROR_EAGAIN) ;
                if (rcount < 0) {
                    printl(LOG_CRIT, "Unable to receive a reply from the Socks5 server via SSH2 channel");
                    return SOCKS5_REPLY_KO;
                }
            #endif
        break;

        default:
            printl(LOG_CRIT, "Error Socket / SSH2 Channel specified");
            return SOCKS5_REPLY_KO;
        break;
    }

    rep = (s5_reply_short *)buf;
    printl(LOG_VERB, "Socks5 reply: [%d][%d]:[%s], Bytes [%d]", rep->ver,
        rep->status, socks5_status[rep->status], rcount);

    if (rep->ver != PROXY_PROTO_SOCKS_V5 - '0') {                             /* Report Socks5 general failure */
        printl(LOG_WARN, "Socks5 server speaks unsupported protocol v:[%d]", rep->ver);
        return SOCKS5_REPLY_KO;
    }

    printl(LOG_VERB, "Socks5 server reply status: [%d]:[%s]", rep->status, socks5_status[rep->status]);
    return rep->status;
}

/* --Socks server part ---------------------------------------------------------------------------------------------- */
int socks5_server_hello(int socket) {
    /* Parse client's 'hello' request and send reply; Return AUTH_METHOD_NOAUTH if OK or AUTH_METHOD_NOACCEPT if NOK */

    s5_request_hello req;
    s5_reply_hello rep;
    uint8_t na = 0;

    rep.ver = PROXY_PROTO_SOCKS_V5 - '0';
    rep.cauth = AUTH_METHOD_NOACCEPT;

    /* Receive 'hello' request from Socks-client */
    if ((recv(socket, &req, sizeof req, 0)) == -1) {
        printl(LOG_CRIT, "Unable to receive 'hello' reques from the Socks5 client");
        /* Quit function immediately; no reply back */
        return AUTH_METHOD_NOACCEPT;
    }

    if (req.ver != PROXY_PROTO_SOCKS_V5 - '0')
        printl(LOG_WARN, "Unsupported version: [%i] in the request", req.ver);
    else
        for (na = 0; na < req.nauth; na++)
            if (req.auth[na] == AUTH_METHOD_NOAUTH) {
                printl(LOG_VERB, "Selected Socks5 auth method number: [%i] - [%i]", na, AUTH_METHOD_NOAUTH);
                rep.cauth = AUTH_METHOD_NOAUTH;
                break;
            }

    /* Send 'hello' reply */
    if (send(socket, &rep, sizeof rep, 0) == -1) {
        printl(LOG_CRIT, "Unable to send 'hello' reply to the Socks5 server");
        return AUTH_METHOD_NOACCEPT;
    }

    return rep.cauth;
}

/* ------------------------------------------------------------------------------------------------------------------ */
uint8_t socks5_server_request(int socket, struct uvaddr *daddr) {
    s5_request *req;
    s5_request_ipv4 *req4;
    s5_request_ipv6 *req6;

    char buf[sizeof(s5_request)];                         /* Max Socks5 request size */
    int rcount;
    uint8_t atype = SOCKS5_ATYPE_IPV4;


    if ((rcount = recv(socket, &buf, sizeof buf, 0)) == -1 || rcount < sizeof(s5_request_short)) {
        /* Quit immediately; no reply to the client */
        printl(LOG_WARN, "Unable to receive a request from the Socks5 client");
        return SOCKS5_ATYPE_NONE;
    }

    /* Validate request */
    req = (s5_request *)buf;
    if (req->ver != PROXY_PROTO_SOCKS_V5 - '0') {
        printl(LOG_WARN, "Client speaks unsupported protocol version: [%i]", req->ver);
        return SOCKS5_ATYPE_NONE;
    }

    switch (req->atype) {
        case SOCKS5_ATYPE_IPV4:
            req4 = (s5_request_ipv4 *)buf;
            SA_FAMILY(daddr->ip_addr) = AF_INET;
            memcpy(&SIN4_ADDR(daddr->ip_addr), req4->dstaddr, sizeof(struct in_addr));
            SIN4_PORT(daddr->ip_addr) = req4->dstport;
            atype = SOCKS5_ATYPE_IPV4;
        break;

        case SOCKS5_ATYPE_NAME:
            memcpy(&daddr->name, req->dsthost + 1, req->dsthost[0]);
            daddr->ip_addr = str2inet(daddr->name, NULL);
            if (SA_FAMILY(daddr->ip_addr) == AF_INET)
                SIN4_PORT(daddr->ip_addr) = (req->dsthost[2 + req->dsthost[0]] << 8) +
                    req->dsthost[1 + req->dsthost[0]];
            else
                SIN6_PORT(daddr->ip_addr) = (req->dsthost[2 + req->dsthost[0]] << 8) +
                    req->dsthost[1 + req->dsthost[0]];
            atype = SOCKS5_ATYPE_NAME;
        break;

        case SOCKS5_ATYPE_IPV6:
            req6 = (s5_request_ipv6 *)buf;
            SA_FAMILY(daddr->ip_addr) = AF_INET6;
            memcpy(&SIN6_ADDR(daddr->ip_addr), req6->dstaddr, sizeof(struct in6_addr));
            SIN6_PORT(daddr->ip_addr) = req6->dstport;
            atype = SOCKS5_ATYPE_IPV6;
        break;
    }

    return atype;
}

/* ------------------------------------------------------------------------------------------------------------------ */
uint8_t socks5_server_reply(int socket, struct sockaddr_storage *iaddr, uint8_t status) {
    /* Send Socks5 reply back to the client */
    char buf[sizeof(s5_reply)] = {0};                         /* Max Socks5 reply size */

    s5_reply_ipv4 *rep4;
    s5_reply_ipv6 *rep6;

    if  (SA_FAMILY(*iaddr) == AF_INET) {
        rep4 = (s5_reply_ipv4 *)buf;
        rep4->ver = PROXY_PROTO_SOCKS_V5 - '0';
        rep4->status = status;
        rep4->rsv = 0;
        rep4->atype = SOCKS5_ATYPE_IPV4;
        memcpy(rep4->dstaddr, &SIN4_ADDR(*iaddr), sizeof(rep4->dstaddr));
        rep4->dstport = SIN4_PORT(*iaddr);

        if (send(socket, &buf, sizeof(s5_reply_ipv4), 0) == -1) {
            printl(LOG_CRIT, "Unable to send IPv4 reply to the Socks5 client");
            return 1;
        }

    } else {
        rep6 = (s5_reply_ipv6 *)buf;
        rep6->ver = PROXY_PROTO_SOCKS_V5 - '0';
        rep6->status = status;
        rep6->rsv = 0;
        rep6->atype = SOCKS5_ATYPE_IPV4;
        memcpy(rep6->dstaddr, &SIN6_ADDR(*iaddr), sizeof(rep6->dstaddr));
        rep6->dstport = SIN6_PORT(*iaddr);

        if (send(socket, &buf, sizeof(rep6), 0) == -1) {
            printl(LOG_CRIT, "Unable to send IPv6 reply to the Socks5 client");
            return 1;
        }
   }

    return 0;
}
