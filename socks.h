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


/* -- SOCKS protocol definitions ------------------------------------------------------------------------------------ */
#define PROXY_PROTO_SOCKS_V4    4
#define PROXY_PROTO_SOCKS_V5    5

/* -- SOCKS V4 ------------------------------------------------------------------------------------------------------ */
#define SOCKS4_CMD_TCPCONNECT   0x01
#define SOCKS4_CMD_TCPBIND      0x02

typedef struct {
    uint8_t ver;                /* SOCKS version */
    uint8_t cmd;                /* Command
                                    0x01: TCP Connect; 
                                    0x02: TCP port Binding */
    uint16_t dstport;           /* Dest port number in a network byte order */
    uint32_t dstaddr;           /* IPv4 Address 4 bytes in network byte order */
    unsigned char id[256];      /* Null-terminated user ID string */
} s4_request;

typedef struct {
    uint8_t nul;                /* NUL-byte */
    uint8_t status;             /* Returned status
                                    0x5A: Request granted
                                    0x5B: Request rejected or failed
                                    0x5C: Request failed: client is not running identd (or not reachable from server)
                                    0x5D: Request failed because client's identd could not confirm the user ID */
    uint16_t dstport;           /* Dest port number in a network byte order */
    uint32_t dstaddr;           /* IPv4 Address 4 bytes in network byte order */
} s4_reply;

/* SOCKS4 server replies. Only 0x5a and 0x5b are interesting */
#define SOCKS4_REPLY_OK         0x5a                /* Request granted */
#define SOCKS4_REPLY_KO         0x5b                /* Request rejected or failed */
#define SOCKS4_REPLY_KO_IDENT1  0x5c                /* Request failed because client is not running identd */
#define SOCKS4_REPLY_KO_IDENT2  0x5d                /* Request failed because client's identd couldn't confirm user */

/* -- SOCKS V5 ------------------------------------------------------------------------------------------------------ */
#define AUTH_MAX_METHODS        255

#define AUTH_METHOD_NOAUTH      0x00
#define AUTH_METHOD_GSSAPI      0x01
#define AUTH_METHOD_UNAME       0x02
/* 0x03–0x7F: methods assigned by IANA */
#define AUTH_METHOD_CHAP        0x03                /* Challenge-Handshake Auth Proto */
/*                              0x04                Unassigned */
#define AUTH_METHOD_CRAM        0x05                /* Challenge-Response Auth Method */
#define AUTH_METHOD_SSL         0x06                /* Secure Sockets Layer */
#define AUTH_METHOD_NDS         0x07                /* NDS Authentication (Novell?) */
#define AUTH_METHOD_MAF         0x08                /* Multi-Authentication Framework */
#define AUTH_METHOD_JPB         0x09                /* JSON Parameter Block */
/*                              0x0A–0x7F           Unassigned */
/*                              0x80–0xFE           Reserved for private use */
#define AUTH_METHOD_NOACCEPT    0xFF                /* No methods accepted by server */

typedef struct {
    uint8_t ver;                                    /* SOCKS version */
    uint8_t nauth;                                  /* Number of auth methods */
    uint8_t auth[AUTH_MAX_METHODS];                 /* Authentication methods */
} s5_request_hello;

typedef struct {
    uint8_t ver;                                    /* SOCKS version */
    uint8_t cauth;                                  /* Chosen auth method */
} s5_reply_hello;

typedef struct {
    uint8_t ver;                                    /* SOCKS version */
    uint8_t idlen;                                  /* Username length */
    uint8_t *id;                                    /* ID 1-255 chars */
    uint8_t pwlen;                                  /* Password length */
    uint8_t *pw;                                    /* Password 1-255 */
} s5_request_auth;

typedef struct {
    uint8_t ver;                                    /* SOCKS version */
    uint8_t status;                                 /* 0x00: OK, else KO */
} s5_reply_auth;

/* Useful SOCKS5 request field definitions of s5_request* structures */
/* Command: cmd */
#define SOCKS5_CMD_TCPCONNECT   0x01
#define SOCKS5_CMD_TCPBIND      0x02
#define SOCKS5_CMD_UDPASSOCIATE 0x03
/* Address type: atype */
#define SOCKS5_ATYPE_IPV4       0x01
#define SOCKS5_ATYPE_NAME       0x03
#define SOCKS5_ATYPE_IPV6       0x04

/* Address type: atype max length */
#define SOCKS5_ATYPE_IPV4_LEN   4
#define SOCKS5_ATYPE_NAME_LEN   256                 /* 1 + HOST_NAME_MAX */
#define SOCKS5_ATYPE_IPV6_LEN   16

typedef struct {
    uint8_t ver;                                    /* SOCKS version */
    uint8_t cmd;                                    /* Command
                                                        0x01: TCP Connect;
                                                        0x02: TCP port Binding, e.g. FTP;
                                                        0x03: associate a UDP port */
    uint8_t rsv;                                    /* 0x00: Reserved */
    uint8_t atype;                                  /* Destination address type
                                                        0x01: IPv4 address
                                                        0x03: Domain name
                                                        0x04: IPv6 address */
    uint8_t dsthost[1 + HOST_NAME_MAX + 2];         /* Destination address + port in a net byte order:
                                                        IPv4 address:   4 bytes
                                                        Domain name:    1 byte Length + 1-255 bytes Name
                                                        IPv6 address:   16 bytes
                                                        Dest port:      2 bytes */ 
} s5_request;

