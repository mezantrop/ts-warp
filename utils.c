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


/* -- Logging, ini-parser, printing ----------------------------------------- */
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "utils.h"
#include "socks.h"
#include "pidfile.h"

/* -------------------------------------------------------------------------- */
void usage(int ecode) {
    printf("Usage:\n\
\tts-warp [-I IP] [-i port] [-l file.log] [-v 0-4] [-d] [-c file.ini] [-h]\n\n\
Options:\n\
\t-I IP\t\tIncoming local IP address and ...\n\
\t-i port\t\t... a port number to accept connections on\n\n\
\t-l file.log\tLog filename\n\
\t-v 0..4\t\tLog verbosity level: 0 - off, default 2\n\
\t-d\t\tBecome a daemon\n\n\
\t-c file.ini\tConfiguration file\n\n\
\t-h\t\tThis message\n\n");

    exit(ecode);
}

/* -------------------------------------------------------------------------- */
void printl(int level, char *fmt, ...) {
    /* Print to log */

    /* TODO: Write PID of the process */

    time_t timestamp;
    struct tm *tstamp;
    va_list ap;
    char mesg[256];
    
    if (level > loglevel || !fmt || !fmt[0]) return;
    if (!lfile) lfile = stderr;
    if (pid <= 0) pid = getpid();
    timestamp = time(NULL);
    tstamp = localtime(&timestamp);
    va_start(ap, fmt);
    vsnprintf(mesg, sizeof mesg , fmt, ap);
    va_end(ap);
    fprintf(lfile, "%04d.%02d.%02d %02d:%02d:%02d %s [%d]:\t%s\n", 
        tstamp->tm_year + 1900, tstamp->tm_mon + 1, tstamp->tm_mday, 
        tstamp->tm_hour, tstamp->tm_min, tstamp->tm_sec, 
        LOG_LEVEL[level], pid, mesg);

    fflush(lfile);
}

/* -------------------------------------------------------------------------- */
long toint(char *str) {
    /* strtol() wrapper */

	int	in;

	in = strtol(str, (char **)NULL, 10);
    if (in == 0 && errno == EINVAL) {
        printl(LOG_CRIT, "Conversion error from string to integer: %s", str);
		usage(1);
    }
	return in;
}

