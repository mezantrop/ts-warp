/* ------------------------------------------------------------------------------------------------------------------ */
/* TS-Warp - Transparent SOCKS protocol Wrapper                                                                       */
/* ------------------------------------------------------------------------------------------------------------------ */

/* Copyright (c) 2021, 2022, Mikhail Zakharov <zmey20000@yahoo.com>

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


/* -- Main program file --------------------------------------------------------------------------------------------- */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <sys/types.h>
#include <pwd.h>

#if defined(linux)
#include <netinet/in.h>
#include <linux/netfilter_ipv4.h>
#endif

#include "ts-warp.h"
#include "network.h"
#include "utility.h"
#include "socks.h"
#include "inifile.h"
#include "logfile.h"
#include "pidfile.h"
#include "natlook.h"


/* ------------------------------------------------------------------------------------------------------------------ */
int loglevel = LOG_LEVEL_DEFAULT;
FILE *lfile = NULL;
char *ifile_name = INI_FILE_NAME;
char *lfile_name = LOG_FILE_NAME;
char *pfile_name = PID_FILE_NAME;

int cn = 1;                                                             /* Active clients number */
pid_t pid, mpid;                                                        /* Current and main daemon PID */
int isock, ssock, csock;                                                /* Sockets for in/out/clients */
ini_section *ini_root;                                                  /* Root section of the INI-file */
 
#if !defined(linux)
    int pfd;                                                            /* PF device-file on *BSD */
#endif


