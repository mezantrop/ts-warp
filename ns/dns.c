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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "dns.h"
#include "logfile.h"
#include "ns-warp.h"
#include "network.h"


/* ------------------------------------------------------------------------------------------------------------------ */
struct sockaddr rev_addr(struct sockaddr *ip) {
    /* Reverse sinX_addr to be used in PTR DNS lookups, e.g. 127.0.0.1 -> 1.0.0.127 */

    struct sockaddr sa;
    uint32_t ipv6_quad;

    sa = *ip;
    if (SA_FAMILY(sa) == AF_INET)
        ((struct sockaddr_in *)&sa)->sin_addr.s_addr = htonl(((struct sockaddr_in *)&sa)->sin_addr.s_addr);
    else {
        #if defined(linux)
            ipv6_quad = htonl(((struct sockaddr_in6 *)&sa)->sin6_addr.__in6_u.__u6_addr32[3]);
            ((struct sockaddr_in6 *)&sa)->sin6_addr.__in6_u.__u6_addr32[3] = 
                htonl(((struct sockaddr_in6 *)&sa)->sin6_addr.__in6_u.__u6_addr32[0]);
            ((struct sockaddr_in6 *)&sa)->sin6_addr.__in6_u.__u6_addr32[0] = ipv6_quad;

            ipv6_quad = htonl(((struct sockaddr_in6 *)&sa)->sin6_addr.__in6_u.__u6_addr32[2]);
            ((struct sockaddr_in6 *)&sa)->sin6_addr.__in6_u.__u6_addr32[2] = 
                htonl(((struct sockaddr_in6 *)&sa)->sin6_addr.__in6_u.__u6_addr32[1]);
            ((struct sockaddr_in6 *)&sa)->sin6_addr.__in6_u.__u6_addr32[1] = ipv6_quad;
        #else
            ipv6_quad = htonl(((struct sockaddr_in6 *)&sa)->sin6_addr.__u6_addr.__u6_addr32[3]);
            ((struct sockaddr_in6 *)&sa)->sin6_addr.__u6_addr.__u6_addr32[3] = 
                htonl(((struct sockaddr_in6 *)&sa)->sin6_addr.__u6_addr.__u6_addr32[0]);
            ((struct sockaddr_in6 *)&sa)->sin6_addr.__u6_addr.__u6_addr32[0] = ipv6_quad;

            ipv6_quad = htonl(((struct sockaddr_in6 *)&sa)->sin6_addr.__u6_addr.__u6_addr32[2]);
            ((struct sockaddr_in6 *)&sa)->sin6_addr.__u6_addr.__u6_addr32[2] = 
                htonl(((struct sockaddr_in6 *)&sa)->sin6_addr.__u6_addr.__u6_addr32[1]);
            ((struct sockaddr_in6 *)&sa)->sin6_addr.__u6_addr.__u6_addr32[1] = ipv6_quad;
        #endif
    }
    return sa;
}

/* ------------------------------------------------------------------------------------------------------------------ */
char *reverse_ip(struct sockaddr *ip, char *rev_ip) {
    /* Reverse an IP address to be used as used as reverse lookup in PTR queries */

    const char *rev_d = DNS_REV_LOOKUP_SUFFIX_IPV4;
    struct sockaddr sa;
    
    if (!rev_ip) return NULL;

    sa = *ip;
    sa = rev_addr(&sa);
    if (SA_FAMILY(sa) == AF_INET6) rev_d = DNS_REV_LOOKUP_SUFFIX_IPV6;

    inet2str(&sa, rev_ip);
    strcat(rev_ip, rev_d);

    printl(LOG_VERB, "Reverse IP: [%s]", rev_ip);

    return rev_ip;
}

/* ------------------------------------------------------------------------------------------------------------------ */
struct sockaddr forward_ip(char *rev_ip) {
    /* Make normal forward IP address from a reversed one */

    char *suff = NULL;
    struct sockaddr sa;
    char buf[HOST_NAME_MAX];

    memset(buf, 0, sizeof(buf));
    if (rev_ip && ((suff = strstr(rev_ip, DNS_REV_LOOKUP_SUFFIX_IPV4)) || 
        (suff = strstr(rev_ip, DNS_REV_LOOKUP_SUFFIX_IPV6))))
            strncpy(buf, rev_ip, suff - rev_ip);

    sa = str2inet(buf, NULL);
    sa = rev_addr(&sa);

    printl(LOG_VERB, "Forward IP: [%s]", inet2str(&sa, buf));

