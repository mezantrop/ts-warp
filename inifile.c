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


/* -- INI-file processing --------------------------------------------------- */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "inifile.h"
#include "socks.h"
#include "utils.h"
#include "xedec.h"

/* -------------------------------------------------------------------------- */
ini_section *read_ini(char *ifile_name) {
    /*Read and parse INI-file */

    FILE *fini;
    char buffer[BUF_SIZE], section[STR_SIZE];
    char *s = NULL, *d = NULL, *x = NULL;   /* String manipulation pointers */
    ini_entry entry;                        /* tmp place for the parsed line */
    ini_section *ini_root = NULL, *c_sect = NULL, *l_sect = NULL;
    ini_target *c_targ = NULL, *l_targ = NULL;
    chain_list *chain_root = NULL, *chain_this = NULL, *chain_temp = NULL;
    int target_type = INI_TARGET_NOTSET;
    struct addrinfo res;
    int ln = 0;


    if (!(fini = fopen(ifile_name, "r"))) {
        printl(LOG_CRIT, "Error opening INI-file: %s", ifile_name);
        mexit(1, pfile_name);
    }

    while (fgets(buffer, sizeof buffer, fini) != NULL) {
        ln++;                           /* Increse the current line number */

        /* Chop remarks and newlines */
        s = buffer;
        strsep(&s, "#;\n");

        /* Get section */
        if (sscanf(buffer, "[%[A-Z \t0-9]]", section) == 1) {
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
            c_sect->next = NULL;

            if (!ini_root) ini_root = c_sect; else l_sect->next = c_sect;
            l_sect = c_sect;    /* lsect always points to the last section */
        } else {                        /* Entries within sections */
            
            /* Remove whitespaces */
            s = d = buffer; do while(isspace(*s)) s++; while((*d++ = *s++));

            if (!*buffer) continue;             /* Skip an empty line */
            if (strchr(buffer, '=') == NULL) {  /* Skip variables without vals */
                printl(LOG_VERB,
                    "LN: %d IGNORED: The variable must be assigned a value", ln);
                continue;
            }
            s = buffer;
            /* Get entry fields or NULLs */
            entry.var = strsep(&s, "=");        /* var */
            entry.val = strdup(s);              /* The raw value */
            entry.val1 = strsep(&s, "/");       /* val1 w opt. mod1/mod2 */
            entry.val2 = strsep(&s, "/");       /* val2 */
            /* val1 token round two parsing */
            s = entry.val1;
            entry.val1 = strsep(&s, ":-");      /* val1 clean */
            entry.mod1 = strsep(&s, ":-");      /* mod1 optional */
            entry.mod2 = strsep(&s, ":-");      /* mod2 optional */

            if (!entry.val1 || !entry.val1[0]) {
                printl(LOG_VERB,
                    "LN: %d IGNORED: The variable must be assigned a value", ln);
                continue;
            }

            printl(LOG_VERB, "LN: %d V: %s v1: %s v2: %s m1: %s m2: %s",
                ln, entry.var, entry.val1, entry.val2,
                !strcasecmp(entry.var, INI_ENTRY_SOCKS_PASSWORD) ? "********" : 
                entry.mod1, entry.mod2);

            /* Parse socks_* entries */
            if (!strcasecmp(entry.var, INI_ENTRY_SOCKS_SERVER)) {
                chk_inivar(&c_sect->socks_server, INI_ENTRY_SOCKS_SERVER);
                c_sect->socks_server = *(str2inet(entry.val1, entry.mod1 ? 
                    entry.mod1 : "1080", &res, NULL));
            } else
                if (!strcasecmp(entry.var, INI_ENTRY_SOCKS_VERSION)) {
                    chk_inivar(&c_sect->socks_version, INI_ENTRY_SOCKS_VERSION);
                    c_sect->socks_version = toint(entry.val);
            } else 
                if (!strcasecmp(entry.var, INI_ENTRY_SOCKS_USER)) {
                    if (chk_inivar(&c_sect->socks_user, INI_ENTRY_SOCKS_USER))
                        free(c_sect->socks_user);
                    c_sect->socks_user = strdup(entry.val);
            } else
                if (!strcasecmp(entry.var, INI_ENTRY_SOCKS_CHAIN)) {
                    chain_temp = (struct chain_list *)malloc(sizeof(struct chain_list));
                    chain_temp->txt_section = strdup(c_sect->section_name);
                    chain_temp->txt_chain = strdup(entry.val);
                    chain_temp->next = NULL;
                    if (chain_this) chain_this->next = chain_temp; 
                    else { chain_root = chain_temp; chain_this = chain_root; }

            } else
                if (!strcasecmp(entry.var, INI_ENTRY_SOCKS_PASSWORD)) {
                    if (chk_inivar(&c_sect->socks_password, INI_ENTRY_SOCKS_PASSWORD))
                            free(c_sect->socks_password);

                        if (!strcasecmp(entry.val1, XEDEC_PLAIN))
                            c_sect->socks_password = strdup(entry.val + strlen(XEDEC_PLAIN) + 1);
                        else if (!strcasecmp(entry.val1, XEDEC_TSW01)) {
                            if (!(x = xdecrypt(entry.val + strlen(XEDEC_TSW01) + 1, XEDEC_TSW01))) {
                                printl(LOG_CRIT, "Detected wrong encryption hash version!");
                                mexit(1, pfile_name);
                            }

                            c_sect->socks_password = strdup(x);
                            free(x);
                        } else {
                            printl(LOG_CRIT, "Malformed INI-file entry: [%s]",
                                INI_ENTRY_SOCKS_PASSWORD);
                            mexit(1, pfile_name);
                        }
            } else {
                target_type = INI_TARGET_NOTSET;
                /* Parse target_* entries: var=val1[:mod1[-mod2]]/val2 */
                if (!strcasecmp(entry.var, INI_ENTRY_TARGET_HOST))
                    target_type = INI_TARGET_HOST;
                else if (!strcasecmp(entry.var, INI_ENTRY_TARGET_DOMAIN))
                    target_type = INI_TARGET_DOMAIN;
                else if (!strcasecmp(entry.var, INI_ENTRY_TARGET_NETWORK))
                    target_type = INI_TARGET_NETWORK;
                else if (!strcasecmp(entry.var, INI_ENTRY_TARGET_RANGE))
                    target_type = INI_TARGET_RANGE;
                if (target_type) {
                    c_targ = (struct ini_target *)malloc(sizeof(struct ini_target));

                    c_targ->target_type = target_type;
                    /* Default values */
                    memset(&c_targ->ip1, 0, sizeof(struct sockaddr));
                    memset(&c_targ->ip2, 0, sizeof(struct sockaddr));
                    SIN4_FAMILY(c_targ->ip1) = AF_INET;
                    SIN4_FAMILY(c_targ->ip2) = AF_INET; 
 
                    if (target_type == INI_TARGET_DOMAIN)
                        c_targ->name = strdup(entry.val1);          /* Domain */
                    else {
                        c_targ->name = NULL;
                        c_targ->ip1 = *(str2inet(entry.val1, entry.mod1,
                            &res, NULL));
                        if (entry.val2)
                            c_targ->ip2 = *(str2inet(entry.val2, entry.mod2, 
                                &res, NULL));
                    }

                    /* Set defined ports range or default one: 0-65535 */
                    if (c_targ->ip1.sa_family == AF_INET) { 
                        SIN4_PORT(c_targ->ip1) = entry.mod1 ?
                            htons((int)strtol(entry.mod1, (char **)NULL, 10)) :
                            0;
                        SIN4_PORT(c_targ->ip2) = entry.mod2 ?
                            htons((int)strtol(entry.mod2, (char **)NULL, 10)) :
                            0xFFFF;
                    } else if (c_targ->ip1.sa_family == AF_INET6) {
                        SIN6_PORT(c_targ->ip1) = entry.mod1 ? 
                            htons((int)strtol(entry.mod1, (char **)NULL, 10)) :
                            0;
                        SIN6_PORT(c_targ->ip2) = entry.mod2 ?
                            htons((int)strtol(entry.mod2, (char **)NULL, 10)) :
                            0xFFFF;
                    } 
                    
                    c_targ->next = NULL;

                    if (!c_sect->target_entry) 
                        c_sect->target_entry = c_targ; 
                    else 
                        l_targ->next = c_targ;
                    l_targ = c_targ;    /* l_targ always points to the last */
                }
            }
            free(entry.val);
        }
    }
    
    create_chains(ini_root, chain_root);

    fclose(fini);
    return ini_root;
}