typedef struct {
    uint8_t ver;                                    /* SOCKS version */
    uint8_t cmd;                                    /* Command
                                                        0x01: TCP Connect;
                                                        0x02: TCP port Binding, e.g. FTP;
                                                        0x03: associate a UDP port */
    uint8_t rsv;                                    /* 0x00: Reserved */
    uint8_t atype;                                  /* Destination address type
                                                        0x01: IPv4 address
                                                        0x03: Domain name
                                                        0x04: IPv6 address */
    uint8_t dstaddr[4];                             /* Destination address
                                                        IPv4 address:   4 bytes
                                                        Domain name:    1 byte Length + 1-255 bytes Name
                                                        IPv6 address:   16 bytes */
    uint16_t dstport;                               /* Dest port number in a network byte order */ 
} s5_request_ipv4;

typedef struct {
    uint8_t ver;                                    /* SOCKS version */
    uint8_t cmd;                                    /* Command
                                                        0x01: TCP Connect;
                                                        0x02: TCP port Binding, e.g. FTP;
                                                        0x03: associate a UDP port */
    uint8_t rsv;                                    /* 0x00: Reserved */
    uint8_t atype;                                  /* Destination address type
                                                        0x01: IPv4 address
                                                        0x03: Domain name
                                                        0x04: IPv6 address */
    uint8_t dstaddr[16];                            /* Destination address
                                                        IPv4 address:   4 bytes
                                                        Domain name:    1 byte Length + 1-255 bytes Name
                                                        IPv6 address:   16 bytes */
    uint16_t dstport;                               /* Dest port number in a network byte order */
} s5_request_ipv6;

typedef struct {
    uint8_t ver;                                    /* SOCKS version */
    uint8_t cmd;                                    /* Command
                                                        0x01: TCP Connect; 
                                                        0x02: TCP port Binding, e.g. FTP;
                                                        0x03: associate a UDP port */
    uint8_t rsv;                                    /* 0x00: Reserved */
    uint8_t atype;                                  /* Destination address type */
} s5_request_short;

typedef struct {
    uint8_t ver;                                    /* SOCKS version */
    uint8_t status;                                 /* Returned status
                                                        0x00: Request granted
                                                        0x01: General failure
                                                        0x02: Connection not allowed by ruleset
                                                        0x03: Network unreachable
                                                        0x04: Host unreachable
                                                        0x05: Connection refused by destination host
                                                        0x06: TTL expired
                                                        0x07: Command not supported / protocol error
                                                        0x08: Address type not supported */
    uint8_t rsv;                                    /* 0x00: Reserved */
    uint8_t atype;                                  /* Server bound address type
                                                        0x01: IPv4 address
                                                        0x03: Domain name
                                                        0x04: IPv6 address */
    uint8_t bndaddrport[1 + HOST_NAME_MAX + 2];     /* A buffer to accomodate all types of addresses and a port */
} s5_reply;

typedef struct {
    uint8_t ver;                                    /* SOCKS version */
    uint8_t status;                                 /* Returned status
                                                        0x00: Request granted
                                                        0x01: General failure
                                                        0x02: Connection not allowed by ruleset
                                                        0x03: Network unreachable
                                                        0x04: Host unreachable
                                                        0x05: Connection refused by destination host
                                                        0x06: TTL expired
                                                        0x07: Command not supported / protocol error
                                                        0x08: Address type not supported */
    uint8_t rsv;                                    /* 0x00: Reserved */
    uint8_t atype;                                  /* Server bound address type 0x01: IPv4 address */
} s5_reply_short;

/* SOCKS5 reply statuses */
#define SOCKS5_REPLY_OK             0x00            /* Request granted */
#define SOCKS5_REPLY_KO             0x01            /* General failure */
#define SOCKS5_REPLY_DENIED         0x02            /* Connection not allowed by ruleset */
#define SOCKS5_REPLY_NET_UNREACH    0x03            /* Network unreacheable */
#define SOCKS5_REPLY_HOST_UNREACH   0x04            /* Host unreacheable */
#define SOCKS5_REPLY_CONN_REFUSED   0x05            /* Connection refused by target host */
#define SOCKS5_REPLY_TTL_EXPIRED    0x06            /* TTL expired */
#define SOCKS5_REPLY_UNSUPPORTED    0x07            /* Command unsupported / protocol error */
#define SOCKS5_REPLY_ATYPE_ERROR    0x08            /* Address type is not supported */

/* -- Function prototypes ------------------------------------------------------------------------------------------- */
int socks4_request(int socket, uint8_t cmd, struct sockaddr_in *daddr, char *user);
int socks5_hello(int socket, unsigned int auth_method, ...);
int socks5_auth(int socket, char *user, char *password);
int socks5_request(int socket, uint8_t cmd, uint8_t atype, struct sockaddr_storage *daddr);
int socks5_server_hello(int socket);
uint8_t socks5_server_request(int socket, struct sockaddr_storage *daddr, char *dname);