/* ------------------------------------------------------------------------------------------------------------------ */
int main(int argc, char* argv[]) { 
/* Usage:
  ts-warp -i IP:Port -c file.ini -l file.log -v 0-4 -d -p file.pid -f -u user -h

Version:
  TS-Warp-X.Y.Z

All parameters are optional:
  -i IP:Port      Incoming local IP address and port
  -c file.ini     Configuration file

  -l file.log     Log filename
  -v 0..4         Log verbosity level: 0 - off, default: 3

  -d              Daemon mode
  -p file.pid     PID filename
  -f              Force start

  -u user         A user to run ts-warp, default: nobody

  -h              This message */

    int flg;                                                            /* Command-line options flag */
    char *iaddr = LISTEN_DEFAULT;                                       /* Our (incomming) address and... */
    char *iport = LISTEN_PORT;                                          /* ...a port to accept clients */
    int l_flg = 0;                                                      /* User didn't set the log file */
    int d_flg = 0;                                                      /* Daemon mode */
    int f_flg = 0;                                                      /* Force start */

    #if !defined(__APPLE__)
        char *runas_user = RUNAS_USER;                                  /* A user to run ts-warp */
    #endif

    struct addrinfo ihints, *ires = NULL;                               /* TS-Warp incoming address info structures */
    ini_section *s_ini;                                                 /* Current section of the INI-file */
    unsigned char auth_method;                                          /* SOCKS5 accepted auth method */

    struct sockaddr caddr;                                              /* Client address */
    socklen_t caddrlen;                                                 /* Client address len */
    struct sockaddr daddr;                                              /* Client destination address */

    fd_set rfd;
    struct timeval tv;

    char buf[BUF_SIZE];                                                 /* Multipurpose buffer */
    char suf[STR_SIZE];                                                 /* String buffer */
    int ret;                                                            /* Various function return codes */
    int rec, snd;                                                       /* received/sent bytes */


    while ((flg = getopt(argc, argv, "i:c:l:v:dp:fu:h")) != -1)
        switch(flg) {
            case 'i':                                                   /* Our IP/name */
                iaddr = strsep(&optarg, ":");                           /* IP:PORT */
                if (optarg) iport = optarg;
                break;
            case 'c':                                                   /* INI-file */
                ifile_name = optarg;
                break;
            case 'l': 
                l_flg = 1; lfile_name = optarg;                         /* Logfile */
                break;
            case 'v':                                                   /* Log verbosity */
                loglevel = toint(optarg);
                if (loglevel < LOG_NONE || loglevel > LOG_VERB) {
                    fprintf(stderr, "Wrong -v verbosity level value: [%s]\n", optarg);
                    usage(1);
                }
                break;
            case 'd':                                                   /* Daemon mode */
                d_flg = 1; 
                break;
            case 'p':
                pfile_name = optarg;                                    /* PID-file */
                break;
            case 'f':                                                   /* Force start */
                f_flg = 1; 
                break;
            case 'u':
                #if defined(__APPLE__)
                    fprintf(stderr, "Warning: -u option under macOS is not available\n");
                #else
                    runas_user = optarg;
                #endif 
                break;
            case 'h':                                                   /* Help */
            default:
                (void)usage(0);
        }

    if (!iaddr[0]) iaddr = LISTEN_DEFAULT;
    if (!iport[0]) iport = LISTEN_PORT;

    /* Open log-file */
    if (!d_flg && !l_flg) {
        lfile = stdout;
        printl(LOG_INFO, "Log file: [STDOUT], verbosity level: [%d]", loglevel);
    } else if (!(lfile = fopen(lfile_name, "a"))) {
        printl(LOG_WARN, "Unable to open log: [%s], now trying: [%s]", lfile_name, LOG_FILE_NAME);
        lfile_name = LOG_FILE_NAME;
        if (!(lfile = fopen(lfile_name, "a"))) {
            printl(LOG_CRIT, "Unable to open the default log: [%s]", lfile_name);
            exit(1);
        }
        printl(LOG_INFO, "Log file: [%s], verbosity level: [%d]", lfile_name, loglevel);
    }
    printl(LOG_INFO, "ts-warp incoming address: [%s:%s]", iaddr, iport);

    #if !defined(linux)
        pfd = pf_open();                                                /* Open PF device-file on *BSD */
    #endif

    #if !defined(__APPLE__)  
        struct passwd *pwd = getpwnam(runas_user);
    #endif

    if (d_flg) {
        /* -- Daemonizing ------------------------------------------------------------------------------------------- */
        signal(SIGHUP, trap_signal);
        signal(SIGINT, trap_signal);
        signal(SIGQUIT, trap_signal);
        signal(SIGTERM, trap_signal);
        signal(SIGCHLD, trap_signal);

        if ((pid = fork()) == -1) {
            printl(LOG_CRIT, "Daemonizing failed. The 1-st fork() failed");
            exit(1);
        }
        if (pid > 0) exit(0);
        if (setsid() < 0) {
            printl(LOG_CRIT, "Daemonizing failed. Fatal setsid()");
            exit(1);
        }
        if ((pid = fork()) == -1) {
            printl(LOG_CRIT, "Daemonizing failed. The 2-nd fork() failed");
            exit(1);
        }
        if (pid > 0) exit(0);

        printl(LOG_CRIT, "%s-%s daemon started", PROG_NAME, PROG_VERSION);
        #if defined(__APPLE__)
            mpid = mk_pidfile(pfile_name, f_flg, 0, 0);
        #else
            mpid = mk_pidfile(pfile_name, f_flg, pwd->pw_uid, pwd->pw_gid);
        #endif
    }

    #if !defined(__APPLE__)
        if (setuid(pwd->pw_uid) && setgid(pwd->pw_gid)) {
            printl(LOG_CRIT, "Failed to lower privilege level to UID:GID [%d:%d]", pwd->pw_uid, pwd->pw_gid);
            exit(1);
        }
    #endif

    /* -- Try validating our address for incoming connections ------------------------------------------------------- */
    memset(&ihints, 0, sizeof ihints);
    ihints.ai_family = PF_UNSPEC;
    ihints.ai_socktype = SOCK_STREAM;
    ihints.ai_flags = AI_PASSIVE;
    if ((ret = getaddrinfo(iaddr, iport, &ihints, &ires)) > 0) {
        printl(LOG_CRIT, "Error resolving ts-warp address [%s]: %s", iaddr, gai_strerror(ret));
        mexit(1, pfile_name);
    }

    printl(LOG_INFO, "ts-warp address [%s] succesfully resolved to [%s]", iaddr, inet2str(ires->ai_addr, buf));

    ini_root = read_ini(ifile_name);
    show_ini(ini_root);

    /* -- Create socket for the incoming connections ---------------------------------------------------------------- */
    if ((isock = socket(ires->ai_family, ires->ai_socktype, ires->ai_protocol)) == -1) {
        printl(LOG_CRIT, "Error creating a socket for incoming connections");
        mexit(1, pfile_name);
    }
    printl(LOG_VERB, "Our socket for incoming connections created");

    #if (WITH_TCP_NODELAY)
        int tpc_ndelay = 1;
        if (setsockopt(isock, IPPROTO_TCP, TCP_NODELAY, (int *)&tpc_ndelay, sizeof(int)) == -1)
            printl(LOG_WARN, "Error setting TCP_NODELAY socket option for incoming connections");
    #endif

    /* -- Bind incoming connections socket -------------------------------------------------------------------------- */
    int raddr = 1;
    if (setsockopt(isock, SOL_SOCKET, SO_REUSEADDR, &raddr, sizeof(raddr))) {
        printl(LOG_CRIT, "Error setting incomming socket to be reusable");
        close(isock);
        mexit(1, pfile_name);
    }
    if (bind(isock, ires->ai_addr, ires->ai_addrlen) < 0) {
        printl(LOG_CRIT, "Error binding socket for the incoming connections");
        close(isock);
        mexit(1, pfile_name);
    }
    printl(LOG_VERB, "The socket for incoming connections succesfully bound");

    /* -- Start listening for clients ------------------------------------------------------------------------------- */
    if (listen(isock, SOMAXCONN) == -1) {
        printl(LOG_CRIT, "Error listening the socket for incoming connections");
        close(isock);
        mexit(1, pfile_name);
    }
    printl(LOG_INFO, "Listening for incoming connections");

    /* -- Process clients ------------------------------------------------------------------------------------------- */
    while (1) {
        caddrlen = sizeof caddr;
        memset(&caddr, 0, caddrlen);
        if ((csock = accept(isock, &caddr, &caddrlen)) < 0) {
            printl(LOG_CRIT, "Error accepting incoming connection");
            return 1;
        }

        printl(LOG_INFO, "Client: [%d], IP: [%s] accepted",
            cn++, inet2str(&caddr, buf));
        
        if ((pid = fork()) == -1) {
            printl(LOG_CRIT, "Failed fork() to serve a client request");
            mexit(1, pfile_name);
        }
        if (pid > 0) {                                                  /* Main (parent process) */
            setpgid(pid, mpid);
            close(csock);
        }

        if (pid == 0) {
            /* -- Client processing (child) ------------------------------------------------------------------------- */
            close(isock);

            pid = getpid();
            printl(LOG_VERB, "A new client process started");

            /* Get the client original destination from NAT --------------------------------------------------------- */
            socklen_t daddrlen = sizeof daddr;                          /* Client dest address len */
#if defined(linux)
            /* On Linux && IPTABLES */
            memset(&daddr, 0, daddrlen); 
            daddr.sa_family = caddr.sa_family;
            ret = getsockopt(csock, SOL_IP, SO_ORIGINAL_DST, &daddr, &daddrlen);
#else
            /* On *BSD with PF */
            ret = nat_lookup(pfd, &caddr, ires->ai_addr, &daddr);
#endif
            if (ret != 0) {
                printl(LOG_WARN, "Failed to find the real destination IP, trying to get it from the socket");
                getpeername(csock, &daddr, &daddrlen);
            }

            printl(LOG_INFO, "The client destination address is: [%s]", inet2str(&daddr, buf));

            /* Find SOCKS server to serve the destination address in INI file */
            s_ini = ini_look_server(ini_root, daddr);
            if (!s_ini) {
                /* No SOCKS-proxy server found for the destinbation IP */
                printl(LOG_WARN, "No SOCKS server is defined for the destination: [%s]", inet2str(&daddr, buf));

                if ((daddr.sa_family == AF_INET && S4_ADDR(daddr) == S4_ADDR(*ires->ai_addr)) ||
                    (daddr.sa_family == AF_INET6 && !memcmp(S6_ADDR(daddr), S6_ADDR(*ires->ai_addr), sizeof(S6_ADDR(daddr))))) {
                        /* Desination address:port is the same as ts-warp income ip:port, i.e., a client contacted 
                        ts-warp dirctly: no NAT/redirection */
                        printl(LOG_WARN, "Dropping loop connection with ts-warp");
                        close(csock);
                        exit(1);
                }

                /*  Direct connection with the destination address bypassing SOCKS */
                printl(LOG_INFO, "Making a direct connection with the destination address: [%s]", inet2str(&daddr, buf));
                if ((ssock = connect_desnation(daddr)) == -1) {
                    printl(LOG_WARN, "Unable to connect with destination: [%s]", inet2str(&daddr, buf));
                    close(csock);
                    exit(1);
                }

                printl(LOG_INFO, "Succesfully connected with desination address: [%s]", inet2str(&daddr, buf));
            } else
                /* We have found a SOCKS-proxy server for the destination */
                if ((daddr.sa_family == AF_INET && S4_ADDR(daddr) == S4_ADDR(*ires->ai_addr)) ||
                    (daddr.sa_family == AF_INET6 && !memcmp(S6_ADDR(daddr), S6_ADDR(*ires->ai_addr), sizeof(S6_ADDR(daddr))))) {
                        /* Desination address:port is the same as ts-warp income ip:port, i.e., a client contacted 
                        ts-warp directly: no NAT/redirection */                    
                        printl(LOG_INFO, "Connecting the client with SOCKS server directly");
                        if ((ssock = connect_desnation(s_ini->socks_server)) == -1) {
                            printl(LOG_WARN, "Unable to connect with destination: [%s]", inet2str(&daddr, buf));
                            close(csock);
                            exit(1);
                        }
                    }
            else {
                /* Start SOCKS proto -------------------------------------------------------------------------------- */

                if (s_ini->proxy_chain) {
                    /* SOCKS chain */

                    struct socks_chain *sc = s_ini->proxy_chain;

                    printl(LOG_INFO, "Connecting a CHAIN: [%s] SOCKS server: [%s]",
                        sc->chain_member->section_name,
                        inet2str(&sc->chain_member->socks_server, buf));

                    /* Connect the first member of the chain */
                    if ((ssock = connect_desnation(sc->chain_member->socks_server)) == -1) {
                        printl(LOG_WARN, "Unable to connect with CHAIN SOCKS server: [%s]", 
                            inet2str(&sc->chain_member->socks_server, buf));
                        close(csock);
                        exit(1);
                    }

                    while (sc) {
                        if (sc->chain_member->socks_version == PROXY_PROTO_SOCKS_V5) {
                            switch (auth_method = socks5_hello(ssock, AUTH_METHOD_NOAUTH, AUTH_METHOD_UNAME, AUTH_METHOD_NOACCEPT)) {
                                case AUTH_METHOD_NOAUTH:
                                    /* No authentication required */
                                    break;
                                case AUTH_METHOD_UNAME:
                                    /* Perform user/password auth */
                                    if (socks5_auth(ssock, sc->chain_member->socks_user, sc->chain_member->socks_password)) {
                                        printl(LOG_WARN, "CHAIN SOCKS5 server rejected user: [%s]", sc->chain_member->socks_user);
                                        close(csock);
                                        exit(1);
                                    }
                                    break;
                                case AUTH_METHOD_GSSAPI:
                                case AUTH_METHOD_CHAP:
                                case AUTH_METHOD_CRAM:
                                case AUTH_METHOD_SSL:
                                case AUTH_METHOD_NDS:
                                case AUTH_METHOD_MAF:
                                case AUTH_METHOD_JPB:
                                    printl(LOG_CRIT, "CHAIN SOCKS5 server accepted unsupported auth-method: [%d]", auth_method);
                                    close(csock);
                                    exit(1);
                                case AUTH_METHOD_NOACCEPT:
                                default:
                                    printl(LOG_WARN, "No auth methods were accepted by CHAIN SOCKS5 server");
                                    close(csock);
                                    exit(1);
                            }

                            if (sc->next) {
                                /* We want to connect with the next chain member */                                
                                printl(LOG_VERB, "Initiate CHAIN SOCKS5 protocol: request [%s] -> [%s]",
                                    inet2str(&sc->chain_member->socks_server, suf),
                                    inet2str(&sc->next->chain_member->socks_server, buf));

                                if (socks5_request(ssock, SOCKS5_CMD_TCPCONNECT,
                                    sc->next->chain_member->socks_server.sa_family == AF_INET ? SOCKS5_ATYPE_IPV4 : SOCKS5_ATYPE_IPV6,
                                    &sc->next->chain_member->socks_server) > 0) {
                                        printl(LOG_WARN, "CHAIN SOCKS5 server returned an error");
                                        close(csock);
                                        exit(1);
                                }
                            } else {
                                /* We are at the end of the chain, so connect with the section server */
                                printl(LOG_VERB, "Initiate CHAIN SOCKS5 protocol: request [%s] -> [%s]",
                                    inet2str(&sc->chain_member->socks_server, suf),
                                    inet2str(&s_ini->socks_server, buf));

                                if (socks5_request(ssock, SOCKS5_CMD_TCPCONNECT,
                                    s_ini->socks_server.sa_family == AF_INET ? SOCKS5_ATYPE_IPV4 : SOCKS5_ATYPE_IPV6,
                                    &s_ini->socks_server) > 0) {
                                        printl(LOG_WARN, "SOCKS5 server returned an error");
                                        printl(LOG_WARN, "CHAIN SOCKS5 server returned an error");
                                        close(csock);
                                        exit(1);
                                }
                                goto single_server;
                            }
                        } else if (sc->chain_member->socks_version == PROXY_PROTO_SOCKS_V4) {
                            if (sc->next) {
                                /* We want to connect with the next chain member */
                                printl(LOG_VERB, "Initiate CHAIN SOCKS4 protocol: request [%s] -> [%s]",
                                    inet2str(&sc->chain_member->socks_server, suf),
                                    inet2str(&sc->next->chain_member->socks_server, buf));

                                if (socks4_request(ssock, SOCKS4_CMD_TCPCONNECT, (struct sockaddr_in *)&sc->next->chain_member->socks_server,
                                        sc->next->chain_member->socks_user) > 0) {
                                            printl(LOG_WARN, "CHAIN SOCKS4 server returned an error");
                                            close(csock);
                                            exit(1);
                                }
                            } else {
                                /* We are at the end of the chain, so connect with the section server */
                                printl(LOG_VERB, "Initiate CHAIN SOCKS4 protocol: request [%s] -> [%s]",
                                    inet2str(&sc->chain_member->socks_server, buf),
                                    inet2str(&s_ini->socks_server, buf));

                                if (socks4_request(ssock, SOCKS4_CMD_TCPCONNECT, (struct sockaddr_in *)&s_ini->socks_server,
                                        s_ini->socks_user) > 0) {
                                            printl(LOG_WARN, "SOCKS4 server returned an error");
                                            printl(LOG_WARN, "CHAIN SOCKS4 server returned an error");
                                            close(csock);
                                            exit(1);
                                }
                                goto single_server;
                            }
                        } else {
                            /* Must be cleared already by read_ini() */
                            printl(LOG_WARN, "Detected unsupported CHAIN SOCKS version: [%d]",
                                s_ini->proxy_chain->chain_member->socks_version);
                            close(csock);
                            exit(1);
                        }
                        sc = sc->next;
                    }
                } else {
                    /* Only a single SOCKS server: no chain */
                    printl(LOG_INFO, "Connecting the SOCKS server: [%s]", inet2str(&s_ini->socks_server, buf));

                    if ((ssock = connect_desnation(s_ini->socks_server)) == -1) {
                        printl(LOG_WARN, "Unable to connect with SOCKS server: [%s]", inet2str(&s_ini->socks_server, buf));
                        close(csock);
                        exit(1);
                    }

                    printl(LOG_INFO, "Succesfully connected with the SOCKS server: [%s]", inet2str(&s_ini->socks_server, buf));
single_server:
                    if (s_ini->socks_version == PROXY_PROTO_SOCKS_V5) {

                        printl(LOG_VERB, "Initiate SOCKS5 protocol: hello: [%s]", inet2str(&s_ini->socks_server, buf));

                        switch (auth_method = socks5_hello(ssock, AUTH_METHOD_NOAUTH, AUTH_METHOD_UNAME, AUTH_METHOD_NOACCEPT)) {
                            case AUTH_METHOD_NOAUTH:                    /* No authentication required */
                                break;
                            case AUTH_METHOD_UNAME:                     /* Perform user/password auth */
                                if (socks5_auth(ssock, s_ini->socks_user, s_ini->socks_password)) {
                                    printl(LOG_WARN, "SOCKS rejected user: [%s]", s_ini->socks_user);
                                    close(csock);
                                    exit(1);
                                }
                                break;
                            case AUTH_METHOD_GSSAPI:
                            case AUTH_METHOD_CHAP:
                            case AUTH_METHOD_CRAM:
                            case AUTH_METHOD_SSL:
                            case AUTH_METHOD_NDS:
                            case AUTH_METHOD_MAF:
                            case AUTH_METHOD_JPB:
                                printl(LOG_WARN, "SOCKS server accepted unsupported auth-method: [%d]", auth_method);
                                close(csock);
                                exit(1);
                            case AUTH_METHOD_NOACCEPT:
                            default:
                                printl(LOG_WARN, "No auth methods were accepted by SOCKS server");
                                close(csock);
                                exit(1);
                        }

                        printl(LOG_VERB, "Initiate SOCKS5 protocol: request [%s] -> [%s]", 
                            inet2str(&s_ini->socks_server, suf), inet2str(&daddr, buf));
                        
                        if (socks5_request(ssock, SOCKS5_CMD_TCPCONNECT, socks5_atype(s_ini, daddr), &daddr) > 0) {
                            printl(LOG_CRIT, "SOCKS server returned an error");
                            close(csock);
                            exit(1);
                        }
                    } else if (s_ini->socks_version == PROXY_PROTO_SOCKS_V4) {

                        printl(LOG_VERB, "Initiate SOCKS4 protocol: request: [%s] -> [%s]",
                            inet2str(&s_ini->socks_server, suf), inet2str(&daddr, buf));
                        if (socks4_request(ssock, SOCKS4_CMD_TCPCONNECT,  
                            (struct sockaddr_in *)&daddr, s_ini->socks_user) != SOCKS4_REPLY_OK) {
                                printl(LOG_WARN, "SOCKS4 server returned an error");
                                close(csock);
                                exit(1);
                        }
                    } else {
                        /* Must be cleared already by read_ini() */
                        printl(LOG_WARN, "Detected unsupported SOCKS version: [%d]", s_ini->socks_version);
                        close(csock);
                        exit(1);
                    }
                }
            }

            printl(LOG_VERB, "Starting connection-forward loop");

            /* -- Forward connections ------------------------------------------------------------------------------- */
            while (1) {
                FD_ZERO(&rfd);
                FD_SET(csock, &rfd);
                FD_SET(ssock, &rfd);

                tv.tv_sec = 0;
                tv.tv_usec = 100000;
                ret = select(ssock > csock ? ssock + 1: csock + 1, &rfd, 0, 0, &tv);

                if (ret < 0) break;
                if (ret == 0) continue;
                if (ret > 0) {    
                    memset(buf, 0, BUF_SIZE);
                    if (FD_ISSET(csock, &rfd)) {
                        /* Client writes */
                        rec = recv(csock, buf, BUF_SIZE, 0);
                        if (rec == 0) {
                            printl(LOG_VERB, "Connection closed by the client");
                            break;
                        }
                        if (rec == -1) {
                            printl(LOG_CRIT, "Error receving data from the client");
                            break;
                        }
                        while ((snd = send(ssock, buf, rec, 0)) == 0) {
                            printl(LOG_CRIT, "C:[0] -> S:[0] bytes");
                            usleep(100);                                /* 0.1 ms */
                            break;
                        }
                        if (snd == -1) {
                            printl(LOG_CRIT, "Error sending data to SOCKS server");
                            break;
                        }
                        if (rec != snd)
                            printl(LOG_CRIT, "C:[%d] -> S:[%d] bytes", rec, snd);
                        else
                            printl(LOG_VERB, "C:[%d] -> S:[%d] bytes", rec, snd);
                    } else {
                        /* Server writes */
                        rec = recv(ssock, buf, BUF_SIZE, 0);
                        if (rec == 0) {
                            printl(LOG_INFO, "Connection closed by SOCKS server");
                            break;
                        }
                        if (rec == -1) {
                            printl(LOG_CRIT, "Error receving data from SOCKS server");
                            break;
                        }
                        while ((snd = send(csock, buf, rec, 0)) == 0) {
                            printl(LOG_CRIT, "S:[0] -> C:[0] bytes");
                            usleep(100);
                        }
                        if (snd == -1) {
                            printl(LOG_CRIT, "Error sending data to SOCKS server");
                            break;
                        }
                        if (rec != snd)
                            printl(LOG_CRIT, "S:[%d] -> C:[%d] bytes", rec, snd);
                        else
                            printl(LOG_VERB, "S:[%d] -> C:[%d] bytes", rec, snd);
                    }
                }
            }
            shutdown(csock, SHUT_RDWR);
            shutdown(ssock, SHUT_RDWR);
            printl(LOG_INFO, "Client finished operations");
            close(csock);
            close(ssock);
            exit(0);
        }

    }
    freeaddrinfo(ires);
    return 0;
}

