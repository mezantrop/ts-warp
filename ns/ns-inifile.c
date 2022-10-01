/* ------------------------------------------------------------------------------------------------------------------ */
/* NS-Warp - DNS responder/proxy                                                                             */
/* ------------------------------------------------------------------------------------------------------------------ */

/* Copyright (c) 2022, Mikhail Zakharov <zmey20000@yahoo.com>

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


/* -- INI-file processing ------------------------------------------------------------------------------------------- */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "ns-warp.h"
#include "network.h"
#include "logfile.h"
#include "dns.h"
#include "ns-inifile.h"

/* ------------------------------------------------------------------------------------------------------------------ */
nit *read_ini(char *ifile_name) {
    FILE *fini;
    char buffer[BUF_SIZE];
    char *s = NULL, *d = NULL;                                          /* String manipulation pointers */
    ns_ini_entry entry = {NULL, NULL, NULL, NULL, NULL};
    struct addrinfo res;
    int ln = 0, i = 0;                                                  /* INI-file line count, misc counter */
    struct nit *nit_root = NULL, *c_nit = NULL, *l_nit = NULL;
    unsigned int nit_size = 0;                                          /* NIT pool length */


    if (!(fini = fopen(ifile_name, "r"))) {
        printl(LOG_CRIT, "Error opening INI-file: %s", ifile_name);
        exit(1);
    }

    while (fgets(buffer, sizeof buffer, fini) != NULL) {
        ln++;                                                                   /* Increse the current line number */

        /* Chop remarks and newlines */
        s = buffer;
        strsep(&s, "#;\n");

        /* Remove whitespaces */
        s = d = buffer; do while(isspace(*s)) s++; while((*d++ = *s++));

        if (!*buffer) continue;                                                 /* Skip an empty line */
        if (strchr(buffer, '=') == NULL) {                                      /* Skip variables without vals */
            printl(LOG_VERB, "LN: %d IGNORED: The variable must be assigned a value", ln);
            continue;
        }

        /* Parsed INI-entry: var=val1:val2/mod1 */
        s = buffer;
        /* Get entry fields or NULLs */
        entry.var = strsep(&s, "=");                                /* var should be i.e., nit_pool */
        entry.val = strdup(s);                                      /* The raw value of var */
        entry.val1 = strsep(&s, ":");                               /* domain name */
        entry.val2 = strsep(&s, "/");                               /* IP network */
        entry.mod1 = s;                                             /* IP netmask */

        printl(LOG_VERB, "LN: [%d] V: [%s] v1: [%s] v2: [%s] m1: [%s]", ln, entry.var, entry.val1, entry.val2, entry.mod1);

        if (!entry.var && !entry.val1 && !entry.val2 && !entry.mod1) {
            printl(LOG_VERB, "LN: %d IGNORED: Corrupted format", ln);
            free(entry.val);
            continue;
        }

        if (!strcasecmp(entry.var, NS_INI_ENTRY_NIT_POOL)) {
            c_nit = (struct nit *)malloc(sizeof(struct nit));
            c_nit->domain = strdup(entry.val1);
            c_nit->ip_addr = *(str2inet(entry.val2, NULL, &res, NULL));
            c_nit->ip_mask = *(str2inet(entry.mod1, NULL, &res, NULL));
            c_nit->next = NULL;
            c_nit->iname = 0;

            if (SA_FAMILY(c_nit->ip_addr) == AF_INET) {
                nit_size = ~ntohl(S4_ADDR(c_nit->ip_mask)); 
                printl(LOG_VERB, "Netmask: [%0X] Pool size: [%0X]", S4_ADDR(c_nit->ip_mask), nit_size);
            } else {
                nit_size = NS_NIT_IPV6_POOL_SIZE;
                printl(LOG_VERB, "Netmask: [%0X] Pool size: [%0X]", S6_ADDR(c_nit->ip_mask), nit_size);
            }

            /* Create NIT pool: array of pointers to strings */
            c_nit->names = malloc(nit_size * sizeof(char*));            /* Don't forget to free() it after! */
            for (i = 0; i < nit_size; i++) c_nit->names[i] = NULL;

            if (!nit_root) nit_root = c_nit; else l_nit->next = c_nit;
            l_nit = c_nit;                                              /* l_nit always points to the last element */
        } else {
            printl(LOG_VERB, "LN: %d IGNORED: Not NS-Warp variable", ln);
            free(entry.val);
            continue;
        }
    }

    free(entry.val);
    fclose(fini);
    
    return nit_root;
}

/* ------------------------------------------------------------------------------------------------------------------ */
void show_ini(struct nit *nit_root) {
    struct nit *s;
    unsigned int nit_size, i;
    char ip1[INET6_ADDRSTRLEN], ip2[INET6_ADDRSTRLEN];

    s = nit_root;
    while (s) {
        nit_size = ~ntohl(S4_ADDR(s->ip_mask));
        printl(LOG_VERB, "Domain: [%s] Pool: [%s] Netmask: [%s] Pool size: [%d]", 
            s->domain, inet2str(&s->ip_addr, ip1), inet2str(&s->ip_mask, ip2), nit_size);

        printl(LOG_VERB, "Pool contents:", nit_size);
        for (i = 0; i < nit_size; i++)
            printl(LOG_VERB, "Address: [%d]\tName: [%s]", i, s->names[i]);

        s = s->next;
    }
}