/* -------------------------------------------------------------------------- */
ini_section *read_ini(char *ifile_name) {
    FILE *fini;
    char buffer[BUF_SIZE], section[STR_SIZE];
    char *s = NULL, *d = NULL, *x = NULL;   /* String manipulation pointers */
    ini_entry entry;                        /* tmp place for the parsed line */
    ini_section *ini_root = NULL, *c_sect = NULL, *l_sect = NULL;
    ini_target *c_targ = NULL, *l_targ = NULL;
    chain_list *chain_root = NULL, *chain_this = NULL, *chain_temp = NULL;
    int target_type = INI_TARGET_NOTSET;
    struct addrinfo res;
    int ln = 1;


    if (!(fini = fopen(ifile_name, "r"))) {
        printl(LOG_CRIT, "Error opening INI-file: %s", ifile_name);
        exit(1);
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
            c_sect->socks_version = PROXY_PROTO_SOCKS_V5;
            c_sect->socks_user = NULL;
            c_sect->socks_password = NULL;
            c_sect->proxy_chain = NULL;
            c_sect->target_entry = NULL;
            c_sect->next = NULL;

            if (!ini_root) ini_root = c_sect; else l_sect->next = c_sect;
            l_sect = c_sect;      /* lsect always points to the last section */
        } else {                                          /* Section entries */
            /* Remove whitespaces */
            s = d = buffer; do while(isspace(*s)) s++; while((*d++ = *s++));

            if (!*buffer) continue;             /* Skip an empty line */

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

            if (!entry.val1) {
                printl(LOG_VERB, "LN: %d INVALID", ln);
                continue;
            }

            /* TODO: hide password hash! */
            printl(LOG_VERB, "LN: %d V: %s v1: %s v2: %s m1: %s m2: %s",
                ln, entry.var, entry.val1, entry.val2, entry.mod1, entry.mod2);

            /* Parse socks_* entries */
            /* TODO: Check for duplicate variables and reject them */
            if (!strcasecmp(entry.var, INI_ENTRY_SOCKS_SERVER))
                c_sect->socks_server = *(str2inet(entry.val1, entry.mod1 ? 
                    entry.mod1 : "1080", &res, NULL));
            else if (!strcasecmp(entry.var, INI_ENTRY_SOCKS_VERSION))
                c_sect->socks_version = toint(entry.val);
            else if (!strcasecmp(entry.var, INI_ENTRY_SOCKS_USER))
                c_sect->socks_user = strdup(entry.val);
            else if (!strcasecmp(entry.var, INI_ENTRY_SOCKS_CHAIN)) {
                chain_temp = (struct chain_list *)malloc(sizeof(struct chain_list));
                chain_temp->txt_section = strdup(c_sect->section_name);
                chain_temp->txt_chain = strdup(entry.val);
                chain_temp->next = NULL;
                if (chain_this) chain_this->next = chain_temp; 
                else { chain_root = chain_temp; chain_this = chain_root; }

            } else if (!strcasecmp(entry.var, INI_ENTRY_SOCKS_PASSWORD)) {
                if (!strcasecmp(entry.val1, XEDEC_PLAIN))
                    c_sect->socks_password = strdup(entry.val + strlen(XEDEC_PLAIN) + 1);
                else if (!strcasecmp(entry.val1, XEDEC_TSW01)) {
                    x = xdecrypt(entry.val + strlen(XEDEC_TSW01) + 1, XEDEC_TSW01);
                    c_sect->socks_password = strdup(x);
                    free(x);
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
                    SIN4_PORT(c_targ->ip1) = entry.mod1 ? 
                        htons((int)strtol(entry.mod1, (char **)NULL, 10)) : 0;
                    SIN4_PORT(c_targ->ip2) = entry.mod2 ?
                        htons((int)strtol(entry.mod2, (char **)NULL, 10)) : 0xFFFF;

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
    /* TODO: Delete the chain_list referred by chain_root */

    fclose(fini);
    return ini_root;
}

/* -------------------------------------------------------------------------- */
int create_chains(struct ini_section *ini, struct chain_list *chain) {
    struct ini_section *s, *sts;
    struct socks_chain *sc, *st;
    struct chain_list *c;
    char *chain_section, *p;
    
    c = chain;
    while (c) {
        if (c->txt_section) {
            if ((s = getsection(ini, c->txt_section))) {
                printl(LOG_VERB, "Section: [%s] has a chain link: %s", 
                    c->txt_section, c->txt_chain);

                sc = s->proxy_chain; 
                p = c->txt_chain;
                while ((chain_section = strsep(&p, ",")) != NULL) {
                    if ((sts = getsection(ini, chain_section))) {
                        st = (struct socks_chain *)malloc(sizeof(struct socks_chain));
                        st->chain_member = sts;
                        st->next = NULL;
                        if (sc) sc->next = st; else s->proxy_chain = st;
                        printl(LOG_VERB, "Linking chain section: %s", 
                            st->chain_member->section_name); 
                    } else
                        printl(LOG_VERB, "Chain section: [%s] linked from: [%s] does not really exist", 
                            chain_section, s->section_name);
                }
            }
        }
        c = c->next;
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
            printl(LOG_VERB, "Section [%s] found!", name);        
            return s;
        }
        s = s->next;
    }
    printl(LOG_VERB, "Section [%s] not found!", name);
    return NULL;
}

/* -------------------------------------------------------------------------- */
int show_ini(struct ini_section *ini) {
    /* Debug only function to print out parsed INI-file (NB: IPv4 only!)
    TODO: Remove in release??? */
    
    struct ini_section *s;
    struct socks_chain *c;
    struct ini_target *t;
    char ip1[4 * sizeof "123"], ip2[4 * sizeof "123"];

    s = ini;
    while (s) {
        /* Display section */
        printl(LOG_VERB,            /* TODO: obfuscate password output! */
            "LIST: Section: %s; Server: %s:%d; Version: %d; User/Password: %s/%s",
            s->section_name, inet_ntoa(SIN4_ADDR(s->socks_server)), 
            ntohs(SIN4_PORT(s->socks_server)), s->socks_version, s->socks_user,
            s->socks_password);
            /* Display Socks chain */
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
            strncpy(ip1, inet_ntoa(SIN4_ADDR(t->ip1)), sizeof ip1);
            strncpy(ip2, inet_ntoa(SIN4_ADDR(t->ip2)), sizeof ip2);

            printl(LOG_VERB, 
                "LIST IP1: %s; IP2: %s; Port1: %d; Port2: %d; Name: %s; Type: %d",
                    ip1, ip2, ntohs(SIN4_PORT(t->ip1)), ntohs(SIN4_PORT(t->ip2)),
                    t->name?t->name:"", t->target_type);
            t = t->next;
        }
        s = s->next;
    }

    return 0;
}

/* -------------------------------------------------------------------------- */
struct ini_section *ini_look_server(struct ini_section *ini, struct sockaddr ip) {
    /* Lookup a SOCKS server ip in the list referred by ini */
    
    struct ini_section *s;
    struct ini_target *t;
    char *buf1 = NULL, *buf2 = NULL, *buf3 = NULL, *buf4 = NULL;
    char host[HOST_NAME_MAX];
    int hostlen = 0;

#if defined(linux)
    int ipaddrlen = ip.sa_family == AF_INET ? sizeof((struct sockaddr_in *)&ip) : sizeof((struct sockaddr_in6*)&ip);
    if (getnameinfo(&ip, ipaddrlen, host, sizeof host, 0, 0, 0))
#else
    if (getnameinfo(&ip, ip.sa_len, host, sizeof host, 0, 0, 0))
#endif
        printl(LOG_WARN, "Error resolving host: [%s]", inet2str(&ip, buf1));
    
    hostlen = strnlen(host, HOST_NAME_MAX);

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
#if defined(linux)
                    /* TODO: Write a custom strnstr() for linux */
                    if ((hostlen && strstr(t->name, host) && 
#else
                    if ((hostlen && strnstr(t->name, host, strnlen(t->name, HOST_NAME_MAX)) && 
#endif
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
char *inet2str(struct sockaddr *ai_addr, char *str_addr) {
    /* inet_ntop() wrapper. If str_add is NULL, memory is auto-allocated,
     don't forget to free it after usage! */

    if (!str_addr) str_addr = (char *)malloc(INET6_ADDRSTRLEN + 1);

    memset(str_addr, 0, INET6_ADDRSTRLEN + 1);

    switch (ai_addr->sa_family) {
        case AF_INET:
            return 
                (char *)inet_ntop(AF_INET, &SIN4_ADDR(*ai_addr),
                    str_addr, INET_ADDRSTRLEN);

        case AF_INET6:
            return
                (char *)inet_ntop(AF_INET6, &SIN6_ADDR(*ai_addr),
                    str_addr, INET6_ADDRSTRLEN);

        default:
            printl(LOG_CRIT, "Unrecognized address family: %d", 
                ai_addr->sa_family);
            exit(1);
    }
    return str_addr;
}

/* -------------------------------------------------------------------------- */
struct sockaddr *str2inet(char *str_addr, char *str_port, struct addrinfo *res, 
    struct addrinfo *hints) {
    
    int free_mem = 0;
    int ret;

    if (!hints) {
        hints = (struct addrinfo *)malloc(sizeof(struct addrinfo));
        memset(hints, 0, sizeof(struct addrinfo));
        hints->ai_family = PF_UNSPEC;
        hints->ai_socktype = SOCK_STREAM;
        free_mem = 1;
    }

    if ((ret = getaddrinfo(str_addr, str_port, hints, &res)) > 0) {
        printl(LOG_CRIT, "Error resolving address [%s]:[%s]: [%s]",
            str_addr, str_port, gai_strerror(ret));
        exit(1);
    }
        
    if (free_mem) free(hints);
    return res->ai_addr;
}

/* -------------------------------------------------------------------------- */
void mexit(int status, char *pid_file) {
    /* Exit program */

    kill(0, SIGTERM);
    printl(LOG_VERB, "Clients requested to exit");
    while (wait3(&status, WNOHANG, 0) > 0) ;
    printl(LOG_INFO, "Program finished");
    if (pid_file) {
        unlink(pid_file);
        printl(LOG_VERB, "PID file removed");
    }
    exit(status);
}