/* ------------------------------------------------------------------------------------------------------------------ */
void trap_signal(int sig) {
    /* Signal handler */

    int	status;                                                         /* Client process status */

    switch (sig) {
            case SIGHUP:
                ini_root = delete_ini(ini_root);
                ini_root = read_ini(ifile_name);
                show_ini(ini_root);
                break;
            case SIGINT:                                                /* Exit processes */
            case SIGQUIT:
            case SIGTERM:
                if (getpid() == mpid) {                                 /* Main daemon */
                    shutdown(isock, SHUT_RDWR);
                    close(isock);
                    #if !defined(linux)
                        pf_close(pfd);
                    #endif
                    mexit(0, pfile_name);
                } else {                                                /* Client process */
                    shutdown(csock, SHUT_RDWR);
                    shutdown(ssock, SHUT_RDWR);
                    close(ssock);
                    close(csock);
                    printl(LOG_INFO, "Client exited");
                    exit(0);
                }
            case SIGCHLD:
                /* Never use printf() in SIGCHLD processor, it causes SIGILL */
                while (wait3(&status, WNOHANG, 0) > 0) cn--;
                break;

            default:
                printl(LOG_INFO, "Got unhandled signal: %d", sig);
                break;
    }
}

/* ------------------------------------------------------------------------------------------------------------------ */
void usage(int ecode) {
    printf("Usage:\n\
  ts-warp -i IP:Port -c file.ini -l file.log -v 0-4 -d -p file.pid -f -u user -h\n\n\
Version:\n\
  %s-%s\n\n\
All parameters are optional:\n\
  -i IP:Port\t    Incoming local IP address and port\n\
  -c file.ini\t    Configuration file, default: %s\n\
  \n\
  -l file.log\t    Log filename, default: %s\n\
  -v 0..4\t    Log verbosity level: 0 - off, default %d\n\
  \n\
  -d\t\t    Daemon mode\n\
  -p file.pid\t    PID filename, default: %s\n\
  -f\t\t    Force start\n\
  \n\
  -u user\t    A user to run ts-warp, default: %s\n\
  \n\
  -h\t\t    This message\n\n", 
    PROG_NAME, PROG_VERSION, INI_FILE_NAME, LOG_FILE_NAME, LOG_LEVEL_DEFAULT, PID_FILE_NAME, RUNAS_USER);

    exit(ecode);
}