/* -------------------------------------------------------------------------- */
int create_chains(struct ini_section *ini, struct chain_list *chain) {
    struct ini_section *s, *sts;
    struct socks_chain *sc, *st;
    struct chain_list *c, *cc;
    char *chain_section, *p;
    
    c = chain;
    while (c) {
        if (c->txt_section) {
            if ((s = getsection(ini, c->txt_section))) {
                printl(LOG_VERB, "Section: [%s] has a chain link: [%s]", 
                    c->txt_section, c->txt_chain);

                sc = s->proxy_chain; 
                p = c->txt_chain;
                while ((chain_section = strsep(&p, ",")) != NULL) {
                    if ((sts = getsection(ini, chain_section))) {
                        st = (struct socks_chain *)malloc(sizeof(struct socks_chain));
                        st->chain_member = sts;
                        st->next = NULL;
                        if (sc) sc->next = st; else s->proxy_chain = st;
                        printl(LOG_VERB, "Linking chain section: [%s]", 
                            st->chain_member->section_name); 
                    } else
                        printl(LOG_VERB,
                            "Chain section: [%s] linked from: [%s] does not really exist", 
                            chain_section, s->section_name);
                }
            }
        }
        cc = c;
        c = c->next;
        free(cc);               /* Delete processed element from the list */
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
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

/* -------------------------------------------------------------------------- */
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
        printl(LOG_VERB,
            "SHOW Section: [%s] Server: [%s:%d] Version: [%d] User/Password: [%s/%s]",
            s->section_name, inet2str(&s->socks_server, ip1), 
            ntohs(SIN4_PORT(s->socks_server)), s->socks_version,
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

        /* Display target entries */
        t = s->target_entry;
        while (t) {
            printl(LOG_VERB, 
                "SHOW IP1: [%s] IP2: [%s] Port1: [%d] Port2:[%d] Name: [%s] Type: [%d]",
                inet2str(&t->ip1, ip1), inet2str(&t->ip2, ip2),
                t->ip1.sa_family == AF_INET ?
                    ntohs(SIN4_PORT(t->ip1)) : ntohs(SIN6_PORT(t->ip1)),
                t->ip1.sa_family == AF_INET ?
                    ntohs(SIN4_PORT(t->ip2)) : ntohs(SIN6_PORT(t->ip2)),
                t->name ? t->name : "", t->target_type);
            t = t->next;
        }
        s = s->next;
    }
}

/* -------------------------------------------------------------------------- */
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
        
        /* Delete target entries */
        t = ini->target_entry;
        while (t) {
            printl(LOG_VERB, 
                "DELETE IP1: [%s] IP2: [%s] Port1: [%d] Port2: [%d] Name: [%s] Type: [%d]",
                 inet2str(&t->ip1, ip1), inet2str(&t->ip2, ip2),
                 t->ip1.sa_family == AF_INET ?
                     ntohs(SIN4_PORT(t->ip1)) : ntohs(SIN6_PORT(t->ip1)),
                 t->ip1.sa_family == AF_INET ?
                     ntohs(SIN4_PORT(t->ip2)) : ntohs(SIN6_PORT(t->ip2)),
                 t->name ? t->name : "", t->target_type);
            tt = t->next;
            free(t);
            t = tt;
        }

        s = ini->next;
        free(ini);
        ini = s;
    }
    return (ini); 
}