    return sa;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int dns_reply_a(unsigned int id, unsigned char *dnsq_raw, int dnsq_siz, struct sockaddr *ip, unsigned char *rbuf) {
    /* Prepare both A and AAAA replies in *rbuf based on input data */

    dns_header *dnsh;
    dns_answer_ref *dnsa;


    if (SA_FAMILY(*ip) != AF_INET && SA_FAMILY(*ip) != AF_INET6) {
        printl(LOG_WARN, "dns_reply_a(): Skipping answer: Unknown address family: [%d] in reply IP", SA_FAMILY(*ip));
        return 0;
    }

    /* Fill in the header */
    dnsh = (dns_header *)rbuf;   
    dnsh->id = id;
    dnsh->flags = 0x8085;                                           /* Standard query response, No error */
    dnsh->qdcount = 0x0100;
    dnsh->ancount = 0x0100;
    dnsh->nscount = 0x0000;
    dnsh->arcount = 0x0000;

    /* Add the query part */    
    memcpy(rbuf + sizeof(dns_header), dnsq_raw, dnsq_siz);

    /* Add answer part */
    dnsa = (dns_answer_ref *)(rbuf + sizeof(dns_header) + dnsq_siz); 
    dnsa->name_ref = 0x0CC0;
    dnsa->type = SA_FAMILY(*ip) == AF_INET ? 0x0100 : 0x1C00;
    dnsa->classr = 0x0100;
    dnsa->ttl = 0x00000000;
    dnsa->rdlength = 0x0400;

    if (SA_FAMILY(*ip) == AF_INET) {
        in_addr_t *rdata = (in_addr_t *)(rbuf + sizeof(dns_header) + dnsq_siz + sizeof(dns_answer_ref));
        *rdata = S4_ADDR(*ip);
        return sizeof(dns_header) + dnsq_siz + sizeof(dns_answer_ref) + sizeof(*rdata);
    } else {
        char *rdata = (char *)(rbuf + sizeof(dns_header) + dnsq_siz + sizeof(dns_answer_ref));
        memcpy(rdata, S6_ADDR(*ip), 16);
        return sizeof(dns_header) + dnsq_siz + sizeof(dns_answer_ref) + sizeof(*rdata);
    }

    return 0;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int dns_reply_ptr(unsigned int id, unsigned char *dnsq_raw, int dnsq_siz, char *q_name, unsigned char *rbuf) {
    dns_header *dnsh;
    dns_answer_ref *dnsa;
    char *s;
    int sl = 0;


    /* Fill in the header */
    dnsh = (dns_header *)rbuf;   
    dnsh->id = id;
    dnsh->flags = 0x8085;                                           /* Standard query response, No error */
    dnsh->qdcount = 0x0100;
    dnsh->ancount = 0x0100;
    dnsh->nscount = 0x0000;
    dnsh->arcount = 0x0000;

    /* Add the query part */    
    memcpy(rbuf + sizeof(dns_header), dnsq_raw, dnsq_siz);

    /* Add answer part */
    dnsa = (dns_answer_ref *)(rbuf + sizeof(dns_header) + dnsq_siz); 
    dnsa->name_ref = 0x0CC0;
    dnsa->type = 0x0C00;                                            /* PTR */
    dnsa->classr = 0x0100;
    dnsa->ttl = 0x00000000;
    dnsa->rdlength = 0x0100;                                        /* Count the final NULL, terminating rdata */

    char *rdata = (char *)(rbuf + sizeof(dns_header) + dnsq_siz + sizeof(dns_answer_ref));
    while ((s = strsep(&q_name, "."))) {
        sl = strlen(s);
        *rdata = sl;
        rdata++;
        strcpy(rdata, s);        
        rdata += sl;
        dnsa->rdlength += htons(sl + 1);
    }

    return sizeof(dns_header) + dnsq_siz + sizeof(dns_answer_ref) + ntohs(dnsa->rdlength) + 1;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int dns_reply_nfound(unsigned int id, unsigned int typ, unsigned char *dnsq_raw, int dnsq_siz, unsigned char *rbuf) {
    /* Reply the name is not found */
    
    dns_header *dnsh;


    /* Fill in the header */
    dnsh = (dns_header *)rbuf;   
    dnsh->id = id;
    dnsh->flags = 0x8385;                                           /* Standard query response, No error */
    dnsh->qdcount = 0x0100;
    dnsh->ancount = 0x0000;
    dnsh->nscount = 0x0000;
    dnsh->arcount = 0x0000;

    /* Add the query part */    
    memcpy(rbuf + sizeof(dns_header), dnsq_raw, dnsq_siz);

    return sizeof(dns_header) + dnsq_siz;
}
