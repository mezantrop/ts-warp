/* ------------------------------------------------------------------------------------------------------------------ */
/* TS-Warp - Transparent SOCKS protocol Wrapper                                                                       */
/* ------------------------------------------------------------------------------------------------------------------ */

/*
* Copyright (c) 2021, 2022, Mikhail Zakharov <zmey20000@yahoo.com>
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

/*
* Copyright (c) 2001 Daniel Hartmeier
* Copyright (c) 2002 - 2013 Henning Brauer <henning@openbsd.org>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the 
* following conditions are met:
*
*    - Redistributions of source code must retain the above copyright notice, this list of conditions and the following 
*      disclaimer.
*    - Redistributions in binary form must reproduce the above copyright notice, this list of conditions and 
*      the following disclaimer in the documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* -- IP address NAT lookup structure definitions ------------------------------------------------------------------- */
#include <sys/socket.h>
#include <netinet/in.h>


#define PF_DEV  "/dev/pf"

/* Structures defined in netpfil/pf/pf.h and net/pfvar.h for PF packet filer. */
enum    { PF_INOUT, PF_IN, PF_OUT };

struct pf_addr {
    union {
        struct in_addr  v4;
        struct in6_addr v6;
        u_int8_t        addr8[16];
        u_int16_t       addr16[8];
        u_int32_t       addr32[4];
    } pfa;
    #define v4      pfa.v4
    #define v6      pfa.v6
    #define addr8   pfa.addr8
    #define addr16  pfa.addr16
    #define addr32  pfa.addr32
};

struct pfioc_natlook {
    struct pf_addr  saddr;
    struct pf_addr  daddr;
    struct pf_addr  rsaddr;
    struct pf_addr  rdaddr;
    #if defined(__OpenBSD__)
        u_int16_t       rdomain;
        u_int16_t       rrdomain;
    #endif
    #if defined(__APPLE__)
        union { u_int16_t sport;  u_int32_t _f1; };     /* _f1 to _f5 are some Apple specific unused fields */
        union { u_int16_t dport;  u_int32_t _f2; };
        union { u_int16_t rsport; u_int32_t _f3; };
        union { u_int16_t rdport; u_int32_t _f4; };
    #else
        u_int16_t       sport;
        u_int16_t       dport;
        u_int16_t       rsport;
        u_int16_t       rdport;
    #endif
    sa_family_t     af;
    u_int8_t        proto;
    #if defined(__APPLE__)
        u_int8_t        _f5;
    #endif
    u_int8_t        direction;
};

#define DIOCNATLOOK    _IOWR('D', 23, struct pfioc_natlook)

/* -- Function prototypes ------------------------------------------------------------------------------------------- */
int pf_open();
int pf_close(int pfd);
int nat_lookup(int pfd, struct sockaddr *caddr, struct sockaddr *iaddr, struct sockaddr *daddr);
