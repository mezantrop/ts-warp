/* ------------------------------------------------------------------------------------------------------------------ */
/* TS-Warp - Transparent SOCKS proxy Wrapper                                                                          */
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


/* -- INI-file processing ------------------------------------------------------------------------------------------- */
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h> 


/* ------------------------------------------------------------------------------------------------------------------ */
typedef struct ini_section {
    char *section_name;                                                 /* Section name */
    uint8_t section_balance;                                            /* Balance SOCKS server on accessibility */
    struct sockaddr_storage socks_server;                               /* SOCKS server IP-address and Port */
    uint8_t socks_version;                                              /* SOCKS version: 4 | 5 */
    char *socks_user;                                                   /* SOCKS server username */
    char *socks_password;                                               /* SOCKS server user password */
    struct socks_chain *proxy_chain;                                    /* SOCKS proxy chain */
    struct ini_target *target_entry;                                    /* List of target definitions */

    /*NIT Pool specification */
    char *nit_domain;                                                   /* NIT Domain name */
    struct sockaddr_storage nit_ipaddr;                                 /* NIT IP/address */
    struct sockaddr_storage nit_ipmask;                                 /* NIT address mask */

    struct ini_section *next;                                           /* The next INI-section */
} ini_section;

typedef struct ini_entry {          /* Parsed INI-entry: var=val1[[:mod1[-mod2]]/val2] */
    char *var; 
    char *val;                      /* Raw value: whatever right from the '=' char */
    char *val1;
    char *mod1;
    char *mod2;
    char *val2;
} ini_entry;

typedef struct ini_target {
    int target_type;                /* Hostname, Host IP, Domain, Network, Range */
    char *name;                     /* Hostname / Domain or null */
    struct sockaddr_storage ip1;    /* Host IP, Net IP, First IP in Range or null and optional port number 0 65535 */
    struct sockaddr_storage ip2;    /* Netmask, Last IP in Range or null + port */

    struct ini_target *next;                                /* The next range entry */
} ini_target;

typedef struct socks_chain {                                /* SOCKS server chains */
    struct ini_section *chain_member;
    struct socks_chain *next;
} socks_chain;

typedef struct chain_list {                                 /* Chains as they defind in the INI */
    char *txt_section;                                      /* section name */
    char *txt_chain;                                        /* String representing sections to chain */
    struct chain_list *next;
} chain_list;

/* -- INI-file entries  --------------------------------------------------------------------------------------------- */
/* Section balancing modes in memory */
#define SECTION_BALANCE_NONE        0
#define SECTION_BALANCE_FAILOVER    1                               /* Default */
#define SECTION_BALANCE_ROUNDROBIN  2

/* Section balancing modes in the INI-file */
#define INI_ENTRY_SECTION_BALANCE               "section_balance"   /* Socks section balance policy */
#define INI_ENTRY_SECTION_BALANCE_NONE          "none"              /* 0 */
#define INI_ENTRY_SECTION_BALANCE_FAILOVER      "failover"          /* 1 - Default */
#define INI_ENTRY_SECTION_BALANCE_ROUNDROBIN    "roundrobin"        /* 2 */

#define INI_ENTRY_SOCKS_SERVER      "socks_server"
#define INI_ENTRY_SOCKS_CHAIN       "socks_chain"
#define INI_ENTRY_SOCKS_VERSION     "socks_version"
#define INI_ENTRY_SOCKS_USER        "socks_user"
#define INI_ENTRY_SOCKS_PASSWORD    "socks_password"

/* Target type IDs in memory */
#define INI_TARGET_NOTSET   0
#define INI_TARGET_HOST     1
#define INI_TARGET_DOMAIN   2
#define INI_TARGET_NETWORK  3
#define INI_TARGET_RANGE    4

/* Target type IDs in the INI-file */
#define INI_ENTRY_TARGET_NOTSET      ""
#define INI_ENTRY_TARGET_HOST       "target_host"
#define INI_ENTRY_TARGET_DOMAIN     "target_domain"
#define INI_ENTRY_TARGET_NETWORK    "target_network"
#define INI_ENTRY_TARGET_RANGE      "target_range"

#define NS_INI_ENTRY_NIT_POOL       "nit_pool"              /* nit_pool = domain.net:192.168.168.0/255.255.255.0 */

/* -- Function prototypes ------------------------------------------------------------------------------------------- */
ini_section *read_ini(char *ifile_name);
void show_ini(struct ini_section *ini, int loglvl);
struct ini_section *delete_ini(struct ini_section *ini);
int pushback_ini(struct ini_section **ini, struct ini_section *target);
struct ini_section *ini_look_server(struct ini_section *ini, struct sockaddr_storage ip);
int create_chains(struct ini_section *ini, struct chain_list *chain);
struct ini_section *getsection(struct ini_section *ini, char *name);
int chk_inivar(void *v, char *vi, int d);
int socks5_atype(ini_section *ini, struct sockaddr_storage daddr);
