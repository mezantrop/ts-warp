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


#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

/* Program name and version */
#define PROG_NAME       "TS-Warp"
#define PROG_NAME_SHORT "TSW"
#define PROG_VERSION    "001"
#define PROG_NAME_FULL  PROG_NAME " " PROG_VERSION
#define PROG_NAME_CODE  PROG_NAME_SHORT PROG_VERSION

#define XEDEC_XKEY_LEN  8
#define XEDEC_PLAIN     "PLAIN"
#define XEDEC_TSW01     "TSW01"

#define URANDOM     "/dev/urandom"

/* Used ports */
#define LISTEN_IPV4     "127.0.0.1"     /* We listen on this IPv4 address or */
#define LISTEN_IPV6     "::1"           /* on this IPv6 address */
#define LISTEN_DEFAULT  LISTEN_IPV4
#define LISTEN_PORT     "10800"         /* This is our TCP port */
#define SOCKS_PORT      "1080"          /* That is remote SOCKS server port */

/* Log verbosity levels */
#define LOG_NONE    0
#define LOG_CRIT    1
#define LOG_WARN    2
#define LOG_INFO    3
#define LOG_VERB    4
#define LOG_LEVEL_DEFAULT   LOG_INFO
#define LOG_LEVEL ((char const*[]){"NONE", "CRIT", "WARN", "INFO", "VERB"})

#define BUF_SIZE        1024 * 1024
#define STR_SIZE        255

/* sockaddr to sockaddr_in */
#define     SIN4_ADDR(sa)   ((struct sockaddr_in *)&sa)->sin_addr
#define     SIN4_PORT(sa)   ((struct sockaddr_in *)&sa)->sin_port
#define     SIN4_FAMILY(sa) ((struct sockaddr_in *)&sa)->sin_family
#if !defined(linux)
#define     SIN4_LENGTH(sa) ((struct sockaddr_in *)&sa)->sin_len
#endif
#define     S4_ADDR(sa)     ((struct sockaddr_in *)&sa)->sin_addr.s_addr
/* sockaddr to sockaddr_in6 */
#define     SIN6_ADDR(sa)   ((struct sockaddr_in6 *)&sa)->sin6_addr
#define     SIN6_PORT(sa)   ((struct sockaddr_in6 *)&sa)->sin6_port
#define     SIN6_FAMILY(sa) ((struct sockaddr_in6 *)&sa)->sin6_family
#if !defined(linux)
#define     SIN6_LENGTH(sa) ((struct sockaddr_in6 *)&sa)->sin6_len
#endif

#if defined(linux)
#define     S6_ADDR(sa)     ((struct sockaddr_in6 *)&sa)->sin6_addr.__in6_u.__u6_addr8
#else
#define     S6_ADDR(sa)     ((struct sockaddr_in6 *)&sa)->sin6_addr.__u6_addr.__u6_addr8
#endif

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 255
#endif

typedef struct ini_section {
    char *section_name;                 /* Sectionn name */
    struct sockaddr socks_server;       /* SOCKS server IP-address and Port */
    uint8_t socks_version;              /* SOCKS version: 4 | 5 */
    char *socks_user;                   /* SOCKS server username */
    char *socks_password;               /* SOCKS server user password */
    struct socks_chain *proxy_chain;    /* SOCKS proxy chain */
    struct ini_target *target_entry;    /* List of target definitions */
    struct ini_section *next;           /* The next INI-section */
} ini_section;

typedef struct ini_entry {  /* Parsed INI-entry: var=val1[[:mod1[-mod2]]/val2] */
    char *var; 
    char *val;              /* Raw value: whatever right from the '=' char */
    char *val1;
    char *mod1;
    char *mod2;
    char *val2;
} ini_entry;

#define INI_TARGET_NOTSET   0
#define INI_TARGET_HOST     1
#define INI_TARGET_DOMAIN   2
#define INI_TARGET_NETWORK  3
#define INI_TARGET_RANGE    4

typedef struct ini_target {
    int target_type;        /* Hostname, Host IP, Domain, Network, Range */
    char *name;             /* Hostname / Domain or null */
    struct sockaddr ip1;    /* Host IP, Net IP, First IP in Range or null 
                                and optional port number 0 65535 */
    struct sockaddr ip2;    /* Netmask, Last IP in Range or null + port */

    struct ini_target *next; /* The next range entry */
} ini_target;

typedef struct socks_chain {            /* SOCKS server chains */
    struct ini_section *chain_member;
    struct socks_chain *next;
} socks_chain;

typedef struct chain_list {     /* Chains as they defind in the INI */
    char *txt_section;          /* section name */
    char *txt_chain;            /* String representing sections to chain */
    struct chain_list *next;
} chain_list;

#define INI_ENTRY_SOCKS_SERVER      "socks_server"
#define INI_ENTRY_SOCKS_CHAIN       "socks_chain"
#define INI_ENTRY_SOCKS_PORT        "socks_port"
#define INI_ENTRY_SOCKS_VERSION     "socks_version"
#define INI_ENTRY_SOCKS_USER        "socks_user"
#define INI_ENTRY_SOCKS_PASSWORD    "socks_password"
#define INI_ENTRY_TARGET_HOST       "target_host"
#define INI_ENTRY_TARGET_DOMAIN     "target_domain"
#define INI_ENTRY_TARGET_NETWORK    "target_network"
#define INI_ENTRY_TARGET_RANGE      "target_range"

/* -- Global variables ------------------------------------------------------ */
extern uint8_t loglevel;
extern FILE *lfile;
extern int pid;
extern char *pfile_name;

/* -- Function prototypes --------------------------------------------------- */
void usage(int ecode);
void printl(int level, char *fmt, ...);
long toint(char *str);
ini_section *read_ini(char *ifile_name);
void show_ini(struct ini_section *ini);
struct ini_section *delete_ini(struct ini_section *ini);
struct ini_section *ini_look_server(struct ini_section *ini, struct sockaddr ip);
int create_chains(struct ini_section *ini, struct chain_list *chain);
struct ini_section *getsection(struct ini_section *ini, char *name);
char *inet2str(struct sockaddr *ai_addr, char *str_addr);
struct sockaddr *str2inet(char *str_addr, char *str_port, struct addrinfo *res, 
    struct addrinfo *hints);
char *init_xcrypt(int xkey_len);
char *xencrypt(char *xkey, char *prefix, char *text);
char *xdecrypt(char *hex_hash, char *prefix);
void mexit(int status, char *pid_file);
int chk_inivar(void *v, char *vi);
