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


/* -- INI-file processing ------------------------------------------------------------------------------------------- */
typedef struct nit {
    char *domain;                           /* Domain name */
    struct sockaddr ip_addr;                /* IP network */
    struct sockaddr ip_mask;                /* IP mask */
    int iname;                              /* Index of the last inserted record */
    char **names;                           /* Array of N char pointers to strings with hostnames. N == !IP-mask */
    struct nit *next;
} nit;

typedef struct ns_ini_entry {               /* Parsed INI-entry: var=val1:val2/mod1 */
    char *var; 
    char *val;                              /* Raw value: whatever right from the '=' char */
    char *val1;
    char *val2;
    char *mod1;
} ns_ini_entry;

/* NS-Warp INI-file entries */
#define NS_INI_ENTRY_NIT_POOL       "nit_pool"              /* nit_pool = domain.net:192.168.168.0/255.255.255.0 */

#define NS_NIT_IPV6_POOL_SIZE       0xFFFFFFFF              /* Fixed size for the NIT IPv6 pool */

/* -- Function prototypes ------------------------------------------------------------------------------------------- */
nit *read_ini(char *ifile_name);
void show_ini(struct nit *ini);
int nit_lookup_name(struct nit *nit_root, char *name, int af, struct sockaddr *ip);
int nit_lookup_ip(struct nit *nit_root, struct sockaddr *ip, char *name);
struct sockaddr rev_addr(struct sockaddr *sa);
char *reverse_ip(struct sockaddr *ip, char *rev_ip);
struct sockaddr forward_ip(char *rev_ip);