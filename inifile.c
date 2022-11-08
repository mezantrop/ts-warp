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


/* -- INI-file processing ------------------------------------------------------------------------------------------- */
#if defined(linux)
#define _GNU_SOURCE 
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "inifile.h"
#include "network.h"
#include "socks.h"
#include "logfile.h"
#include "pidfile.h"
#include "utility.h"
#include "xedec.h"


/* ------------------------------------------------------------------------------------------------------------------ */
ini_section *read_ini(char *ifile_name) {
    /* Read and parse INI-file */

    FILE *fini;
    char buffer[BUF_SIZE], section[STR_SIZE];
    char *s = NULL, *d = NULL, *x = NULL;                               /* String manipulation pointers */
    ini_entry entry;                                                    /* tmp place for the parsed line */
    ini_section *ini_root = NULL, *c_sect = NULL, *l_sect = NULL;
    ini_target *c_targ = NULL, *l_targ = NULL;
    chain_list *chain_root = NULL, *chain_this = NULL, *chain_temp = NULL;
    int target_type = INI_TARGET_NOTSET;
    int ln = 0;


    if (!(fini = fopen(ifile_name, "r"))) {
        printl(LOG_CRIT, "Error opening INI-file: %s", ifile_name);
        mexit(1, pfile_name);
    }

    while (fgets(buffer, sizeof buffer, fini) != NULL) {
        ln++;                                                           /* Increse the current line number */

        /* Chop remarks and newlines */
        s = buffer;
        strsep(&s, "#;\n\r");

        /* Get section */
        if (sscanf(buffer, "[%[a-zA-Z -_\t0-9]]", section) == 1) {
            printl(LOG_VERB, "LN: %d S: %s", ln, section);

            /* Current section to use */
            c_sect = (struct ini_section *)malloc(sizeof(struct ini_section));
            c_sect->section_name = strndup(section, sizeof section);
            memset(&c_sect->socks_server, 0, sizeof(struct sockaddr));
            c_sect->socks_version = PROXY_PROTO_SOCKS_V5;
            c_sect->socks_user = NULL;
            c_sect->socks_password = NULL;
            c_sect->proxy_chain = NULL;
            c_sect->target_entry = NULL;

            c_sect->nit_domain = NULL;
            memset(&c_sect->nit_ipaddr, 0, sizeof(struct sockaddr));
            memset(&c_sect->nit_ipmask, 0, sizeof(struct sockaddr));

            c_sect->next = NULL;

            if (!ini_root) ini_root = c_sect; else l_sect->next = c_sect;
            l_sect = c_sect;                                        /* lsect always points to the last section */
        } else {                                                    /* Entries within sections */
            s = d = buffer; 
            do {
                while(isspace(*s)) s++;                             /* Remove whitespaces */
                if ((!isascii(*s) || iscntrl(*s)) && *s != '\0') {  /* Ignore lines with non-ASCII or Control chars */
                    printl(LOG_WARN, "LN: %d IGNORED: Contains non-ASCII or Control character: [%#x]!",
                        ln, (unsigned char)*s);
                    *buffer = '\0';
                    break;
                }
            } while((*d++ = *s++));

            if (!*buffer) continue;                                 /* Skip an empty line */
            if (strchr(buffer, '=') == NULL) {                      /* Skip variables without vals */
                printl(LOG_WARN, "LN: %d IGNORED: The variable must be assigned a value", ln);
                continue;
            }
            s = buffer;
            /* Get entry fields or NULLs */
            entry.var = strsep(&s, "=");                            /* var */
            entry.val = strdup(s);                                  /* The raw value */
            entry.val1 = strsep(&s, "/");                           /* val1 w opt. mod1/mod2 */
            entry.val2 = strsep(&s, "/");                           /* val2 */
            /* val1 token round two parsing */
            s = entry.val1;
            entry.val1 = strsep(&s, ":-");                          /* val1 clean */
            entry.mod1 = strsep(&s, ":-");                          /* mod1 optional */
            entry.mod2 = strsep(&s, ":-");                          /* mod2 optional */

            if (!l_sect && entry.var) {
                /* A line is not in a section */
                printl(LOG_WARN, "LN: %d IGNORED: The variable is not in a section", ln);
                free(entry.val);
                continue;
            }

            if (!entry.val1 || !entry.val1[0]) {
                printl(LOG_WARN, "LN: %d IGNORED: The variable must be assigned a value", ln);
                free(entry.val);
                continue;
            }

            printl(LOG_VERB, "LN: [%d] V: [%s] v1: [%s] v2: [%s] m1: [%s] m2: [%s]",
                ln, entry.var, entry.val1, entry.val2,
                !strcasecmp(entry.var, INI_ENTRY_SOCKS_PASSWORD) ? "********" : entry.mod1, entry.mod2);

            /* Parse socks_* entries */
            if (!strcasecmp(entry.var, INI_ENTRY_SOCKS_SERVER)) {
                chk_inivar(&c_sect->socks_server, INI_ENTRY_SOCKS_SERVER, ln);
                c_sect->socks_server = str2inet(entry.val1, entry.mod1 ? entry.mod1 : "1080");
            } else
                if (!strcasecmp(entry.var, INI_ENTRY_SOCKS_VERSION)) {
                    chk_inivar(&c_sect->socks_version, INI_ENTRY_SOCKS_VERSION, ln);
                    c_sect->socks_version = toint(entry.val);
                    if (c_sect->socks_version != PROXY_PROTO_SOCKS_V4 &&
                        c_sect->socks_version != PROXY_PROTO_SOCKS_V5) {
                            printl(LOG_WARN,
                                "LN: [%d] Detected unsupported SOCKS version: [%d]", ln, c_sect->socks_version);
                            
                            c_sect->socks_version = PROXY_PROTO_SOCKS_V5;

                            printl(LOG_WARN, 
                                "LN: [%d] Resetting SOCKS version to default: [%d]", ln, c_sect->socks_version);
                        }
            } else 
                if (!strcasecmp(entry.var, INI_ENTRY_SOCKS_USER)) {
                    if (chk_inivar(&c_sect->socks_user, INI_ENTRY_SOCKS_USER, ln)) free(c_sect->socks_user);
                    c_sect->socks_user = strdup(entry.val);
            } else
                if (!strcasecmp(entry.var, INI_ENTRY_SOCKS_CHAIN)) {
                    chain_temp = (struct chain_list *)malloc(sizeof(struct chain_list));
                    chain_temp->txt_section = strdup(c_sect->section_name);
                    chain_temp->txt_chain = strdup(entry.val);
                    chain_temp->next = NULL;
                    if (!chain_root) chain_root = chain_temp; else chain_this->next = chain_temp;
                    chain_this = chain_temp;
            } else
                if (!strcasecmp(entry.var, INI_ENTRY_SOCKS_PASSWORD)) {
                    if (chk_inivar(&c_sect->socks_password, INI_ENTRY_SOCKS_PASSWORD, ln))
                            free(c_sect->socks_password);

                        if (!strcasecmp(entry.val1, XEDEC_PLAIN))
                            c_sect->socks_password = strdup(entry.val + strlen(XEDEC_PLAIN) + 1);
                        else if (!strcasecmp(entry.val1, XEDEC_TSW01)) {
                            if (!(x = xdecrypt(entry.val + strlen(XEDEC_TSW01) + 1, XEDEC_TSW01))) {
                                printl(LOG_CRIT, "LN: [%d] Detected wrong encryption hash version!", ln);
                                mexit(1, pfile_name);
                            }

                            c_sect->socks_password = strdup(x);
                            free(x);
                        } else {
                            printl(LOG_CRIT, "LN: [%d] Malformed INI-file entry: [%s]", ln, INI_ENTRY_SOCKS_PASSWORD);
                            mexit(1, pfile_name);
                        }                        
            } else
                /* Parse nit_* entries */
                if (!strcasecmp(entry.var, NS_INI_ENTRY_NIT_POOL)) {
                    c_sect->nit_domain = strdup(entry.val1);
                    c_sect->nit_ipaddr = str2inet(entry.mod1, NULL);
                    c_sect->nit_ipmask = str2inet(entry.val2, NULL);
            } else {
                target_type = INI_TARGET_NOTSET;
                /* Parse target_* entries: var=val1[:mod1[-mod2]]/val2 */
                if (!strcasecmp(entry.var, INI_ENTRY_TARGET_HOST)) target_type = INI_TARGET_HOST;
                else if (!strcasecmp(entry.var, INI_ENTRY_TARGET_DOMAIN)) target_type = INI_TARGET_DOMAIN;
                else if (!strcasecmp(entry.var, INI_ENTRY_TARGET_NETWORK)) target_type = INI_TARGET_NETWORK;
                else if (!strcasecmp(entry.var, INI_ENTRY_TARGET_RANGE)) target_type = INI_TARGET_RANGE;
                if (target_type) {
                    c_targ = (struct ini_target *)malloc(sizeof(struct ini_target));

                    c_targ->target_type = target_type;
                    /* Default values */
                    memset(&c_targ->ip1, 0, sizeof(struct sockaddr));
                    memset(&c_targ->ip2, 0, sizeof(struct sockaddr));
                    SIN4_FAMILY(c_targ->ip1) = AF_INET;
                    SIN4_FAMILY(c_targ->ip2) = AF_INET; 
 
                    if (target_type == INI_TARGET_DOMAIN) {
                        d = entry.val;
                        strsep(&d, "±§!@#$%^&*()_+=`~,<>/\\|{}[]:\"'");         /* Delete unwanted chars from domain */
                        c_targ->name = strdup(entry.val);                       /* Domain */
                    } else {
                        c_targ->name = NULL;
                        c_targ->ip1 = str2inet(entry.val1, entry.mod1);
                        if (entry.val2) c_targ->ip2 = str2inet(entry.val2, entry.mod2);
                    }

                    /* Set defined ports range or default one: 0-65535 */
                    if (c_targ->ip1.sa_family == AF_INET) { 
                        SIN4_PORT(c_targ->ip1) = entry.mod1 ? htons((int)strtol(entry.mod1, (char **)NULL, 10)) : 0;
                        SIN4_PORT(c_targ->ip2) = entry.mod2 ? htons((int)strtol(entry.mod2, (char **)NULL, 10)) : 0xFFFF;
                    } else if (c_targ->ip1.sa_family == AF_INET6) {
                        SIN6_PORT(c_targ->ip1) = entry.mod1 ? htons((int)strtol(entry.mod1, (char **)NULL, 10)) : 0;
                        SIN6_PORT(c_targ->ip2) = entry.mod2 ? htons((int)strtol(entry.mod2, (char **)NULL, 10)) : 0xFFFF;
                    } 
                    
                    c_targ->next = NULL;

                    if (!c_sect->target_entry) c_sect->target_entry = c_targ; else l_targ->next = c_targ;
                    l_targ = c_targ;                                    /* l_targ always points to the last */
                }
            }
            free(entry.val);
        }
    }
    
    create_chains(ini_root, chain_root);

    fclose(fini);
    return ini_root;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int create_chains(struct ini_section *ini, struct chain_list *chain) {
    struct ini_section *s, *sts;
    struct socks_chain *sc, *st;
    struct chain_list *c, *cc;
    char *chain_section, *p;
    
    c = chain;
    while (c) {
        if (c->txt_section[0]) {
            if ((s = getsection(ini, c->txt_section))) {
                printl(LOG_VERB, "Section: [%s] has a chain link: [%s]", c->txt_section, c->txt_chain);

                sc = s->proxy_chain; 
                p = c->txt_chain;
                while ((chain_section = strsep(&p, ",")) != NULL) {
                    if ((sts = getsection(ini, chain_section))) {
                        st = (struct socks_chain *)malloc(sizeof(struct socks_chain));
                        st->chain_member = sts;
                        st->next = NULL;
                        if (sc) sc->next = st; else s->proxy_chain = st;
                        printl(LOG_VERB, "Linking chain section: [%s]", st->chain_member->section_name); 
                    } else
                        printl(LOG_VERB, "Chain section: [%s] linked from: [%s] does not really exist",
                            chain_section, s->section_name);
                }
            }
        }
        cc = c;
        c = c->next;
        /* Delete processed element from the list */
        if (cc->txt_chain && cc->txt_chain[0]) free(cc->txt_chain);
        if (cc->txt_section && cc->txt_section[0]) free(cc->txt_section);
        free(cc);
    }

    return 0;
}

/* ------------------------------------------------------------------------------------------------------------------ */
struct ini_section *getsection(struct ini_section *ini, char *name) {
    /* Get reference to the INI section by it's name */
    
    struct ini_section *s;
    
    s = ini;
    while (s) {
        if (!strcmp(s->section_name, name)) {
            printl(LOG_VERB, "Section: [%s] found!", name);
            return s;
        }
        s = s->next;
    }
    printl(LOG_VERB, "Section: [%s] not found!", name);
    return NULL;
}

/* ------------------------------------------------------------------------------------------------------------------ */
void show_ini(struct ini_section *ini) {
    /* Debug only function to print parsed INI-file. TODO: Remove in release? */
    
    struct ini_section *s;
    struct socks_chain *c;
    struct ini_target *t;
    char ip1[INET6_ADDRSTRLEN], ip2[INET6_ADDRSTRLEN];

    printl(LOG_VERB, "Show INI-Configuration");

    s = ini;
    while (s) {
        /* Display section */
        printl(LOG_VERB, "SHOW Section: [%s] Server: [%s:%d] Version: [%d] User/Password: [%s/%s]",
            s->section_name, inet2str(&s->socks_server, ip1), ntohs(SIN4_PORT(s->socks_server)), s->socks_version,
            s->socks_user,  s->socks_password ? "********" : "(null)");

        /* Display SOCKS chain */
        if (s->proxy_chain) {
            printl(LOG_VERB, "Socks Chain:");
            c = s->proxy_chain;
            while (c) {
                printl(LOG_VERB, "[%s] ->", c->chain_member->section_name);
                c = c->next;
            }
            printl(LOG_VERB, "-> [%s]", s->section_name);
        }

        /* Display NIT pool */
        if (s->nit_domain)
            printl(LOG_VERB, "SHOW NIT Pool Domain: [%s], IP/Mask: [%s:%s]", 
                s->nit_domain, inet2str(&s->nit_ipaddr, ip1), inet2str(&s->nit_ipmask, ip2));

        /* Display target entries */
        t = s->target_entry;
        while (t) {
            printl(LOG_VERB, "SHOW IP1: [%s] IP2: [%s] Port1: [%d] Port2: [%d] Name: [%s] Type: [%d]",
                inet2str(&t->ip1, ip1), inet2str(&t->ip2, ip2),
                t->ip1.sa_family == AF_INET ? ntohs(SIN4_PORT(t->ip1)) : ntohs(SIN6_PORT(t->ip1)),
                t->ip1.sa_family == AF_INET ? ntohs(SIN4_PORT(t->ip2)) : ntohs(SIN6_PORT(t->ip2)),
                t->name ? t->name : "", t->target_type);
            t = t->next;
        }
        s = s->next;
    }
}

/* ------------------------------------------------------------------------------------------------------------------ */
struct ini_section *delete_ini(struct ini_section *ini) {
    /* Clean INI-data */

    struct ini_section *s;
    struct socks_chain *c, *cc;
    struct ini_target *t, *tt;
    char ip1[INET6_ADDRSTRLEN], ip2[INET6_ADDRSTRLEN];

    printl(LOG_VERB, "Delete INI-configuration");
    
    while (ini) {
        printl(LOG_VERB, "DELETE Section: [%s]", ini->section_name);
        
        /* Delete socks chain */
        if (ini->proxy_chain) {
            printl(LOG_VERB, "DELETE SOCKS Chain:");
            c = ini->proxy_chain;
            while (c) {
                printl(LOG_VERB, "[%s] ->", c->chain_member->section_name);
                cc = c->next;
                free(c);
                c = cc;
            }
        } else 
            printl(LOG_VERB, "No SOCKS Chain detected");
        
        /* Delete target_* entries */
        t = ini->target_entry;
        while (t) {
            printl(LOG_VERB, "DELETE IP1: [%s] IP2: [%s] Port1: [%d] Port2: [%d] Name: [%s] Type: [%d]",
                inet2str(&t->ip1, ip1), inet2str(&t->ip2, ip2),
                t->ip1.sa_family == AF_INET ? ntohs(SIN4_PORT(t->ip1)) : ntohs(SIN6_PORT(t->ip1)),
                t->ip1.sa_family == AF_INET ? ntohs(SIN4_PORT(t->ip2)) : ntohs(SIN6_PORT(t->ip2)),
                t->name ? t->name : "", t->target_type);
            tt = t->next;
            if (t->name && t->name[0]) free(t->name);    
            free(t);
            t = tt;
        }

        /* Delete socks_* entries */
        s = ini->next;
        if (ini->socks_user && ini->socks_user[0]) free(ini->socks_user);
        if (ini->socks_password && ini->socks_password[0]) free(ini->socks_password);
        if (ini->nit_domain && ini->nit_domain[0]) free(ini->nit_domain);

        /* Delete the section name */
        if (ini->section_name && ini->section_name[0]) free(ini->section_name);
        free(ini);
        ini = s;
    }
    return (ini); 
}

/* ------------------------------------------------------------------------------------------------------------------ */
struct ini_section *ini_look_server(struct ini_section *ini, struct sockaddr ip) {
    /* Lookup a SOCKS server ip in the list referred by ini */
    
    struct ini_section *s;
    struct ini_target *t;
    char buf1[INET6_ADDRSTRLEN], buf2[INET6_ADDRSTRLEN];
    char buf3[INET6_ADDRSTRLEN], buf4[INET6_ADDRSTRLEN];
    char host[HOST_NAME_MAX], *domain = NULL;
    int domainlen = 0;

    host[0] = 0;
    if (getnameinfo(&ip, sizeof(ip), host, sizeof host, 0, 0, NI_NAMEREQD))
        printl(LOG_VERB, "Unable to resolve hostname: [%s] into IP", inet2str(&ip, buf1));
    else
        if ((domain = strchr(host, '.'))) {
            domain++;
            domainlen = strnlen(domain, HOST_NAME_MAX);
        }
    
    printl(LOG_VERB, "IP: [%s] resolves to: [%s] domain: [%s]", inet2str(&ip, buf1), host, domain);

    s = ini;
    while (s) {
        t = s->target_entry;

        while (t) {
            switch(t->target_type) {
                case INI_TARGET_HOST:
                    if ((ip.sa_family == AF_INET && S4_ADDR(ip) == S4_ADDR(t->ip1) &&
                        SIN4_PORT(ip) >= SIN4_PORT(t->ip1) && SIN4_PORT(ip) <= SIN4_PORT(t->ip2)) ||
                        (ip.sa_family == AF_INET6 && !memcmp(S6_ADDR(ip), S6_ADDR(t->ip1), sizeof(S6_ADDR(ip))) &&
                        SIN6_PORT(ip) >= SIN6_PORT(t->ip1) && SIN6_PORT(ip) <= SIN6_PORT(t->ip2))) {
                            printl(LOG_VERB, "Found SOCKS server: [%s] to serve IP: [%s] in: [%s]",
                                inet2str(&s->socks_server, buf1), inet2str(&ip, buf2), s->section_name);
                            return s;
                    }
                    break;

                case INI_TARGET_DOMAIN:
                    if ((domainlen && strcasestr(t->name, domain) &&
                        (ip.sa_family == AF_INET &&
                            SIN4_PORT(ip) >= SIN4_PORT(t->ip1) && SIN4_PORT(ip) <= SIN4_PORT(t->ip2))) ||
                        (ip.sa_family == AF_INET6 &&
                            SIN6_PORT(ip) >= SIN6_PORT(t->ip1) && SIN6_PORT(ip) <= SIN6_PORT(t->ip2))) {
                            printl(LOG_VERB, "Found SOCKS server: [%s] to serve host: [%s] domain: [%s] in: [%s]",
                                inet2str(&s->socks_server, buf1), host, t->name, s->section_name);
                            return s;
                    }
                    break;

                case INI_TARGET_NETWORK:
                    if (ip.sa_family == AF_INET) {
                        /* IP & MASK_from_ini vs IP_from_ini & MASK_from_ini */
                        if ((S4_ADDR(ip) & S4_ADDR(t->ip2)) == (S4_ADDR(t->ip1) & S4_ADDR(t->ip2)) &&
                            SIN4_PORT(ip) >= SIN4_PORT(t->ip1) && SIN4_PORT(ip) <= SIN4_PORT(t->ip2)) {
                                printl(LOG_VERB, "Found SOCKS server: [%s] to serve IP: [%s] in NETWORK: [%s/%s] in: [%s]",
                                    inet2str(&s->socks_server, buf1), inet2str(&ip, buf2), inet2str(&t->ip1, buf3), 
                                    inet2str(&t->ip2, buf4), s->section_name);
                                return s;
                        }
                    } else if (ip.sa_family == AF_INET6) {
                        /* IPv6 & MASK_from_ini vs IPv6_from_ini & MASK_from_ini */
                        int b;
                        for (b = 0; b < 16; ++b)
                            if ((S6_ADDR(ip)[b] & S6_ADDR(t->ip2)[b]) != (S6_ADDR(t->ip1)[b] & S6_ADDR(t->ip2)[b]))
                                break;

                        printl(LOG_VERB, "Found SOCKS server: [%s] to serve IP: [%s] in NETWORK: [%s/%s] in: [%s]",
                            inet2str(&s->socks_server, buf1), inet2str(&ip, buf2), inet2str(&t->ip1, buf3),
                            inet2str(&t->ip2, buf4), s->section_name);
                        return s;
                    }
                    break;

                case INI_TARGET_RANGE:
                    if ((ip.sa_family == AF_INET &&
                        ntohl(S4_ADDR(ip)) >= ntohl(S4_ADDR(t->ip1)) && ntohl(S4_ADDR(ip)) <= ntohl(S4_ADDR(t->ip2)) &&
                        SIN4_PORT(ip) >= SIN4_PORT(t->ip1) && SIN4_PORT(ip) <= SIN4_PORT(t->ip2)) ||
                        (ip.sa_family == AF_INET6 &&
                        memcmp(S6_ADDR(ip), S6_ADDR(t->ip1), sizeof(S6_ADDR(ip))) > 0 &&
                        memcmp(S6_ADDR(ip), S6_ADDR(t->ip2), sizeof(S6_ADDR(ip))) < 0 &&
                        SIN6_PORT(ip) >= SIN6_PORT(t->ip1) && SIN6_PORT(ip) <= SIN6_PORT(t->ip2))) {
                            printl(LOG_VERB, "Found SOCKS server: [%s] to serve IP: [%s] in RANGE: [%s/%s] in: [%s]",
                                inet2str(&s->socks_server, buf1), inet2str(&ip, buf2), inet2str(&t->ip1, buf3),
                                inet2str(&t->ip2, buf4), s->section_name);
                            return s;
                    }
                    break;
                case INI_TARGET_NOTSET:                                 /* Nothing to do, skip */
                default:
                    break;
            }
            t = t->next;
        }
        s = s->next;
    }
    
    return NULL;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int chk_inivar(void *v, char *vi, int ln) {
    /* Check if an INI-file variable already has a value in memory. Here:
        *v      a variable in memory, 
        *vi     a variable name in the INI-file (just for logging) 
        ln      a line number in the ini-file (for logging only) */

    if (*(int *)v) {
        printl(LOG_WARN, "LN: [%d] Updating default or already defined [%s] variable", ln, vi);
        return 1;
    }

    return 0;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int socks5_atype(ini_section *ini, struct sockaddr daddr) {
    /* Return the SOCKS5 address type based on the address family and presence in NIT */
    int sat = SOCKS5_ATYPE_IPV4;
    int b = 0;

    if (daddr.sa_family == AF_INET && S4_ADDR(ini->socks_server) != S4_ADDR(daddr)) {
        if (ini->nit_domain && 
            (S4_ADDR(ini->nit_ipaddr) & S4_ADDR(ini->nit_ipmask)) == (S4_ADDR(daddr) & S4_ADDR(ini->nit_ipmask)))
                sat = SOCKS5_ATYPE_NAME;
    } else {
        /* AF_INET6 */
        sat = SOCKS5_ATYPE_NAME;
        for (b = 0; b < 16; ++b)
            if ((S6_ADDR(ini->nit_ipaddr)[b] & S6_ADDR(ini->nit_ipmask)[b]) != (S6_ADDR(daddr)[b] & S6_ADDR(ini->nit_ipmask)[b])) {
                sat = SOCKS5_ATYPE_IPV6;
                break;
            }
    }

    printl(LOG_VERB, "Selecting SOCKS5 address type: [%d]", sat);
    return sat;
}
