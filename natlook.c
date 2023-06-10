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


/* -- IP address NAT lookup implementation -------------------------------------------------------------------------- */
#if !defined(linux)

#include <stdio.h>
#include <unistd.h>
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
#include "network.h"
#include "logfile.h"
#include "utility.h"


extern char *pfile_name;


/* ------------------------------------------------------------------------------------------------------------------ */
int pf_open() {
    int pfd;

    if ((pfd = open(PF_DEV, O_RDWR)) == -1) {
        printl(LOG_CRIT, "Error openin PF device: [%s]", PF_DEV);
        mexit(1, pfile_name);
    }
    return pfd;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int pf_close(int pfd) { return close(pfd); }

/* ------------------------------------------------------------------------------------------------------------------ */
int nat_lookup(int pfd, struct sockaddr_storage *caddr, struct sockaddr_storage *iaddr, struct sockaddr_storage *daddr) {

    struct pfioc_natlook pfnl;
    char dstr_addr[INET_ADDRPORTSTRLEN];


    memset(&pfnl, 0, sizeof(struct pfioc_natlook));
    pfnl.direction = PF_OUT;
    pfnl.af = caddr->ss_family;
    pfnl.proto = IPPROTO_TCP;

    memset(daddr, 0, sizeof *daddr);
    switch (pfnl.af) {
        case AF_INET:
            pfnl.saddr.v4 = SIN4_ADDR(*caddr);
            pfnl.daddr.v4 = SIN4_ADDR(*iaddr);
            pfnl.sport = SIN4_PORT(*caddr);
            pfnl.dport = SIN4_PORT(*iaddr);
            break;

        case AF_INET6:
            pfnl.saddr.v6 = SIN6_ADDR(*caddr);
            pfnl.daddr.v6 = SIN6_ADDR(*iaddr);
            pfnl.sport = SIN6_PORT(*caddr);
            pfnl.dport = SIN6_PORT(*iaddr);
            break;

        default:
            printl(LOG_CRIT, "Unsupported Address family in nat_lookup() request: [%d]", pfnl.af);
            return 1;
    }

    if (ioctl(pfd, DIOCNATLOOK, &pfnl) == -1) {
        if (errno == ENOENT) {
            printl(LOG_WARN, "Failed to query PF NAT about PF_OUT packets");
            pfnl.direction = PF_IN;
            if (ioctl(pfd, DIOCNATLOOK, &pfnl) != 0) {
                printl(LOG_WARN, "Failed to query PF NAT about PF_IN packets");
                return 1;
            }
        } else {
            printl(LOG_WARN, "Failed to query PF NAT");
            return 1;
        }
    }

    switch (pfnl.af) {
        case AF_INET:
            SIN4_ADDR(*daddr) = pfnl.rdaddr.v4;
            SIN4_PORT(*daddr) = pfnl.rdport;
            SIN4_FAMILY(*daddr) = pfnl.af;
            SIN4_LENGTH(*daddr) = sizeof(struct sockaddr_in);
            break;

        case AF_INET6:
            SIN6_ADDR(*daddr) = pfnl.rdaddr.v6;
            SIN6_PORT(*daddr) = pfnl.rdport;
            SIN6_FAMILY(*daddr) = pfnl.af;
            SIN6_LENGTH(*daddr) = sizeof(struct sockaddr_in6);
            break;

        default:
            printl(LOG_CRIT, "Unsupported Address family in nat_lookup() response: [%d]", pfnl.af);
            return 1;
    }
    printl(LOG_VERB, "Real destination address: [%s]", inet2str(daddr, dstr_addr));

    return 0;
}

#endif  /* #if !defined(linux) */