/* -------------------------------------------------------------------------- */
struct ini_section *ini_look_server(struct ini_section *ini, struct sockaddr ip) {
    /* Lookup a SOCKS server ip in the list referred by ini */
    
    struct ini_section *s;
    struct ini_target *t;
    char *buf1 = NULL, *buf2 = NULL, *buf3 = NULL, *buf4 = NULL;
    char host[HOST_NAME_MAX], *domain = NULL;
    int hostlen = 0, domainlen = 0;

    if (getnameinfo(&ip, sizeof(ip), host, sizeof host, 0, 0, 0))
        printl(LOG_WARN, "Error resolving host: [%s]", inet2str(&ip, buf1));
    hostlen = strnlen(host, HOST_NAME_MAX);
 
    if ((domain = strchr(host, '.'))) {
        domain++;
        domainlen = strnlen(domain, HOST_NAME_MAX);
    }
    
    printl(LOG_VERB, "IP: [%s] resolves to: [%s] domain: [%s]",
        inet2str(&ip, buf1), host, domain);

    s = ini;
    while (s) {
        t = s->target_entry;

        while (t) {
            switch(t->target_type) {
                case INI_TARGET_HOST:
                    if ((ip.sa_family == AF_INET && 
                        S4_ADDR(ip) == S4_ADDR(t->ip1) &&
                        SIN4_PORT(ip) >= SIN4_PORT(t->ip1) && SIN4_PORT(ip) <= SIN4_PORT(t->ip2)) ||
                        (ip.sa_family == AF_INET6 && 
                        S6_ADDR(ip) == S6_ADDR(t->ip1) && 
                        SIN6_PORT(ip) >= SIN6_PORT(t->ip1) && SIN6_PORT(ip) <= SIN6_PORT(t->ip2))) {
                        printl(LOG_VERB, "Found SOCKS server: [%s] to serve IP: [%s] in: [%s]",
                            inet2str(&s->socks_server, buf1), inet2str(&ip, buf2), s->section_name);
                            free(buf1); free(buf2);
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
                            free(buf1);
                        return s;
                    }
                    break;

                case INI_TARGET_NETWORK:
                    if (ip.sa_family == AF_INET) {
                        /* IP & MASK_from_ini vs IP_from_ini & MASK_from_ini */
                        if ((S4_ADDR(ip) & S4_ADDR(t->ip2)) == (S4_ADDR(t->ip1) & S4_ADDR(t->ip2)) && 
                            SIN4_PORT(ip) >= SIN4_PORT(t->ip1) && SIN4_PORT(ip) <= SIN4_PORT(t->ip2)) {
                            printl(LOG_VERB, "Found SOCKS server: [%s] to serve IP: [%s] in NETWORK: [%s/%s] in: [%s]",
                                inet2str(&s->socks_server, buf1), inet2str(&ip, buf2),
                                inet2str(&t->ip1, buf3), inet2str(&t->ip2, buf4),
                                s->section_name);
                                free(buf1); free(buf2), free(buf3); free(buf4);
                            return s;
                        }
                    } else if (ip.sa_family == AF_INET6) {
                        /* IPv6 & MASK_from_ini vs IPv6_from_ini & MASK_from_ini */
			            int b;
                        for (b = 0; b < 16; ++b)
                            if ((S6_ADDR(ip)[b] & S6_ADDR(t->ip2)[b]) != (S6_ADDR(t->ip1)[b] & S6_ADDR(t->ip2)[b]))
                                break;

                        printl(LOG_VERB, "Found SOCKS server: [%s] to serve IP: [%s] in NETWORK: [%s/%s] in: [%s]",
                            inet2str(&s->socks_server, buf1), inet2str(&ip, buf2),
                            inet2str(&t->ip1, buf3), inet2str(&t->ip2, buf4),
                            s->section_name);
                            free(buf1); free(buf2), free(buf3); free(buf4);
                        return s;
                    }
                    break;

                case INI_TARGET_RANGE:
                    if ((ip.sa_family == AF_INET &&
                        ntohl(S4_ADDR(ip)) >= ntohl(S4_ADDR(t->ip1)) && ntohl(S4_ADDR(ip)) <= ntohl(S4_ADDR(t->ip2)) &&
                        SIN4_PORT(ip) >= SIN4_PORT(t->ip1) && SIN4_PORT(ip) <= SIN4_PORT(t->ip2)) ||
                        (ip.sa_family == AF_INET6 &&
                        S6_ADDR(ip) >= S6_ADDR(t->ip1) && S6_ADDR(ip) <= S6_ADDR(t->ip2) &&
                        SIN6_PORT(ip) >= SIN6_PORT(t->ip1) && SIN6_PORT(ip) <= SIN6_PORT(t->ip2))) {
                        printl(LOG_VERB, "Found SOCKS server: [%s] to serve IP: [%s] in RANGE: [%s/%s] in: [%s]",
                            inet2str(&s->socks_server, buf1), inet2str(&ip, buf2),
                            inet2str(&t->ip1, buf3), inet2str(&t->ip2, buf4),
                            s->section_name);
                            free(buf1); free(buf2), free(buf3); free(buf4);
                        return s;
                    }
                    break;
                case INI_TARGET_NOTSET:         /* Nothing to do, skip */
                default:
                    break;
            }
            t = t->next;
        }
        s = s->next;
    }
    
    return NULL;
}

/* -------------------------------------------------------------------------- */
int chk_inivar(void *v, char *vi) {
    /* Check if an INI-file variable already has a value in memory. Here:
        *v      a variable in memory, 
        *vi     a variable name in the INI-file (just for logging) */

    if (*(int *)v) {
        printl(LOG_WARN, "Updating already defined [%s] variable", vi);
        return 1;
    }

    return 0;
}
