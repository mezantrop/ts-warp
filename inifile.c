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


/* -- INI-file processing ------------------------------------------------------------------------------------------- */
#if defined(linux)
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "utility.h"
#include "network.h"
#include "socks.h"
#include "http.h"
#include "ssh2.h"
#include "logfile.h"
#include "pidfile.h"
#include "xedec.h"
#include "inifile.h"


/* ------------------------------------------------------------------------------------------------------------------ */
ini_section *read_ini(char *ifile_name) {
    /* Read and parse INI-file */

    FILE *fini;
    char buffer[STR_SIZE], section[STR_SIZE];
    char *s = NULL, *d = NULL, *x = NULL;                               /* String manipulation pointers */
    ini_entry entry;                                                    /* tmp place for the parsed line */
    ini_section *ini_root = NULL, *c_sect = NULL, *l_sect = NULL;
    ini_target *c_targ = NULL, *l_targ = NULL;
    chain_list *chain_root = NULL, *chain_this = NULL, *chain_temp = NULL;
    int target_type = INI_TARGET_NOTSET;
    int ln = 0;
    char *proxy_server = NULL, *proxy_port = NULL;
    int fproxy_port = 0;


    if (!(fini = fopen(ifile_name, "r"))) {
        printl(LOG_CRIT, "Error opening INI-file: %s", ifile_name);
        mexit(1, pfile_name, tfile_name);
    }

    while (fgets(buffer, sizeof buffer, fini) != NULL) {
        ln++;                                                           /* Increse the current line number */

        /* Chop remarks and newlines */
        s = buffer;
        strsep(&s, "#;\n\r");

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

        /* Get section */
        if (sscanf(buffer, "[%[a-zA-Z0-9_\t +()-]]", section) == 1) {
            printl(LOG_VERB, "LN: [%d] S: [%s]", ln, section);

            /* Current section to use */
            c_sect = (struct ini_section *)malloc(sizeof(struct ini_section));
            c_sect->section_name = strndup(section, sizeof section);
            c_sect->section_balance = SECTION_BALANCE_FAILOVER;
            memset(&c_sect->proxy_server, 0, sizeof(struct sockaddr_storage));
            c_sect->proxy_type = PROXY_PROTO_SOCKS_V5;
            c_sect->proxy_user = NULL;
            c_sect->proxy_password = NULL;
            c_sect->proxy_key_passphrase = NULL;
            c_sect->proxy_key = NULL;
            c_sect->proxy_ssh_force_auth = 'N';
            c_sect->p_chain = NULL;
            c_sect->target_entry = NULL;
            c_sect->nit_domain = NULL;
            memset(&c_sect->nit_ipaddr, 0, sizeof(struct sockaddr_storage));
            memset(&c_sect->nit_ipmask, 0, sizeof(struct sockaddr_storage));

            c_sect->next = NULL;

            if (!ini_root) ini_root = c_sect; else l_sect->next = c_sect;
            l_sect = c_sect;                                        /* lsect always points to the last section */
            if (proxy_server) {
                free(proxy_server); proxy_server = NULL;
            }
            if (proxy_port && fproxy_port) {
                free(proxy_port); proxy_port = NULL; fproxy_port = 0;
                }
        } else {                                                    /* Entries within sections */
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
            entry.val1 = strsep(&s, ":");                           /* val1 clean */
            entry.mod1 = strsep(&s, ":-");                          /* mod1 optional */
            entry.mod2 = strsep(&s, ":-");                          /* mod2 optional */

            if (!l_sect && entry.var) {
                /* A line is not in a section */
                printl(LOG_WARN, "LN: %d IGNORED: The variable is not in a section", ln);
                free(entry.val);
                continue;
            }

            if ((!entry.val1 || !entry.val1[0]) && !entry.val[0]) {
                printl(LOG_WARN, "LN: %d IGNORED: The variable must be assigned a value", ln);
                free(entry.val);
                continue;
            }

            printl(LOG_VERB, "LN: [%d] V: [%s] v1: [%s] v2: [%s] m1: [%s] m2: [%s]",
                ln, entry.var, entry.val1 ? : "", entry.val2 ? : "",
                !strcasecmp(entry.var, INI_ENTRY_PROXY_KEY_PASSPHRASE) ||
                    !strcasecmp(entry.var, INI_ENTRY_PROXY_PASSWORD) ? "********" : entry.mod1?:"", entry.mod2?:"");

            /* -- Parse proxy_* entries ----------------------------------------------------------------------------- */
            /* TODO: Remove deprecated: INI_ENTRY_SOCKS_* check */
            if (!strcasecmp(entry.var, INI_ENTRY_PROXY_SERVER) || !strcasecmp(entry.var, INI_ENTRY_SOCKS_SERVER)) {
                chk_inivar(&c_sect->proxy_server, INI_ENTRY_PROXY_SERVER, ln);
                proxy_server = strdup(entry.val1);
                if (entry.mod1) {
                    proxy_port = strdup(entry.mod1);
                    c_sect->proxy_server = str2inet(proxy_server, proxy_port);
                    fproxy_port = 1;
                } else
                    c_sect->proxy_server = str2inet(proxy_server, proxy_port ? proxy_port : SOCKS_PORT);
            } else
                if (!strcasecmp(entry.var, INI_ENTRY_PROXY_TYPE)) {
                    chk_inivar(&c_sect->proxy_type, INI_ENTRY_PROXY_TYPE, ln);
                    c_sect->proxy_type = toupper(entry.val[0]);
                    switch(c_sect->proxy_type) {
                        case PROXY_PROTO_SOCKS_V4:
                        case PROXY_PROTO_SOCKS_V5:
                            if (!proxy_port) {
                                proxy_port = SOCKS_PORT;
                                c_sect->proxy_server = str2inet(proxy_server, SOCKS_PORT);
                            }
                        break;

                        case PROXY_PROTO_HTTP:
                            if (!proxy_port) {
                                proxy_port = SQUID_PORT;
                                c_sect->proxy_server = str2inet(proxy_server, SQUID_PORT);
                            }
                        break;

                        case PROXY_PROTO_SSH2:
                            if (!proxy_port) {
                                proxy_port = SSH2_PORT;
                                c_sect->proxy_server = str2inet(proxy_server, SSH2_PORT);
                            }
                        break;

                        default:
                            printl(LOG_WARN, "LN: [%d] Resetting unsupported proxy type [%c] to default: [%c]",
                                ln, c_sect->proxy_type, PROXY_PROTO_SOCKS_V5);
                            c_sect->proxy_type = PROXY_PROTO_SOCKS_V5;
                            c_sect->proxy_server = str2inet(proxy_server, SOCKS_PORT);
                            proxy_port = SOCKS_PORT;
                    }
            } else
                /* TODO: Remove deprecated: INI_ENTRY_SOCKS_* check */
                if (!strcasecmp(entry.var, INI_ENTRY_PROXY_USER) || !strcasecmp(entry.var, INI_ENTRY_SOCKS_USER)) {
                    if (chk_inivar(&c_sect->proxy_user, INI_ENTRY_PROXY_USER, ln)) free(c_sect->proxy_user);
                    c_sect->proxy_user = strdup(entry.val);
            } else
                /* TODO: Remove deprecated: INI_ENTRY_SOCKS_* check */
                if (!strcasecmp(entry.var, INI_ENTRY_PROXY_CHAIN) || !strcasecmp(entry.var, INI_ENTRY_SOCKS_CHAIN)) {
                    chain_temp = (struct chain_list *)malloc(sizeof(struct chain_list));
                    chain_temp->txt_section = strdup(c_sect->section_name);
                    chain_temp->txt_chain = strdup(entry.val);
                    chain_temp->next = NULL;
                    if (!chain_root) chain_root = chain_temp; else chain_this->next = chain_temp;
                    chain_this = chain_temp;
            } else
                /* TODO: Remove deprecated: INI_ENTRY_SOCKS_* check */
                if (!strcasecmp(entry.var, INI_ENTRY_PROXY_PASSWORD) ||
                    !strcasecmp(entry.var, INI_ENTRY_SOCKS_PASSWORD)) {

                    if (chk_inivar(&c_sect->proxy_password, INI_ENTRY_PROXY_PASSWORD, ln))
                            free(c_sect->proxy_password);

                        if (!strcasecmp(entry.val1, XEDEC_PLAIN))
                            c_sect->proxy_password = strdup(entry.val + strlen(XEDEC_PLAIN) + 1);
                        else if (!strcasecmp(entry.val1, XEDEC_TSW01)) {
                            if (!(x = xdecrypt(entry.val + strlen(XEDEC_TSW01) + 1, XEDEC_TSW01))) {
                                printl(LOG_CRIT, "LN: [%d] Detected wrong encryption hash!", ln);
                                mexit(1, pfile_name, tfile_name);
                            }
                            c_sect->proxy_password = strdup(x);
                            free(x);
                        } else {
                            printl(LOG_CRIT, "LN: [%d] Malformed INI-file entry: [%s]", ln, INI_ENTRY_PROXY_PASSWORD);
                            mexit(1, pfile_name, tfile_name);
                        }
            } else
                if (!strcasecmp(entry.var, INI_ENTRY_PROXY_KEY)) {
                    if (chk_inivar(&c_sect->proxy_key, INI_ENTRY_PROXY_KEY, ln))
                            free(c_sect->proxy_key);

                    c_sect->proxy_key = strdup(entry.val);
            } else
                if (!strcasecmp(entry.var, INI_ENTRY_PROXY_SSH_FORCE_AUTH)) {
                    chk_inivar(&c_sect->proxy_ssh_force_auth, INI_ENTRY_PROXY_SSH_FORCE_AUTH, ln);
                    c_sect->proxy_ssh_force_auth = toupper(entry.val[0]);
            } else
                if (!strcasecmp(entry.var, INI_ENTRY_PROXY_KEY_PASSPHRASE)) {
                    if (chk_inivar(&c_sect->proxy_key_passphrase, INI_ENTRY_PROXY_KEY_PASSPHRASE, ln))
                            free(c_sect->proxy_key_passphrase);

                        if (!strcasecmp(entry.val1, XEDEC_PLAIN))
                            c_sect->proxy_key_passphrase = strdup(entry.val + strlen(XEDEC_PLAIN) + 1);
                        else if (!strcasecmp(entry.val1, XEDEC_TSW01)) {
                            if (!(x = xdecrypt(entry.val + strlen(XEDEC_TSW01) + 1, XEDEC_TSW01))) {
                                printl(LOG_CRIT, "LN: [%d] Detected wrong encryption hash!", ln);
                                mexit(1, pfile_name, tfile_name);
                            }
                            c_sect->proxy_key_passphrase = strdup(x);
                            free(x);
                        } else {
                            printl(LOG_CRIT, "LN: [%d] Malformed INI-file entry: [%s]",
                                ln, INI_ENTRY_PROXY_KEY_PASSPHRASE);
                            mexit(1, pfile_name, tfile_name);
                        }
            } else
                if (!strcasecmp(entry.var, INI_ENTRY_SECTION_BALANCE)) {
                    if (!strcasecmp(entry.val, INI_ENTRY_SECTION_BALANCE_NONE))
                        c_sect->section_balance = SECTION_BALANCE_NONE;
                    else if (!strcasecmp(entry.val, INI_ENTRY_SECTION_BALANCE_FAILOVER))
                        c_sect->section_balance = SECTION_BALANCE_FAILOVER;
                    else if (!strcasecmp(entry.val, INI_ENTRY_SECTION_BALANCE_ROUNDROBIN))
                        c_sect->section_balance = SECTION_BALANCE_ROUNDROBIN;
                    else {
                        printl(LOG_WARN, "Unknown section balance mode: [%s], setting default: Failover", entry.val);
                        c_sect->section_balance = SECTION_BALANCE_FAILOVER;
                    }
            } else
                /* -- Parse nit_* entries --------------------------------------------------------------------------- */
                if (!strcasecmp(entry.var, NS_INI_ENTRY_NIT_POOL)) {
                    c_sect->nit_domain = strdup(entry.val1);
                    c_sect->nit_ipaddr = str2inet(entry.mod1, NULL);
                    int m;
                    /* Build IPv4 address netmask based on CIDR */
                    if ((m = strtol(entry.val2, NULL, 10)) && m < 33) {
                        SIN4_FAMILY(c_sect->nit_ipmask) = AF_INET;
                        S4_ADDR(c_sect->nit_ipmask) = htonl(~(0xFFFFFFFF >> m));
                    } else
                        c_sect->nit_ipmask = str2inet(entry.val2, NULL);
            } else {
                target_type = INI_TARGET_NOTSET;
                /* -- Parse target_* entries: var=val1[:mod1[-mod2]]/val2 ------------------------------------------- */
                if (!strcasecmp(entry.var, INI_ENTRY_TARGET_HOST)) target_type = INI_TARGET_HOST;
                else if (!strcasecmp(entry.var, INI_ENTRY_TARGET_DOMAIN)) target_type = INI_TARGET_DOMAIN;
                else if (!strcasecmp(entry.var, INI_ENTRY_TARGET_NETWORK)) target_type = INI_TARGET_NETWORK;
                else if (!strcasecmp(entry.var, INI_ENTRY_TARGET_RANGE)) target_type = INI_TARGET_RANGE;
                if (target_type) {
                    c_targ = (struct ini_target *)malloc(sizeof(struct ini_target));
                    c_targ->name = NULL;
                    c_targ->target_type = target_type;
                    /* Default values */
                    memset(&c_targ->ip1, 0, sizeof(struct sockaddr_storage));
                    memset(&c_targ->ip2, 0, sizeof(struct sockaddr_storage));
                    SIN4_FAMILY(c_targ->ip1) = AF_INET;
                    SIN4_FAMILY(c_targ->ip2) = AF_INET;

                    switch(target_type) {
                        case INI_TARGET_DOMAIN:
                            d = entry.val;
                            strsep(&d, "±§!@#$%^&*()_+=`~,<>/\\|{}[]:\"'");     /* Delete unwanted chars from domain */
                            c_targ->name = strdup(entry.val);                   /* Domain */
                        break;

                        case INI_TARGET_HOST:
                            d = entry.val;
                            strsep(&d, "±§!@#$%^&*()_+=`~,<>/\\|{}[]:\"'");     /* Delete unwanted chars from domain */
                            if (!inet_pton(AF_INET, entry.val, &SIN4_ADDR(c_targ->ip1)))            /* Not an IPv4 */
                                if (!inet_pton(AF_INET6, entry.val, &SIN6_ADDR(c_targ->ip1))) {     /* Not an IPv6 */
                                    c_targ->name = strdup(entry.val);                               /* Hostname? */
                                    break;
                                }

                        default:
                            /* INI_TARGET_NETWORK / INI_TARGET_HOST / INI_TARGET_RANGE */
                            c_targ->ip1 = str2inet(entry.val1, entry.mod1);

                            if (target_type == INI_TARGET_NETWORK) {
                                if (!entry.val2) {
                                    printl(LOG_CRIT,
                                        "LN: [%d] Netmask is mandatory: [%s] = [%s]",
                                        ln, entry.var, entry.val);
                                    mexit(1, pfile_name, tfile_name);
                                }

                                if (strchr(entry.val2, '.')) {
                                    /* Long netmask format, e.g.: 255.0.0.0 */
                                    c_targ->ip2 = str2inet(entry.val2, entry.mod2);
                                    break;
                                }

                                int m = strtol(entry.val2, NULL, 10);
                                /* Build IPv4 address netmask based on CIDR */
                                if (m < 33 && m >= 0) {
                                    SIN4_FAMILY(c_targ->ip2) = AF_INET;
                                    S4_ADDR(c_targ->ip2) = m == 32 ? 0xFFFFFFFF : htonl(~(0xFFFFFFFF >> m));
                                    break;
                                } else {
                                    printl(LOG_CRIT,
                                        "LN: [%d] 0 < Netmask <= 32: [%s] = [%s]",
                                        ln, entry.var, entry.val);
                                    mexit(1, pfile_name, tfile_name);
                                }
                            }

                            /* INI_TARGET_RANGE */
                            if (entry.val2)
                                c_targ->ip2 = str2inet(entry.val2, entry.mod2);
                    }

                    /* Set defined ports range or default one: 0-65535 */
                    if (c_targ->ip1.ss_family == AF_INET) {
                        SIN4_PORT(c_targ->ip1) = entry.mod1 ? htons((int)strtol(entry.mod1, (char **)NULL, 10)) : 0;
                        SIN4_PORT(c_targ->ip2) = entry.mod2 ? htons((int)strtol(entry.mod2, (char **)NULL, 10)) : 0xFFFF;
                    } else if (c_targ->ip1.ss_family == AF_INET6) {
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

    if (proxy_server) free(proxy_server);
    if (proxy_port && fproxy_port) free(proxy_port);

    create_chains(ini_root, chain_root);

    fclose(fini);
    return ini_root;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int create_chains(struct ini_section *ini, struct chain_list *chain) {
    struct ini_section *s, *sts;
    struct proxy_chain *sc, *st;
    struct chain_list *c, *cc;
    char *chain_section, *p;

    c = chain;
    while (c) {
        if (c->txt_section[0]) {
            if ((s = getsection(ini, c->txt_section))) {
                printl(LOG_VERB, "Section: [%s] has a chain link: [%s]", c->txt_section, c->txt_chain);

                sc = s->p_chain;
                p = c->txt_chain;
                while ((chain_section = strsep(&p, ",")) != NULL) {
                    if ((sts = getsection(ini, chain_section))) {
                        st = (struct proxy_chain *)malloc(sizeof(struct proxy_chain));
                        st->chain_member = sts;
                        st->next = NULL;
                        if (!sc) s->p_chain = st; else sc->next = st;
                        sc = st;
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

    if (!name || !name[0]) {
        printl(LOG_VERB, "getsection(): Empty section name queried!");
        return NULL;
    }

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
void show_ini(struct ini_section *ini, int loglvl) {
    /* Debug only function to print parsed INI-file. TODO: Remove in release? */

    struct ini_section *s;
    struct proxy_chain *c;
    struct ini_target *t;
    char ip1[INET_ADDRPORTSTRLEN], ip2[INET_ADDRPORTSTRLEN];

    const char *ini_targets[5] = {
        INI_ENTRY_TARGET_NOTSET, INI_ENTRY_TARGET_HOST, INI_ENTRY_TARGET_DOMAIN,
        INI_ENTRY_TARGET_NETWORK, INI_ENTRY_TARGET_RANGE
    };

    const char *ini_balance[3] = {
        INI_ENTRY_SECTION_BALANCE_NONE, INI_ENTRY_SECTION_BALANCE_FAILOVER, INI_ENTRY_SECTION_BALANCE_ROUNDROBIN
    };


    printl(LOG_VERB, "Show INI-Configuration");

    s = ini;
    while (s) {
        /* Display section */
        printl(loglvl,
            "SHOW Section: [%s] Balance: [%s] Proxy: [%s] Type: [%c] User/Password: [%s/%s], Key: [%s], Force auth: [%c]",
            s->section_name, ini_balance[s->section_balance], inet2str(&s->proxy_server, ip1), s->proxy_type,
            s->proxy_user?:"", s->proxy_password ? "********" : "", s->proxy_key, s->proxy_ssh_force_auth);

        /* Display Socks chain */
        if (s->p_chain) {
            printl(loglvl, "Proxy Chain:");
            c = s->p_chain;
            while (c) {
                printl(loglvl, "[%s] ->", c->chain_member->section_name);
                c = c->next;
            }
            printl(loglvl, "-> [%s]", s->section_name);
        }

        /* Display NIT pool */
        if (s->nit_domain)
            printl(loglvl, "SHOW NIT Pool Domain: [%s], IP/Mask: [%s/%s]",
                s->nit_domain, inet2str(&s->nit_ipaddr, ip1), inet2str(&s->nit_ipmask, ip2));

        /* Display target entries */
        t = s->target_entry;
        while (t) {
            printl(loglvl, "SHOW IP1: [%s] IP2: [%s] Name: [%s] Type: [%s]",
                inet2str(&t->ip1, ip1), inet2str(&t->ip2, ip2), t->name ? t->name : "", ini_targets[t->target_type]);
            t = t->next;
        }
        s = s->next;
    }
}

/* ------------------------------------------------------------------------------------------------------------------ */
struct ini_section *delete_ini(struct ini_section *ini) {
    /* Clean INI-data */

    struct ini_section *s;
    struct proxy_chain *c, *cc;
    struct ini_target *t, *tt;
    char ip1[INET_ADDRPORTSTRLEN], ip2[INET_ADDRPORTSTRLEN];

    printl(LOG_VERB, "Delete INI-configuration");

    while (ini) {
        printl(LOG_VERB, "DELETE Section: [%s]", ini->section_name);

        /* Delete proxy chain */
        if (ini->p_chain) {
            printl(LOG_VERB, "DELETE Proxy Chain:");
            c = ini->p_chain;
            while (c) {
                printl(LOG_VERB, "[%s] ->", c->chain_member->section_name);
                cc = c->next;
                free(c);
                c = cc;
            }
        } else
            printl(LOG_VERB, "No Proxy Chain detected");

        /* Delete target_* entries */
        t = ini->target_entry;
        while (t) {
            printl(LOG_VERB, "DELETE IP1: [%s] IP2: [%s] Port1: [%d] Port2: [%d] Name: [%s] Type: [%d]",
                inet2str(&t->ip1, ip1), inet2str(&t->ip2, ip2),
                t->ip1.ss_family == AF_INET ? ntohs(SIN4_PORT(t->ip1)) : ntohs(SIN6_PORT(t->ip1)),
                t->ip1.ss_family == AF_INET ? ntohs(SIN4_PORT(t->ip2)) : ntohs(SIN6_PORT(t->ip2)),
                t->name ? t->name : "", t->target_type);
            tt = t->next;
            if (t->name && t->name[0]) free(t->name);
            free(t);
            t = tt;
        }

        /* Delete proxy_* entries */
        s = ini->next;
        if (ini->proxy_user && ini->proxy_user[0]) free(ini->proxy_user);
        if (ini->proxy_password && ini->proxy_password[0]) free(ini->proxy_password);
        if (ini->nit_domain && ini->nit_domain[0]) free(ini->nit_domain);

        /* Delete the section name */
        if (ini->section_name && ini->section_name[0]) free(ini->section_name);
        free(ini);
        ini = s;
    }
    return (ini);
}

/* ------------------------------------------------------------------------------------------------------------------ */
int pushback_ini(struct ini_section **ini, struct ini_section *target) {
    struct ini_section *c = *ini;

    if (!c || !target->next) return 1;              /* We don't need to move anything */

    if (c == target) *ini = c->next;
    while (c->next) {
        if (c->next == target) c->next = c->next->next;
        c = c->next;
    }
    c->next = target;
    target->next = NULL;

    return 0;
}

/* ------------------------------------------------------------------------------------------------------------------ */
struct ini_section *ini_look_server(struct ini_section *ini, struct uvaddr addr_u) {
    /* Lookup a Socks server ip in the list referred by ini */

    struct ini_section *s;
    struct ini_target *t;
    char buf1[INET_ADDRPORTSTRLEN], buf2[INET_ADDRPORTSTRLEN];
    char buf3[INET_ADDRPORTSTRLEN], buf4[INET_ADDRPORTSTRLEN];
    char host[HOST_NAME_MAX] = {0}, *domain = NULL;
    int domainlen = 0;


    if (addr_u.name[0]) strncpy(host, addr_u.name, sizeof(host));
    s = ini;
    while (s) {
        if (s->proxy_server.ss_family == AF_UNSPEC) {
            printl(LOG_VERB, "Section: [%s] ignored due to unspecified proxy server", s->section_name);
            s = s->next;
            continue;
        }

        t = s->target_entry;

        while (t) {
            if ((t->target_type == INI_TARGET_HOST || t->target_type == INI_TARGET_DOMAIN)) {
                /* Perform namelookup only if section has target_host or target_domain */
                if (!addr_u.name[0] &&
                    getnameinfo((struct sockaddr *)&addr_u.ip_addr, sizeof(addr_u.ip_addr),
                        host, sizeof host, 0, 0, NI_NAMEREQD) == 0 &&
                    !domain && (domain = strchr(host, '.'))) {
                        domainlen = strnlen(++domain, HOST_NAME_MAX - 1);
                        printl(LOG_VERB, "IP: [%s] resolves to: [%s] domain: [%s]",
                            inet2str(&addr_u.ip_addr, buf1), host, domain ? : "");
                }
            }

            switch(t->target_type) {
                case INI_TARGET_HOST:
                    if ((host[0] && t->name && strcasestr(t->name, host) &&
                        (addr_u.ip_addr.ss_family == AF_INET &&
                            SIN4_PORT(addr_u.ip_addr) >= SIN4_PORT(t->ip1) &&
                            SIN4_PORT(addr_u.ip_addr) <= SIN4_PORT(t->ip2))) ||
                        (addr_u.ip_addr.ss_family == AF_INET6 &&
                            SIN6_PORT(addr_u.ip_addr) >= SIN6_PORT(t->ip1) &&
                            SIN6_PORT(addr_u.ip_addr) <= SIN6_PORT(t->ip2))) {

                            printl(LOG_VERB, "Found proxy: [%s] type [%c] to serve HOST: [%s : %s] in: [%s]",
                                inet2str(&s->proxy_server, buf1), s->proxy_type, host[0] ? host : "-",
                                inet2str(&addr_u.ip_addr, buf2), s->section_name);
                            return s;
                    } else
                        if ((addr_u.ip_addr.ss_family == AF_INET &&
                                S4_ADDR(addr_u.ip_addr) == S4_ADDR(t->ip1) &&
                                SIN4_PORT(addr_u.ip_addr) >= SIN4_PORT(t->ip1) &&
                                SIN4_PORT(addr_u.ip_addr) <= SIN4_PORT(t->ip2)) ||
                            (addr_u.ip_addr.ss_family == AF_INET6 &&
                                !memcmp(S6_ADDR(addr_u.ip_addr), S6_ADDR(t->ip1), sizeof(S6_ADDR(addr_u.ip_addr))) &&
                                SIN6_PORT(addr_u.ip_addr) >= SIN6_PORT(t->ip1) &&
                                SIN6_PORT(addr_u.ip_addr) <= SIN6_PORT(t->ip2))) {

                                printl(LOG_VERB, "Found proxy: [%s] type [%c] to serve IP: [%s : %s] in: [%s]",
                                    inet2str(&s->proxy_server, buf1), s->proxy_type, host[0] ? host : "-",
                                    inet2str(&addr_u.ip_addr, buf2), s->section_name);
                                return s;
                        }
                break;

                case INI_TARGET_DOMAIN:
                    if ((domainlen && strcasestr(t->name, domain) &&
                        (addr_u.ip_addr.ss_family == AF_INET &&
                            SIN4_PORT(addr_u.ip_addr) >= SIN4_PORT(t->ip1) &&
                            SIN4_PORT(addr_u.ip_addr) <= SIN4_PORT(t->ip2))) ||
                        (addr_u.ip_addr.ss_family == AF_INET6 &&
                            SIN6_PORT(addr_u.ip_addr) >= SIN6_PORT(t->ip1) &&
                            SIN6_PORT(addr_u.ip_addr) <= SIN6_PORT(t->ip2))) {

                            printl(LOG_VERB, "Found proxy: [%s] type [%c] to serve HOST: [%s] in DOMAIN: [%s] in: [%s]",
                                inet2str(&s->proxy_server, buf1), s->proxy_type, host, t->name, s->section_name);
                            return s;
                    }
                break;

                case INI_TARGET_NETWORK:
                    if (addr_u.ip_addr.ss_family == AF_INET) {
                        /* IP & MASK_from_ini vs IP_from_ini & MASK_from_ini */
                        if ((S4_ADDR(addr_u.ip_addr) & S4_ADDR(t->ip2)) == (S4_ADDR(t->ip1) & S4_ADDR(t->ip2)) &&
                            SIN4_PORT(addr_u.ip_addr) >= SIN4_PORT(t->ip1) &&
                            SIN4_PORT(addr_u.ip_addr) <= SIN4_PORT(t->ip2)) {

                                printl(LOG_VERB,
                                    "Found proxy: [%s] type [%c] to serve IP: [%s] in NETWORK: [%s/%s] in: [%s]",
                                    inet2str(&s->proxy_server, buf1), s->proxy_type, inet2str(&addr_u.ip_addr, buf2),
                                    inet2str(&t->ip1, buf3), inet2str(&t->ip2, buf4), s->section_name);
                                return s;
                        }
                    } else if (addr_u.ip_addr.ss_family == AF_INET6) {
                        /* IPv6 & MASK_from_ini vs IPv6_from_ini & MASK_from_ini */
                        int b;
                        for (b = 0; b < 16; ++b)
                            if ((S6_ADDR(addr_u.ip_addr)[b] & S6_ADDR(t->ip2)[b]) != (S6_ADDR(t->ip1)[b] & S6_ADDR(t->ip2)[b]))
                                break;

                        printl(LOG_VERB, "Found proxy: [%s] type [%c] to serve IP: [%s] in NETWORK: [%s/%s] in: [%s]",
                            inet2str(&s->proxy_server, buf1), s->proxy_type, inet2str(&addr_u.ip_addr, buf2),
                            inet2str(&t->ip1, buf3), inet2str(&t->ip2, buf4), s->section_name);
                        return s;
                    }
                break;

                case INI_TARGET_RANGE:
                    if ((addr_u.ip_addr.ss_family == AF_INET &&
                            ntohl(S4_ADDR(addr_u.ip_addr)) >= ntohl(S4_ADDR(t->ip1)) &&
                            ntohl(S4_ADDR(addr_u.ip_addr)) <= ntohl(S4_ADDR(t->ip2)) &&
                            SIN4_PORT(addr_u.ip_addr) >= SIN4_PORT(t->ip1) &&
                            SIN4_PORT(addr_u.ip_addr) <= SIN4_PORT(t->ip2)) ||
                        (addr_u.ip_addr.ss_family == AF_INET6 &&
                            memcmp(S6_ADDR(addr_u.ip_addr), S6_ADDR(t->ip1), sizeof(S6_ADDR(addr_u.ip_addr))) > 0 &&
                            memcmp(S6_ADDR(addr_u.ip_addr), S6_ADDR(t->ip2), sizeof(S6_ADDR(addr_u.ip_addr))) < 0 &&
                            SIN6_PORT(addr_u.ip_addr) >= SIN6_PORT(t->ip1) &&
                            SIN6_PORT(addr_u.ip_addr) <= SIN6_PORT(t->ip2))) {

                            printl(LOG_VERB, "Found proxy: [%s] type [%c] to serve IP: [%s] in RANGE: [%s/%s] in: [%s]",
                                inet2str(&s->proxy_server, buf1), s->proxy_type, inet2str(&addr_u.ip_addr, buf2),
                                inet2str(&t->ip1, buf3), inet2str(&t->ip2, buf4), s->section_name);
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
        printl(LOG_VERB, "LN: [%d] Updating default or already defined [%s] variable", ln, vi);
        return 1;
    }

    return 0;
}
