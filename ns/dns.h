/* ------------------------------------------------------------------------------------------------------------------ */
/* NS-Warp - DNS responder/proxy                                                                                      */
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


/* -- DNS ----------------------------------------------------------------------------------------------------------- */

/*
    DNS  Header:
    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                       ID                      |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     QDCOUNT                   |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     ANCOUNT                   |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     NSCOUNT                   |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     ARCOUNT                   |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

    QR: 1       Response: message is response/query
    Opcode: 4   Opcode: QUERY (standard query, 0), IQUERY (inverse query, 1), STATUS (server status request, 2)
    AA: 1       Authoritative Answer, in a response 
    TC: 1       TrunCation
    RD: 1       Recursion Desired, in a request
    RA: 1       Recursion Available, in a response
    Z: 3        Zero, reserved for future use
    RCODE: 4    Response code: NOERROR (0), FORMERR (1, Format error), SERVFAIL (2), NXDOMAIN (3, Nonexistent domain)...

    Question:
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                                               |
    /                     NAME                      /
    /                                               /
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     TYPE                      |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     CLASS                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

    Answer:
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                                               |
    /                                               /
    /                     NAME                      /
    |                                               |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     TYPE                      |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     CLASS                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     TTL                       |
    |                                               |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                   RDLENGTH                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
    /                     RDATA                     /
    /                                               /
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
*/


/* ------------------------------------------------------------------------------------------------------------------ */
#include <stdint.h>

/* ------------------------------------------------------------------------------------------------------------------ */
typedef struct dns_header {
    uint16_t id;                    /* Transaction ID */
    uint16_t flags;
    uint16_t qdcount;               /* Questions: specifying the number of entries in the question section */
    uint16_t ancount;               /* Answer RRs: number of resource records in the answer */
    uint16_t nscount;               /* Authority RRs: number of name server resource records in authority records */
    uint16_t arcount;               /* Additional RRs: number of resource records in the additional records section */
} dns_header;

/* DNS Message flags. Convert to network byte order! */
#define NS_FLAGS_QR_QUERY       0x0         /* Query message:           b0000 0000 0000 0000 */
#define NS_FLAGS_QR_RESPONSE    0x8000      /* Response message:        b1000 0000 0000 0000 */
#define NS_FLAGS_RCODE_NOERROR  0x0         /* Response code: NOERROR:  b0000 0000 0000 0000 */
#define NS_FLAGS_RCODE_FORMERR  0x1         /* Response code: FORMERR:  b0000 0000 0000 0001 */
#define NS_FLAGS_RCODE_SERVFAIL 0x2         /* Response code: SERVFAIL: b0000 0000 0000 0010 */
#define NS_FLAGS_RCODE_NXDOMAIN 0x3         /* Response code: NXDOMAIN: b0000 0000 0000 0011 */
#define NS_FLAGS_RCODE_MASK     0xF         /* Response code: MASK:     b0000 0000 0000 1111 */

/* Do I really need this in ns-warp? */
#define NS_FLAGS_OPCODE_QUERY   0x0         /* Opcode: QUERY */
#define NS_FLAGS_OPCODE_IQUERY  0x800       /* Opcode: IQUERY (inverse): b0000 1000 0000 0000  (?) */

typedef struct dns_question {
    char *name;                     /* a sequence of labels. Each label consists of a length octet followed by that 
                                        number of octets. Terminates with 0 */
    uint16_t type;                  /* Type of RR (A - 0x0001, AAAA - 0x001C, PTR - case 0x000C, MX, TXT, etc.) */
    uint16_t classc;                /* Class code */
    char *raw;                      /* The same fields as raw data */
} dns_question;

#define NS_MESSAGE_TYPE_A       0x0001
#define NS_MESSAGE_TYPE_PTR     0x000C
#define NS_MESSAGE_TYPE_AAAA    0x001C

/* Not required in ns-warp */
typedef struct dns_answer {
    char *name;                     /* The domain name that was queried. The same format as the QNAME in the questions */
    uint16_t type;                  /* TYPE of RDATA, e.g.:
                                        0x0001 (A record), 0x0005 (CNAME), 
                                        0x0002 (name servers) and 0x000f (mail servers) */
    uint16_t classr;                /* Class of RDATA, e.g.: 0x0001 (Internet Address) */
    uint32_t ttl;                   /* The number of seconds the results can be cached */
    uint16_t rdlength;              /* The length of the RDATA */
    char *rdata;                    /* Response data. Format depends on TYPE above: 
                                        A == 0x0001 - IP 4 octets, CNAME == 0x0005 - the name of the alias etc. */
} dns_answer;

#pragma pack(push, 1)
typedef struct dns_answer_ref {
    uint16_t name_ref;              /* A reference to the domain name that was queried */
    uint16_t type;                  /* TYPE of RDATA, e.g.:
                                        0x0001 (A record), 0x0005 (CNAME), 
                                        0x0002 (name servers) and 0x000f (mail servers) */
    uint16_t classr;                /* Class of RDATA, e.g.: 0x0001 (Internet Address) */
    uint32_t ttl;                   /* The number of seconds the results can be cached */
    uint16_t rdlength;              /* The length of the RDATA */
/*    char *rdata;                  Not including variable length response data: Format depends on TYPE above:  
                                        A == 0x0001 - IP 4 octets, CNAME == 0x0005 - the name of the alias etc. */
} dns_answer_ref;
#pragma pack(pop)


#define DNS_REV_LOOKUP_SUFFIX_IPV4  ".in-addr.arpa"
#define DNS_REV_LOOKUP_SUFFIX_IPV6  ".ip6.arpa"

/* -- Function prototypes ------------------------------------------------------------------------------------------- */
struct sockaddr rev_addr(struct sockaddr *sa);
char *reverse_ip(struct sockaddr *ip, char *rev_ip);
struct sockaddr forward_ip(char *rev_ip);
int dns_reply_a(unsigned int id, unsigned char *dnsq_raw, int dnsq_siz, struct sockaddr *ip, unsigned char *rbuf);
int dns_reply_ptr(unsigned int id, unsigned char *dnsq_raw, int dnsq_siz, char *q_name, unsigned char *rbuf);
int dns_reply_nfound(unsigned int id, unsigned int typ, unsigned char *dnsq_raw, int dnsq_siz, unsigned char *rbuf);