/* ------------------------------------------------------------------------------------------------------------------ */
int nit_lookup_name(struct nit *nit_root, char *name, int af, struct sockaddr *ip) {
    /* Forward NIT query for name, set ip as result. Return 0 on success, 1 if Name is not found in NIT */

    struct nit *s = NULL;
    unsigned int i = 0;
    unsigned int nit_size = 0;
    struct sockaddr r_ip;

    s = nit_root;
    while (s) {
        if (strcmp(s->domain, name + strlen(name) - strlen(s->domain)) == 0) {
            /* Found the name from the request in NIT domain */
            printl(LOG_VERB, "Found: [%s] as part of domain: [%s]", name, s->domain);

            if (SA_FAMILY(s->ip_addr) == af) {
                if (af == AF_INET) 
                    nit_size = ~ntohl(S4_ADDR(s->ip_mask)); 
                else 
                    nit_size = NS_NIT_IPV6_POOL_SIZE;

                for (i = 0; i < nit_size; i++)
                    if (s->names[i] && strncmp(s->names[i], name, strlen(name)) == 0) {
                        printl(LOG_VERB, "Found: [%s] in domain [%s] at index: [%d]", name, s->domain, i);

                        r_ip = s->ip_addr;
                        if (af == AF_INET)
                            ((struct sockaddr_in *)&r_ip)->sin_addr.s_addr += htonl(i);
                        else 
                            /* A dirty hack to increase the first only 4 bytes of the IPv6 adress */
                            #if defined(linux)
                                ((struct sockaddr_in6 *)&r_ip)->sin6_addr.__in6_u.__u6_addr32[0] += i;
                            #else
                                ((struct sockaddr_in6 *)&r_ip)->sin6_addr.__u6_addr.__u6_addr32[0] += i;
                            #endif
                        *ip = r_ip;

                        return 0;
                    }

                printl(LOG_VERB, "Adding [%s] to domain [%s] at index [%d]", name, s->domain, s->iname);
                if (s->names[s->iname]) free(s->names[s->iname]);
                s->names[s->iname] = strdup(name);

                r_ip = s->ip_addr;
                if (af == AF_INET)
                    ((struct sockaddr_in *)&r_ip)->sin_addr.s_addr += htonl(s->iname);
                else 
                    /* A dirty hack to increase the first only 4 bytes of the IPv6 adress */
                    #if defined(linux)
                        ((struct sockaddr_in6 *)&r_ip)->sin6_addr.__in6_u.__u6_addr32[0] += s->iname;
                    #else
                        ((struct sockaddr_in6 *)&r_ip)->sin6_addr.__u6_addr.__u6_addr32[0] += s->iname;
                    #endif
                *ip = r_ip;

                s->iname++;
                if (s->iname > nit_size) s->iname = 0;
                return 0;
            } else
                printl(LOG_VERB, "Skipping the pool of non-matching address family");
        } else
            printl(LOG_VERB, "NOT Found: [%s] as part of domain: [%s]", name, s->domain);

        s = s->next;
    }

    /* The requested name does not belong to any domain in NIT */
    return 1;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int nit_lookup_ip(struct nit *nit_root, struct sockaddr *ip, char *name) {
    /* Reverse NIT query for ip, set name as result. Return 0 on success, or 1 if IP is not found in NIT */

    struct nit *s = NULL;
    char buf[STR_SIZE];
    int idx = 0;

    s = nit_root;
    while (s) {
/*        printl(LOG_VERB, "IP/MASK [%08X] & [%08X] = [%08X]", 
            S4_ADDR(s->ip_addr), ~ntohl(S4_ADDR(s->ip_mask)), (ntohl(S4_ADDR(s->ip_addr) & ~S4_ADDR(s->ip_mask))));

        printl(LOG_VERB, "[%08X]&[%08X] = [%08X] ? [%08X]&[%08X] = [%08X]", 
            S4_ADDR(s->ip_addr), S4_ADDR(s->ip_mask), (S4_ADDR(s->ip_addr) & S4_ADDR(s->ip_mask)), 
            S4_ADDR(*ip), S4_ADDR(s->ip_mask), (S4_ADDR(*ip) & S4_ADDR(s->ip_mask)));
*/
        if ((S4_ADDR(s->ip_addr) & S4_ADDR(s->ip_mask)) == (S4_ADDR(*ip) & S4_ADDR(s->ip_mask))) {
            /* The IP is in the pool. Now let's find it's name */

            idx = ntohl(S4_ADDR(*ip) & ~S4_ADDR(s->ip_mask));
            printl(LOG_VERB, "Trying index: [%d]", idx);

            if (idx > s->iname || idx < 0 ) {
                printl(LOG_VERB, "IP address: [%s] is not registered in NIT at index: [%d]",
                    inet2str(ip, buf), idx);
                return 2;
            }
            if (!s->names[idx]) {
                printl(LOG_VERB, "IP address: [%s] at index: [%d] resolves to void", inet2str(ip, buf), idx);
                return 2;
            }

            strcpy(name, s->names[idx]);
            printl(LOG_VERB, "nit_lookup_ip(): Name: [%s]", name);
            printl(LOG_VERB, "Found: [%s] in domain [%s] at index: [%d]", name, s->domain, idx);
            return 0;
        }
        s = s->next;
    }

    return 1;
}
