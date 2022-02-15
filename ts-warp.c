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


/* -- Main program file ----------------------------------------------------- */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>

#if defined(linux)
#include <netinet/in.h>
#include <linux/netfilter_ipv4.h>
#endif

#include "ts-warp.h"
#include "utils.h"
#include "socks.h"
#include "natlook.h"
#include "pidfile.h"

/* -------------------------------------------------------------------------- */
uint8_t loglevel = LOG_LEVEL_DEFAULT;
FILE *lfile = NULL;
char *ifile_name = INI_FILE_NAME;
char *lfile_name = LOG_FILE_NAME;
char *pfile_name = PID_FILE_NAME;

int cn = 1;                                 /* Active clients number */
pid_t pid, mpid;                            /* Current and main daemon PID */
int isock, ssock, csock;                    /* Sockets for in/out/clients */

/* -------------------------------------------------------------------------- */
int main(int argc, char* argv[]) { 
/*  ts-warp [-I IP] [-i port] [-l file.conf] [-v 0..4] [-d] [-c file.ini] [-h]

        -I IP           Incoming local IP address and ...
        -i port         ... port number we listen to and accept connections

        -l file.log     Log filename
        -v 0..4         Log verbosity level: 0 - off, default 2

        -d              Become a daemon

        -c file.ini     Configuration filename
        
        -h              This message */

    int flg;                                /* Command-line options flag */
    char *iaddr = LISTEN_DEFAULT;           /* Our (incomming) address and... */
    char *iport = LISTEN_PORT;              /* ...a port to accept clients */
    int d_flg = 0;                          /* Daemon mode */

    struct addrinfo ihints, *ires = NULL;   /* Our address info structures */
    ini_section *ini_root, *s_ini;          /* Pointers to the root and current
                                               sections of the INI file */
    unsigned char auth_method;              /* SOCKS5 accepted auth method */

    struct sockaddr caddr;                  /* Client address */
    socklen_t caddrlen;                     /* Client address len */
    struct sockaddr daddr;                  /* Client destination address */

    fd_set rfd;
    struct timeval tv;
    struct timespec ts;

    char buf[BUF_SIZE];                     /* Multipurpose purpose buffer */
    int ret;                                /* Various function return codes */
    int rec, snd;                           /* received/sent bytes */


    while ((flg = getopt(argc, argv, "I:i:l:v:dc:h")) != -1)
		switch(flg) {
            case 'I':                               /* Our IP/name */
                if (optarg) iaddr = optarg; break;
            case 'i':                               /* Our port */
                if (optarg) iport = optarg; break;  /* TODO: Set constraints */
            case 'l':                               /* Logfile */
                if (optarg) lfile_name = optarg; break;
            case 'v':                               /* Log verbosity */
                if (optarg) loglevel = (uint8_t)toint(optarg); break;
            case 'd':                               /* Daemon mode */
                d_flg = 1; break;
            case 'c':                               /* Configuration INI-file */
                if (optarg) ifile_name = optarg; break;
                break;
            case 'h':                               /* Help */
            default:
                (void)usage(0);
        }

    if (!iaddr[0]) iaddr = LISTEN_DEFAULT;
    if (!iport[0]) iport = LISTEN_PORT;

    /* Open log-file */
    if (!(lfile = fopen(lfile_name, "a"))) {
        printl(LOG_WARN, "Unable to open log: [%s], now trying: [%s]",
            lfile_name, LOG_FILE_NAME);
        lfile_name = LOG_FILE_NAME;
        if (!(lfile = fopen(lfile_name, "a"))) {
            printl(LOG_CRIT, "Unable to open the default log: [%s]", lfile_name);
            mexit(1, NULL);
        }
    }
    printl(LOG_VERB, "Log file: [%s], verbosity level: [%d]", lfile_name, loglevel);
    printl(LOG_INFO, "We want to listen on: [%s:%s]", iaddr, iport);

    if (d_flg) {
        /* -- Daemonizing --------------------------------------------------- */
        signal(SIGHUP, trap_signal);
        signal(SIGINT, trap_signal);
        signal(SIGQUIT, trap_signal);
        signal(SIGTERM, trap_signal);
        signal(SIGCHLD, trap_signal);

        if ((pid = fork()) == -1) {
            printl(LOG_CRIT, "Daemonizing failed. The 1-st fork() failed");
            mexit(1, NULL);
        }
        if (pid > 0) exit(0);
        if (setsid() < 0) {
            printl(LOG_CRIT, "Daemonizing failed. Fatal setsid()");
            mexit(1, NULL);
        }
        if ((pid = fork()) == -1) {
            printl(LOG_CRIT, "Daemonizing failed. The 2-nd fork() failed");
            mexit(1, NULL);
        }
        if (pid > 0) exit(0);

        printl(LOG_INFO, "Daemon started");
        mpid = mk_pidfile(pfile_name);
    }

    /* -- Try validating our address for incoming connections --------------- */
    memset(&ihints, 0, sizeof ihints);
    ihints.ai_family = PF_UNSPEC;
    ihints.ai_socktype = SOCK_STREAM;
    ihints.ai_flags = AI_PASSIVE;
    if ((ret = getaddrinfo(iaddr, iport, &ihints, &ires)) > 0) {
        printl(LOG_CRIT, "Error resolving our address [%s]: %s", 
            iaddr, gai_strerror(ret));
        mexit(1, pfile_name);
    }

    printl(LOG_INFO, "Our address [%s] succesfully resolved to [%s]",
        iaddr, inet2str(ires->ai_addr, buf));

    ini_root = read_ini(ifile_name);
    show_ini(ini_root);

    /* -- Create socket for the incoming connections ------------------------ */
    if ((isock = socket(ires->ai_family, ires->ai_socktype, 
        ires->ai_protocol)) == -1) {
            printl(LOG_CRIT,
                "Error creating a socket for incoming connections");
            mexit(1, pfile_name);
    }
    printl(LOG_INFO, "Our socket for incoming connections created");
    
    /* -- Bind incoming connections socket ---------------------------------- */
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
    printl(LOG_INFO, "The socket for incoming connections succesfully bound");

    /* -- Start listening for clients --------------------------------------- */
    if (listen(isock, SOMAXCONN) == -1) {
        printl(LOG_CRIT, "Error listening the socket for incoming connections");
        close(isock);
        mexit(1, pfile_name);
    }
    printl(LOG_INFO, "Listening for incoming connections");

    /* -- Process clients --------------------------------------------------- */
    while (1) {
        caddrlen = sizeof caddr;
        memset(&caddr, 0, caddrlen);
        if ((csock = accept(isock, &caddr, &caddrlen)) < 0) {
            printl(LOG_CRIT, "Error accepting incoming connection");
            return 1;
        }

        printl(LOG_INFO, "Serving client #: %d", cn++);

        /* TODO: rewrite as inet2str(): */
        char str_addr[255];
        inet_ntop(AF_INET, &SIN4_ADDR(caddr), str_addr, INET_ADDRSTRLEN);
        printl(LOG_VERB, "Client IP address: [%0s]", str_addr);

        printl(LOG_INFO, "Incoming connection accepted");
        
        if ((pid = fork()) == -1) {
            printl(LOG_INFO, "Failed fork() to serve a client request");
            mexit(1, pfile_name);
        }
        if (pid > 0) {                    /* Main (parent process) */
            setpgid(pid, mpid);
            close(csock);
        }

        if (pid == 0) {
            /* -- Client processing (child) --------------------------------- */
            close(isock);

            pid = getpid();
            printl(LOG_INFO, "A new client process started");

            /* Get the client original destination from NAT ----------------- */
#if defined(linux)
            /* On Linux && IPTABLES: */
            socklen_t daddrlen = sizeof daddr;  /* Client dest address len */
            memset(&daddr, 0, daddrlen); 
            daddr.sa_family = caddr.sa_family;
            if (getsockopt(csock, SOL_IP, SO_ORIGINAL_DST, &daddr, &daddrlen) != 0) {
                printl(LOG_CRIT, "Could not determine client real destination");
                exit(1);
            }
#else
            /* On *BSD with PF: */
            daddr = nat_lookup(&caddr, ires->ai_addr);
#endif
            /* Find SOCKS server to serve the daddr (dest. address) in INI file */
            if (!(s_ini = ini_look_server(ini_root, daddr))) {
                /*  Direct connection with the destination address bypassing SOCKS */
                printl(LOG_WARN, "No suitable SOCKS server was found in INI-file");
                printl(LOG_INFO, "Making direct connection with destination address");

                ssock = connect_desnation(daddr);

                printl(LOG_INFO, "Succesfully connected with desination address");
            } else {
                /* Start SOCKS proto ---------------------------------------- */

                if (s_ini->proxy_chain) {
                    /* SOCKS chain */

                    struct socks_chain *sc = s_ini->proxy_chain;

                    /* Connect the first member of the chain */
                    ssock = connect_desnation(s_ini->proxy_chain->chain_member->socks_server);
                    while (sc) {
                        switch (auth_method = socks5_hello(ssock,
                                AUTH_METHOD_NOAUTH, AUTH_METHOD_UNAME,
                                AUTH_METHOD_NOACCEPT)) {
                            case AUTH_METHOD_NOAUTH:
                                /* No authentication required */
                                break;
                            case AUTH_METHOD_UNAME:
                                /* Perform user/password auth */
                                if (socks5_auth(ssock,
                                                sc->chain_member->socks_user, 
                                                sc->chain_member->socks_password)) {
                                    printl(LOG_CRIT,
                                            "SOCKS rejected user: [%s]",
                                            sc->chain_member->socks_user);
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
                                printl(LOG_CRIT,
                                        "SOCKS server accepted unsupported auth-method: [%d]",
                                        auth_method);
                                exit(1);

                            case AUTH_METHOD_NOACCEPT:
                            default:
                                printl(LOG_CRIT,
                                        "No auth methods were accepted by SOCKS server");
                                exit(1);
                        }

                        printl(LOG_INFO, "Initiate SOCKS protocol: request");

                        if (sc->next) {
                            /* We want to connect with the next chain member */
                            if (socks5_request(ssock, SOCKS5_CMD_TCPCONNECT, 
                                sc->next->chain_member->socks_server.sa_family == AF_INET ? 
                                SOCKS5_ATYPE_IPV4 : SOCKS5_ATYPE_IPV6, 
                                &sc->next->chain_member->socks_server) > 0) {
                                    printl(LOG_CRIT, "SOCKS server returned an error");
                                    exit(1);
                            }
                        } else {
                            /* We are at the end of the chain, so connect with the section server */
                            if (socks5_request(ssock, SOCKS5_CMD_TCPCONNECT, 
                                s_ini->socks_server.sa_family == AF_INET ? SOCKS5_ATYPE_IPV4 : SOCKS5_ATYPE_IPV6, 
                                &s_ini->socks_server) > 0) {
                                    printl(LOG_CRIT, "SOCKS server returned an error");
                                    exit(1);
                            }
                        }

                        sc = sc->next;
                    }
                } else {
                    /* Only a single SOCKS server: no chain */
                    printl(LOG_INFO, "Connecting the SOCKS server");

                    ssock = connect_desnation(s_ini->socks_server);

                    printl(LOG_INFO, "Succesfully connected with SOCKS server");
                    printl(LOG_INFO, "Initiate SOCKS protocol: hello");

                    switch (auth_method = socks5_hello(ssock, AUTH_METHOD_NOAUTH, 
                    AUTH_METHOD_UNAME, AUTH_METHOD_NOACCEPT)) {
                        case AUTH_METHOD_NOAUTH:    /* No authentication required */
                            break;
                        case AUTH_METHOD_UNAME:     /* Perform user/password auth */
                            if (socks5_auth(ssock, s_ini->socks_user, s_ini->socks_password)) {
                                printl(LOG_CRIT, "SOCKS rejected user: [%s]", s_ini->socks_user);
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
                            printl(LOG_CRIT, "SOCKS server accepted unsupported auth-method: [%d]",
                                auth_method);
                            exit(1);

                        case AUTH_METHOD_NOACCEPT:
                        default:
                            printl(LOG_CRIT, "No auth methods were accepted by SOCKS server");
                            exit(1);
                    }

                    printl(LOG_INFO, "Initiate SOCKS protocol: request");

                    if (socks5_request(ssock, SOCKS5_CMD_TCPCONNECT, 
                        daddr.sa_family == AF_INET ? SOCKS5_ATYPE_IPV4 : SOCKS5_ATYPE_IPV6, 
                        &daddr) > 0) {                    
                            printl(LOG_CRIT, "SOCKS server returned an error");
                            exit(1);
                    }
                }
            }

            printl(LOG_VERB, "Start connection-forward loop");

            /* -- Forward connections --------------------------------------- */
            while (1) {
                FD_ZERO(&rfd);
                FD_SET(csock, &rfd);
                FD_SET(ssock, &rfd);

                /* TODO: Is it ppossible to use the only timeval or timespec? */
                ts.tv_sec = 0;
                ts.tv_nsec = 100000;
                tv.tv_sec = 0;
                tv.tv_usec = 100000;
                ret = select(ssock > csock ? ssock + 1: csock + 1, &rfd, 0, 0, &tv);

                if (ret < 0) break;
                if (ret == 0) continue;
                if (ret > 0) {                
                    printl(LOG_VERB, "A socket wants to send");
    
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
                            nanosleep(&ts, NULL);
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
                            nanosleep(&ts, NULL);
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
            printl(LOG_VERB, "End connection-forward loop");
            shutdown(csock, SHUT_RDWR);
            shutdown(ssock, SHUT_RDWR);
            printl(LOG_INFO, "Client finishing operations");
            close(csock);
            close(ssock);
            exit(0);
        }

    }
    freeaddrinfo(ires);
    return 0;
}

/* -------------------------------------------------------------------------- */
void trap_signal(int sig) {
    /* Signal handler */

    int	status;                                     /* Client process status */

	switch(sig) {
            case SIGHUP:                            /* TODO: Re-read INI-file */
                break;
            case SIGINT:                            /* Exit processes */
            case SIGQUIT:
            case SIGTERM:
                if (getpid() == mpid) {             /* Main daemon */
                    shutdown(isock, SHUT_RDWR);
                    close(isock);
                    mexit(0, pfile_name);
                } else {                            /* Client process */
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
