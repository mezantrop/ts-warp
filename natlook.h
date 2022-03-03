/* -------------------------------------------------------------------------- */ 
/* TS-Warp - Transparent SOCKS protocol Wrapper                               */
/* -------------------------------------------------------------------------- */ 

/* Copyright (c) 2021, 2022, Mikhail Zakharov <zmey20000@yahoo.com>

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


/* -- IP address NAT lookup structure definitions --------------------------- */
#include <sys/socket.h>
#include <netinet/in.h>


#define PF_DEV  "/dev/pf"

/* Structs defined in netpfil/pf/pf.h and net/pfvar.h for PF packet filer. */

#if defined(__APPLE__)
    /* Note, actual macOS 11.4 xnu kernel uses a fairly old version: 
    https://opensource.apple.com/source/xnu/xnu-7195.121.3/bsd/net/pfvar.h */

    enum    { PF_INOUT, PF_IN, PF_OUT };

    struct pf_addr {
        union {
            struct in_addr  _v4addr;
            struct in6_addr _v6addr;
            uint8_t         _addr8[16];
            uint16_t        _addr16[8];
            uint32_t        _addr32[4];
        } pfa;
    #define v4addr  pfa._v4addr
    #define v6addr  pfa._v6addr
    #define addr8   pfa._addr8
    #define addr16  pfa._addr16
    #define addr32  pfa._addr32
    };

    union pf_state_xport {
        uint16_t    port;
        uint16_t    call_id;
        uint32_t    spi;
    };

    struct pfioc_natlook {
        struct pf_addr          saddr;
        struct pf_addr          daddr;
        struct pf_addr          rsaddr;
        struct pf_addr          rdaddr;
        union pf_state_xport    sxport;
        union pf_state_xport    dxport;
        union pf_state_xport    rsxport;
        union pf_state_xport    rdxport;
        sa_family_t             af;
        uint8_t                proto;
        uint8_t                proto_variant;
        uint8_t                direction;
    };
#elif defined(__FreeBSD__)
    /* FreeBSD has slightly old definitions: pfvar.h,v 1.282 2009/01/29 */

    enum    { PF_INOUT, PF_IN, PF_OUT };

    struct pf_addr {
        union {
            struct in_addr  v4;
            struct in6_addr v6;
            u_int8_t        addr8[16];
            u_int16_t       addr16[8];
            u_int32_t       addr32[4];
        } pfa;      /* 128-bit address */
        #define v4      pfa.v4
        #define v6      pfa.v6
        #define addr8   pfa.addr8
        #define addr16  pfa.addr16
        #define addr32  pfa.addr32
    };

    struct pfioc_natlook {
        struct pf_addr   saddr;
        struct pf_addr   daddr;
        struct pf_addr   rsaddr;
        struct pf_addr   rdaddr;
        u_int16_t        sport;
        u_int16_t        dport;
        u_int16_t        rsport;
        u_int16_t        rdport;
        sa_family_t      af;
        u_int8_t         proto;
        u_int8_t         direction;
    };
#else
    /* Fresh https://cvsweb.openbsd.org/src/sys/net/pfvar.h, v 1.502 2021/06/23 */

    enum	{ PF_INOUT, PF_IN, PF_OUT, PF_FWD };

    struct pf_addr {
        union {
            struct in_addr  v4;
            struct in6_addr v6;
            u_int8_t        addr8[16];
            u_int16_t       addr16[8];
            u_int32_t       addr32[4];
        } pfa;
        #define v4      pfa.v4
        #define v6	    pfa.v6
        #define addr8	pfa.addr8
        #define addr16	pfa.addr16
        #define addr32	pfa.addr32
    };

    struct pfioc_natlook {
        struct pf_addr  saddr;
        struct pf_addr  daddr;
        struct pf_addr  rsaddr;
        struct pf_addr  rdaddr;
        u_int16_t       rdomain;
        u_int16_t       rrdomain;
        u_int16_t       sport;
        u_int16_t       dport;
        u_int16_t       rsport;
        u_int16_t       rdport;
        sa_family_t     af;
        u_int8_t        proto;
        u_int8_t        direction;
    };
#endif

#define DIOCNATLOOK    _IOWR('D', 23, struct pfioc_natlook)

/* -- Function prototypes --------------------------------------------------- */
struct sockaddr nat_lookup(struct sockaddr *caddr, struct sockaddr *iaddr);
