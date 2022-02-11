/* TS-Warp - Transparent SOCKS protocol Wrapper

Copyright (c) 2021, 2022, Mikhail Zakharov <zmey20000@yahoo.com>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */


/* -- IP address NAT lookup implementation ---------------------------------- */
#if !defined(linux)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <err.h>
#include <errno.h>
#include <net/if.h>

#include "natlook.h"
#include "utils.h"

extern char *pfile_name;

/* -------------------------------------------------------------------------- */
struct sockaddr nat_lookup(struct sockaddr *caddr, struct sockaddr *iaddr) {
    struct sockaddr daddr;
    struct pfioc_natlook pfnl;
    int pfd;
    char dstr_addr[INET6_ADDRSTRLEN];

    if ((pfd = open(PF_DEV, O_RDWR)) == -1) {
        printl(LOG_CRIT, "Error openin PF device: [%s]", PF_DEV);
        mexit(1, pfile_name);
    }

	memset(&pfnl, 0, sizeof(struct pfioc_natlook));
	pfnl.direction = PF_OUT;
	pfnl.af = caddr->sa_family;
	pfnl.proto = IPPROTO_TCP;
    
    switch (pfnl.af) {
        case AF_INET:
            #if defined(__APPLE__)
            pfnl.saddr.v4addr = SIN4_ADDR(*caddr);
            pfnl.daddr.v4addr = SIN4_ADDR(*iaddr);
            pfnl.sxport.port = SIN4_PORT(*caddr);
            pfnl.dxport.port = SIN4_PORT(*iaddr);
            #else
            pfnl.saddr.v4 = SIN4_ADDR(*caddr);
            pfnl.daddr.v4 = SIN4_ADDR(*iaddr);
            pfnl.sport = SIN4_PORT(*caddr);
            pfnl.dport = SIN4_PORT(*iaddr);
            #endif

            inet_ntop(AF_INET, &SIN4_ADDR(caddr), dstr_addr, INET_ADDRSTRLEN);
            #if defined(__APPLE__)
            printl(LOG_VERB,"saddr: %s %d", dstr_addr, pfnl.sxport.port);
            #else
            printl(LOG_VERB,"saddr: %s %d", dstr_addr, pfnl.sport);
            #endif
            inet_ntop(AF_INET, &SIN4_ADDR(*iaddr), dstr_addr, INET_ADDRSTRLEN);
            #if defined(__APPLE__)                        
            printl(LOG_VERB,"daddr: %s %d", dstr_addr, pfnl.dxport.port);
            #else
            printl(LOG_VERB,"daddr: %s %d", dstr_addr, pfnl.dport);
            #endif

            break;

        case AF_INET6:
            #if defined(__APPLE__)
            pfnl.saddr.v6addr = SIN6_ADDR(*caddr);
            pfnl.daddr.v6addr = SIN6_ADDR(*iaddr);
            pfnl.sxport.port = SIN6_PORT(*caddr);
            pfnl.dxport.port = SIN6_PORT(*iaddr);
            #else
            pfnl.saddr.v6 = SIN6_ADDR(*caddr);
            pfnl.daddr.v6 = SIN6_ADDR(*iaddr);
            pfnl.sport = SIN6_PORT(*caddr);
            pfnl.dport = SIN6_PORT(*iaddr);
            #endif

            inet_ntop(AF_INET6, &SIN6_ADDR(caddr), dstr_addr, INET6_ADDRSTRLEN);
            #if defined(__APPLE__)
            printl(LOG_VERB,"saddr: %s %d", dstr_addr, pfnl.sxport.port);
            #else
            printl(LOG_VERB,"saddr: %s %d", dstr_addr, pfnl.sport);
            #endif
            inet_ntop(AF_INET6, &SIN6_ADDR(iaddr), dstr_addr, INET6_ADDRSTRLEN);
            #if defined(__APPLE__)
            printl(LOG_VERB,"daddr: %s %d", dstr_addr, pfnl.dxport.port);
            #else
            printl(LOG_VERB,"daddr: %s %d", dstr_addr, pfnl.dport);
            #endif

            break;

        default:
            printl(LOG_CRIT,
                "Unsupported Address family in nat_lookup() request: [%d]",
                pfnl.af);
            mexit(1, pfile_name);
    }

    if (ioctl(pfd, DIOCNATLOOK, &pfnl) == -1) {
        if (errno == ENOENT) {
            printl(LOG_CRIT, "Failed to query PF NAT about PF_OUT packets");
            pfnl.direction = PF_IN;
            if (ioctl(pfd, DIOCNATLOOK, &pfnl) != 0) {
                printl(LOG_CRIT, "Failed to query PF NAT about PF_IN packets");
                mexit(1, pfile_name);
            }
        } else {
            printl(LOG_CRIT, "Failed to query PF NAT ");
            mexit(1, pfile_name);
        }
    }

    switch (pfnl.af) {
        case AF_INET:
            #if defined(__APPLE__)
            SIN4_ADDR(daddr) = pfnl.rdaddr.v4addr;
            SIN4_PORT(daddr) = pfnl.rdxport.port;
            #else
            SIN4_ADDR(daddr) = pfnl.rdaddr.v4;
            SIN4_PORT(daddr) = pfnl.rdport;
            #endif
            SIN4_FAMILY(daddr) = pfnl.af;
            SIN4_LENGTH(daddr) = sizeof(struct sockaddr_in);

            printl(LOG_VERB, "Real destination IP address: [%s]",
                /* TODO: rewrite as inet2str() */
                inet_ntop(AF_INET, &SIN4_ADDR(daddr), dstr_addr,
                INET_ADDRSTRLEN));
            break;
        
        case AF_INET6:
            #if defined(__APPLE__)
            SIN6_ADDR(daddr) = pfnl.rdaddr.v6addr;
            SIN6_PORT(daddr) = pfnl.rdxport.port;
            #else
            SIN6_ADDR(daddr) = pfnl.rdaddr.v6;
            SIN6_PORT(daddr) = pfnl.rdport;
            #endif
            SIN6_FAMILY(daddr) = pfnl.af;
            SIN6_LENGTH(daddr) = sizeof(struct sockaddr_in6);

            printl(LOG_VERB, "Real destination IP address: [%s]", 
                /* TODO: rewrite as inet2str() */
                inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&daddr)->sin6_addr,
                    dstr_addr, INET6_ADDRSTRLEN));
            break;

        default:
            printl(LOG_CRIT,
                "Unsupported Address family in nat_lookup() response: [%d]",
                pfnl.af);
            mexit(1, pfile_name);
    }

    return daddr;
}

#endif  /* #if !defined(linux) */
