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


/* -- Main program file --------------------------------------------------------------------------------------------- */
#if defined(linux)
    #define _GNU_SOURCE
    #include <netinet/in.h>
    #include <linux/netfilter_ipv4.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include <fcntl.h>
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

#include <sys/param.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#if (WITH_LIBSSH2)
    #include <libssh2.h>
#endif

#include "network.h"
#include "utility.h"

#include "socks.h"
#include "http.h"
#include "ssh2.h"

#include "inifile.h"
#include "logfile.h"
#include "pidfile.h"
#include "pidlist.h"
#include "natlook.h"
#include "ts-warp.h"

/* ------------------------------------------------------------------------------------------------------------------ */
int loglevel = LOG_LEVEL_DEFAULT;
FILE *lfile = NULL;
char *ifile_name = INI_FILE_NAME;
char *lfile_name = LOG_FILE_NAME;
char *tfile_name = ACT_FILE_NAME;
char *pfile_name = PID_FILE_NAME;

int cn = 1;                                         /* Active clients number */
pid_t pid, mpid;                                    /* Current and main daemon PID */
struct pid_list *pids = NULL;                       /* List of active clients with PIDs and Sections */
int Tsock, Ssock, Hsock, isock, csock;              /* Sockets for Transparent/Internal-Socks&HTTP/in/clients */
chs ssock;                                          /* Structure for Out socket or SSH2 channel */
ini_section *ini_root;                              /* Root section of the INI-file */

#if !defined(linux)
    int pfd;                                        /* PF device-file on *BSD */
#endif

int msgid;                                          /* Message Queue ID */
int tfd = -1;                                       /* Traffic log file descriptor */


/* ------------------------------------------------------------------------------------------------------------------ */
int main(int argc, char* argv[]) {
/* Usage:
Usage:
  ts-warp -T IP:Port -S IP:Port -H IP:Port -c file.ini -l file.log -v 0-4 -t file.act -d -p file.pid -f -u user -h

Version:
  TS-Warp-X.Y.Z

All parameters are optional:
  -T IP:Port      Local IP address and port for incoming Transparent requests; set -T 0:0 to disables it
  -S IP:Port      Local IP address and port for internal Socks5 server; set -S 0:0 to disables it
  -H IP:Port      Local IP address and port for internal HTTP server; set -H 0:0 to disables it

  -l file.log     Main log filename
  -v 0..4         Log verbosity level: 0 - off, default: 3
  -t file.act     Active connections and traffic log

  -d              Daemon mode
  -p file.pid     PID filename
  -f              Force start

  -u user         A user to run ts-warp, default: nobody

  -h              This message */

    int flg;                                                            /* Command-line options flag */
    char *taddr = LISTEN_DEFAULT;                                       /* Our address and... */
    char *tport = LISTEN_TRANS_PORT;                                    /* ...a port to accept clients */
    char *saddr = LISTEN_DEFAULT;                                       /* Internal Socks address and... */
    char *sport = LISTEN_SOCKS_PORT;                                    /* ...a port to accept clients */
    char *haddr = LISTEN_DEFAULT;                                       /* Internal HTTP address and... */
    char *hport = LISTEN_HTTP_PORT;                                     /* ...a port to accept clients */

    int l_flg = 0;                                                      /* User didn't set the log file */
    int d_flg = 0;                                                      /* Daemon mode */
    int f_flg = 0;                                                      /* Force start */

    #if !defined(__APPLE__)
        /* This doesn't work in MacOS, it won't allow reading /dev/pf under non-root */
        char *runas_user = RUNAS_USER;                                  /* A user to run ts-warp */
    #endif

    struct addrinfo thints, *tres = NULL, *sres = NULL, *hres = NULL;   /* TS-Warp incoming addresses info structures */
    ini_section *s_ini = NULL;                                          /* Current section of the INI-file */

    struct sockaddr_storage caddr;                                      /* Client address */
    socklen_t caddrlen;                                                 /* Client address len */
    struct uvaddr daddr;                                                /* Client destination ip and/or name */
    unsigned int daddr_len;

    unsigned char auth_method;                                          /* Socks5 accepted auth method */

    fd_set sfd;                                                         /* Internal servers FDs */
    fd_set rfd;                                                         /* Connection FDs */
    struct timeval tv;

    char buf[BUF_SIZE];                                                 /* Multipurpose buffer */
    char suf[STR_SIZE];                                                 /* String buffer */
    int ret;                                                            /* Various function return codes */
    int rec = 0, snd = 0;                                               /* received/sent bytes */
    pid_t cpid;                                                         /* Child PID */

    key_t mskey;                                                        /* IPC ID */
    struct traffic_message tmessage;                                    /* IPC message to pass info about traffic */

    struct pid_list *d = NULL, *c = NULL;                               /* PID list related ... */
    struct ini_section *push_ini = NULL;                                /* variables */

    #if (WITH_LIBSSH2)
        LIBSSH2_SESSION *ssh2sess = NULL;                               /* SSH2 Session */
        LIBSSH2_CHANNEL *ssh2ch = NULL;                                 /* SSH2 channel */
        struct uvaddr p_server;
    #endif


    while ((flg = getopt(argc, argv, "T:S:H:c:l:v:t:dp:fu:h")) != -1)
        switch(flg) {
            case 'T':                                                   /* Internal Transparent server IP/name */
                taddr = strsep(&optarg, ":");                           /* IP:PORT */
                if (optarg) tport = optarg;
            break;

            case 'S':                                                   /* Internal Socks server IP/name */
                saddr = strsep(&optarg, ":");                           /* IP:PORT */
                if (optarg) sport = optarg;
            break;

            case 'H':                                                   /* Internal HTTP server IP/name */
                haddr = strsep(&optarg, ":");                           /* IP:PORT */
                if (optarg) hport = optarg;
            break;

            case 'c':                                                   /* INI-file */
                ifile_name = optarg;
            break;

            case 'l':                                                   /* Logfile */
                l_flg = 1; lfile_name = optarg;
            break;

            case 'v':                                                   /* Log verbosity */
                loglevel = toint(optarg);
                if (loglevel < LOG_NONE || loglevel > LOG_VERB) {
                    fprintf(stderr, "Wrong -v verbosity level value: [%s]\n", optarg);
                    usage(1);
                }
            break;

            case 't':                                                   /* Active connections and traffic log */
                tfile_name = optarg;
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

    if (!taddr[0]) taddr = LISTEN_DEFAULT;
    if (!tport[0]) tport = LISTEN_TRANS_PORT;
    if (!saddr[0]) haddr = LISTEN_DEFAULT;
    if (!sport[0]) hport = LISTEN_SOCKS_PORT;
    if (!haddr[0]) haddr = LISTEN_DEFAULT;
    if (!hport[0]) hport = LISTEN_HTTP_PORT;

    /* -- Open log-files -------------------------------------------------------------------------------------------- */
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

    printl(LOG_INFO, "ts-warp incoming Transparent address: [%s:%s]", taddr, tport);
    printl(LOG_INFO, "ts-warp Internal Socks address: [%s:%s]", saddr, sport);
    printl(LOG_INFO, "ts-warp Internal HTTP address: [%s:%s]", haddr, hport);

    if (mkfifo(tfile_name, S_IFIFO|S_IRWXU|S_IRGRP|S_IROTH) == -1 && errno != EEXIST)
        printl(LOG_WARN, "Unable to create active connections and traffic log pipe: [%s]", tfile_name);
    else
        if ((tfd = open(tfile_name, O_RDWR) ) == -1)
            printl(LOG_WARN, "Unable to open active connections and traffic log pipe: [%s]", tfile_name);
        else
            printl(LOG_INFO, "Active connections and traffic log pipe available: [%s]", tfile_name);

    #if !defined(linux)
        pfd = pf_open();                                                /* Open PF device-file on *BSD */
    #endif

    #if !defined(__APPLE__)
        struct passwd *pwd = getpwnam(runas_user);
    #endif

    #if (WITH_LIBSSH2)                                                   /* Init LIBSSH2 */
        if ((ret = libssh2_init(0))) {
            fprintf (stderr, "libssh2 initialization failed (%d)\n", ret);
            return 1;
        }
    #endif

    if (d_flg) {
        /* -- Daemonizing ------------------------------------------------------------------------------------------- */
        signal(SIGHUP, trap_signal);
        signal(SIGINT, trap_signal);
        signal(SIGQUIT, trap_signal);
        signal(SIGTERM, trap_signal);
        signal(SIGCHLD, trap_signal);
        signal(SIGUSR1, trap_signal);
        signal(SIGUSR2, trap_signal);

        signal(SIGPIPE, SIG_IGN);                       /* Ignore the signal if nobody reads the traffig log pipe! */


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
            pid = mk_pidfile(pfile_name, f_flg, 0, 0);
        #else
            pid = mk_pidfile(pfile_name, f_flg, pwd ? pwd->pw_uid : 0, pwd ? pwd->pw_gid : 0);
        #endif
    }
    mpid = pid;

    #if !defined(__APPLE__)
        if (setuid(pwd->pw_uid) && setgid(pwd->pw_gid)) {
            printl(LOG_CRIT, "Failed to set privilege level to UID:GID [%d:%d]", pwd->pw_uid, pwd->pw_gid);
            exit(1);
        }
    #endif

    /* -- Try validating our address for incoming connections ------------------------------------------------------- */
    memset(&thints, 0, sizeof(thints));
    thints.ai_family = PF_UNSPEC;
    thints.ai_socktype = SOCK_STREAM;
    thints.ai_flags = AI_PASSIVE;

    if ((ret = getaddrinfo(taddr, tport, &thints, &tres)) > 0) {
        printl(LOG_CRIT, "Error resolving ts-warp Transparent address [%s]: %s", taddr, gai_strerror(ret));
        mexit(1, pfile_name, tfile_name);
    }
    printl(LOG_INFO, "ts-warp Transparent address [%s] succesfully resolved to [%s]",
        taddr, inet2str((struct sockaddr_storage *)(tres->ai_addr), buf));

    if ((ret = getaddrinfo(saddr, sport, &thints, &sres)) > 0) {
        printl(LOG_CRIT, "Error resolving ts-warp Socks address [%s]: %s", saddr, gai_strerror(ret));
        mexit(1, pfile_name, tfile_name);
    }
    printl(LOG_INFO, "ts-warp Socks address [%s] succesfully resolved to [%s]",
        saddr, inet2str((struct sockaddr_storage *)(sres->ai_addr), buf));

    if ((ret = getaddrinfo(haddr, hport, &thints, &hres)) > 0) {
        printl(LOG_CRIT, "Error resolving ts-warp HTTP address [%s]: %s", haddr, gai_strerror(ret));
        mexit(1, pfile_name, tfile_name);
    }
    printl(LOG_INFO, "ts-warp HTTP address [%s] succesfully resolved to [%s]",
        haddr, inet2str((struct sockaddr_storage *)(hres->ai_addr), buf));

    ini_root = read_ini(ifile_name);
    show_ini(ini_root, LOG_VERB);

    /* -- Create sockets for incoming connections ------------------------------------------------------------------- */
    if (ntohs(SIN_PORT(*(tres->ai_addr)))) {
        if ((Tsock = socket(tres->ai_family, tres->ai_socktype, tres->ai_protocol)) == -1) {
            printl(LOG_CRIT, "Error creating a socket for Transparent incoming connections");
            mexit(1, pfile_name, tfile_name);
        }
    } else {
        Tsock = -1;
        printl(LOG_INFO, "Transparent connections is disabled!");
    }

    if (ntohs(SIN_PORT(*(sres->ai_addr)))) {
        if ((Ssock = socket(sres->ai_family, sres->ai_socktype, sres->ai_protocol)) == -1) {
            printl(LOG_CRIT, "Error creating a socket for Socks5 incoming connections");
            mexit(1, pfile_name, tfile_name);
        }
    } else {
        Ssock = -1;
        printl(LOG_INFO, "Socks5 incoming connections is disabled!");
    }

    if (ntohs(SIN_PORT(*(hres->ai_addr)))) {
        if ((Hsock = socket(hres->ai_family, hres->ai_socktype, hres->ai_protocol)) == -1) {
            printl(LOG_CRIT, "Error creating a socket for HTTP incoming connections");
            mexit(1, pfile_name, tfile_name);
        }
    } else {
        Hsock = -1;
        printl(LOG_INFO, "HTTP incoming connections is disabled!");
    }

    if (Tsock == -1 && Ssock == -1 && Hsock == -1) {
        printl(LOG_CRIT, "All incoming connections are disabled! Nothing to do, exitting.");
        mexit(0, pfile_name, tfile_name);
    }

    /* Internal servers to be non-blocking, so we can check which one acceps connection */
    if (Tsock != -1) fcntl(Tsock, F_SETFL, O_NONBLOCK);
    if (Ssock != -1) fcntl(Ssock, F_SETFL, O_NONBLOCK);
    if (Hsock != -1) fcntl(Hsock, F_SETFL, O_NONBLOCK);
    printl(LOG_VERB, "Socket(s) for incoming connections created");

    /* -- Apply socket options -------------------------------------------------------------------------------------- */
    #if (WITH_TCP_NODELAY)
        int tpc_ndelay = 1;
        int keepalive_opt = 1;

        if (Tsock != -1) {
            if (setsockopt(Tsock, IPPROTO_TCP, TCP_NODELAY, &tpc_ndelay, sizeof(int)) == -1)
                printl(LOG_WARN, "Error setting TCP_NODELAY socket option for Transparent connections");
            else printl(LOG_INFO, "TCP_NODELAY option for Transparent socket enabled");

            if (setsockopt(Tsock, SOL_SOCKET, SO_KEEPALIVE, &keepalive_opt, sizeof(int)) == -1)
                printl(LOG_WARN, "Error setting SO_KEEPALIVE socket option for Transparent connections");
        }

        if (Ssock != -1) {
            if (setsockopt(Ssock, IPPROTO_TCP, TCP_NODELAY, &tpc_ndelay, sizeof(int)) == -1)
                printl(LOG_WARN, "Error setting TCP_NODELAY socket option for Socks5 incoming connections");
            else printl(LOG_INFO, "TCP_NODELAY option for Socks socket enabled");

            if (setsockopt(Ssock, SOL_SOCKET, SO_KEEPALIVE, &keepalive_opt, sizeof(int)) == -1)
                printl(LOG_WARN, "Error setting SO_KEEPALIVE socket option for Socks5 incoming connections");
        }

        if (Hsock != -1) {
            if (setsockopt(Hsock, IPPROTO_TCP, TCP_NODELAY, &tpc_ndelay, sizeof(int)) == -1)
                printl(LOG_WARN, "Error setting TCP_NODELAY socket option for HTTP incoming connections");
            else printl(LOG_INFO, "TCP_NODELAY option for HTTP socket enabled");

            if (setsockopt(Hsock, SOL_SOCKET, SO_KEEPALIVE, &keepalive_opt, sizeof(int)) == -1)
                printl(LOG_WARN, "Error setting SO_KEEPALIVE socket option for HTTP incoming connections");
        }
    #endif

    #if !defined(__OpenBSD__)
        #if !defined(__APPLE__)
            keepalive_opt = TCP_KEEPIDLE_S;
            if (Tsock != -1)
                if (setsockopt(Tsock, IPPROTO_TCP, TCP_KEEPIDLE, &keepalive_opt, sizeof(int)) == -1)
                    printl(LOG_WARN, "Error setting TCP_KEEPIDLE socket option for Transparent connections");
            if (Ssock != -1)
                if (setsockopt(Ssock, IPPROTO_TCP, TCP_KEEPIDLE, &keepalive_opt, sizeof(int)) == -1)
                    printl(LOG_WARN, "Error setting TCP_KEEPIDLE socket option for Socks5 incoming connections");
            if (Hsock != -1)
                if (setsockopt(Hsock, IPPROTO_TCP, TCP_KEEPIDLE, &keepalive_opt, sizeof(int)) == -1)
                    printl(LOG_WARN, "Error setting TCP_KEEPIDLE socket option for HTTP incoming connections");
        #endif

        keepalive_opt = TCP_KEEPCNT_N;
        if (Tsock != -1)
            if (setsockopt(Tsock, IPPROTO_TCP, TCP_KEEPCNT, &keepalive_opt, sizeof(int)) == -1)
                printl(LOG_WARN, "Error setting TCP_KEEPCNT socket option for Transparent connections");
        if (Ssock != -1)
            if (setsockopt(Ssock, IPPROTO_TCP, TCP_KEEPCNT, &keepalive_opt, sizeof(int)) == -1)
                printl(LOG_WARN, "Error setting TCP_KEEPCNT socket option for Socks5 incoming connections");
        if (Hsock != -1)
            if (setsockopt(Hsock, IPPROTO_TCP, TCP_KEEPCNT, &keepalive_opt, sizeof(int)) == -1)
                printl(LOG_WARN, "Error setting TCP_KEEPCNT socket option for HTTP incoming connections");

        keepalive_opt = TCP_KEEPINTVL_S;
        if (Tsock != -1)
            if (setsockopt(Tsock, IPPROTO_TCP, TCP_KEEPINTVL, &keepalive_opt, sizeof(int)) == -1)
                printl(LOG_WARN, "Error setting TCP_KEEPINTVL socket option for Transparent connections");
        if (Ssock != -1)
            if (setsockopt(Ssock, IPPROTO_TCP, TCP_KEEPINTVL, &keepalive_opt, sizeof(int)) == -1)
                printl(LOG_WARN, "Error setting TCP_KEEPINTVL socket option for Socks5 incoming connections");
        if (Hsock != -1)
            if (setsockopt(Hsock, IPPROTO_TCP, TCP_KEEPINTVL, &keepalive_opt, sizeof(int)) == -1)
                printl(LOG_WARN, "Error setting TCP_KEEPINTVL socket option for HTTP incoming connections");
    #endif          /* __OpenBSD__ */

    int raddr = 1;
    if (Tsock != -1)
        if (setsockopt(Tsock, SOL_SOCKET, SO_REUSEADDR, &raddr, sizeof(int)) == -1)
            printl(LOG_WARN, "Error setting Transparent connections socket to be reusable");
    if (Ssock != -1)
        if (setsockopt(Ssock, SOL_SOCKET, SO_REUSEADDR, &raddr, sizeof(int)) == -1)
            printl(LOG_WARN, "Error setting Socks incomming socket to be reusable");
    if (Hsock != -1)
        if (setsockopt(Hsock, SOL_SOCKET, SO_REUSEADDR, &raddr, sizeof(int)) == -1)
            printl(LOG_WARN, "Error setting HTTP incomming socket to be reusable");

    /* -- Bind incoming connection sockets & start listening for clients -------------------------------------------- */
    if (Tsock != -1) {
        if (bind(Tsock, tres->ai_addr, tres->ai_addrlen) < 0) {
            printl(LOG_CRIT, "Error binding socket for Transparent incoming connections");
            close(Tsock);
            mexit(1, pfile_name, tfile_name);
        }
        printl(LOG_VERB, "The socket for Transparent connections succesfully bound");

        if (listen(Tsock, SOMAXCONN) == -1) {
            printl(LOG_CRIT, "Error listening the socket for Transparent connections");
            close(Tsock);
            mexit(1, pfile_name, tfile_name);
        }
        printl(LOG_INFO, "Listening for Transparent connections");
    }

    if (Ssock != -1) {
        if (bind(Ssock, sres->ai_addr, sres->ai_addrlen) < 0) {
            printl(LOG_CRIT, "Error binding socket for Socks incoming connections connections");
            close(Ssock);
            mexit(1, pfile_name, tfile_name);
        }
        printl(LOG_VERB, "The socket for Socks incoming connections succesfully bound");

        if (listen(Ssock, SOMAXCONN) == -1) {
            printl(LOG_CRIT, "Error listening the socket for Socks incoming connections");
            close(Ssock);
            mexit(1, pfile_name, tfile_name);
        }
        printl(LOG_INFO, "Listening for Socks incoming connections");
    }

    if (Hsock != -1) {
        if (bind(Hsock, hres->ai_addr, hres->ai_addrlen) < 0) {
            printl(LOG_CRIT, "Error binding socket for HTTP incoming connections");
            close(Hsock);
            mexit(1, pfile_name, tfile_name);
        }
        printl(LOG_VERB, "The socket for HTTP incoming connections succesfully bound");

        if (listen(Hsock, SOMAXCONN) == -1) {
            printl(LOG_CRIT, "Error listening the socket for HTTP incoming connections");
            close(Hsock);
            mexit(1, pfile_name, tfile_name);
        }
        printl(LOG_INFO, "Listening for HTTP incoming connections");
    }

    /* -- Set message queue ----------------------------------------------------------------------------------------- */
    mskey = ftok("TS-WARP", 10800);
    if ((msgid = msgget(mskey, 0600 | IPC_CREAT)) == -1)
        printl(LOG_WARN, "Unable to acquire IPC mesage queue ID. No traffic stats will be collected");

    /* -- Process clients ------------------------------------------------------------------------------------------- */
    while (1) {
        FD_ZERO(&sfd);
        if (Tsock != -1) FD_SET(Tsock, &sfd);
        if (Ssock != -1) FD_SET(Ssock, &sfd);
        if (Hsock != -1) FD_SET(Hsock, &sfd);

        tv.tv_sec = 0;
        tv.tv_usec = 10000;
/*        ret = select(Hsock + 1, &sfd, NULL, NULL, &tv); */
        ret = select(MAX(MAX(Hsock, Ssock), Tsock) + 1, &sfd, NULL, NULL, &tv);

        if (ret < 0) continue;                                          /* On an error skip to the next iteration */
        if (ret == 0) {                                                 /* Timeout - no new connections */
            if (msgid != -1 && msgrcv(msgid, &tmessage, sizeof(tmessage), 1, IPC_NOWAIT) != -1)
                pidlist_update_traffic(pids, tmessage.mtext);
            continue;
        }

        if (msgid != -1 && msgrcv(msgid, &tmessage, sizeof(tmessage), 1, IPC_NOWAIT) != -1)
            pidlist_update_traffic(pids, tmessage.mtext);

        /* Check which of the internal servers has a pending connection */
        if (Tsock != -1 && FD_ISSET(Tsock, &sfd)) isock = Tsock; else
            if (Tsock != -1 && FD_ISSET(Ssock, &sfd)) isock = Ssock; else
                isock = Hsock;

        caddrlen = sizeof caddr;
        memset(&caddr, 0, caddrlen);
        if ((csock = accept(isock, (struct sockaddr *)&caddr, &caddrlen)) < 0) {
            printl(LOG_CRIT, "Error accepting incoming connection");
            return 1;
        }
        fcntl(csock, F_SETFL, ~O_NONBLOCK);                         /* Don't block client connections */
        printl(LOG_INFO, "Client: [%d], IP: [%s] accepted", cn++, inet2str(&caddr, buf));

        /* -- Get the client original destination from NAT ---------------------------------------------------------- */

        /* Initialize  daddr */
        daddr_len = sizeof(daddr.ip_addr);
        memset(&daddr.ip_addr, 0, daddr_len);
        memset(&daddr.name, 0, sizeof(daddr.name) - 1);

        #if defined(linux)
            /* On Linux && nftabeles/iptables */
            memset(&daddr.ip_addr, 0, daddr_len);
            daddr.ip_addr.ss_family = caddr.ss_family;
            ret = getsockopt(csock, SOL_IP, SO_ORIGINAL_DST, &daddr.ip_addr, &daddr_len);
        #else
            /* On *BSD with PF */
            ret = nat_lookup(pfd, &caddr, (struct sockaddr_storage *)tres->ai_addr, &daddr.ip_addr);
        #endif
        if (ret) {
            printl(LOG_WARN, "Failed to find the real destination IP, trying to get it from the socket");
            getpeername(csock, (struct sockaddr *)&daddr.ip_addr, &daddr_len);
        }

        printl(LOG_INFO, "The client destination address is: [%s]", inet2str(&daddr.ip_addr, buf));

        /* -- Process the PIDs list: remove exitted clients and execute workload balance functions ------------------ */
        c = pids;
        while (c) {
            if (c == pids && c->status >= 0) {                      /* Remove pidlist root entry */
                pids = c->next;
                if (c->status && (push_ini = getsection(ini_root, c->section_name)))
                    if (push_ini->section_balance == SECTION_BALANCE_FAILOVER)
                        pushback_ini(&ini_root, push_ini);
                free(c->section_name);
                free(c);
                c = pids;
            }

            if (c && c->next && c->next->status >= 0) {             /* Remove a pidlist entry */
                d = c->next;
                c->next = d->next;
                if (d->status && (push_ini = getsection(ini_root, d->section_name)))
                    if (push_ini->section_balance == SECTION_BALANCE_FAILOVER)
                        pushback_ini(&ini_root, push_ini);
                free(d->section_name);
                free(d);
            }

            if (c) c = c->next;
        }

        /* Find Socks server to serve the destination address in INI file */
        s_ini = ini_look_server(ini_root, daddr);
        if (s_ini && s_ini->section_balance == SECTION_BALANCE_ROUNDROBIN) pushback_ini(&ini_root, s_ini);

        if ((cpid = fork()) == -1) {
            printl(LOG_CRIT, "Failed fork() to serve a client request");
            mexit(1, pfile_name, tfile_name);
        }

        if (cpid > 0) {
            /* -- Main (parent process) ----------------------------------------------------------------------------- */
            setpgid(cpid, pid);
            close(csock);

            /* Save the client into the list */
            pids = pidlist_add(pids, s_ini ? s_ini->section_name : "", cpid, caddr, daddr.ip_addr);
        }

        if (cpid == 0) {
            /* -- Client processing (child) ------------------------------------------------------------------------- */

            ssock.t = CHS_SOCKET;                                       /* Type socket */
            #if (WITH_LIBSSH2)
                ssock.c = NULL;
            #endif

            pid = getpid();
            printl(LOG_VERB, "A new client process started");

            if (!s_ini) {
                /* -- No proxy server found for the destination IP -------------------------------------------------- */
                printl(LOG_INFO, "No proxy server is defined for the destination: [%s]", inet2str(&daddr.ip_addr, buf));

                if ((daddr.ip_addr.ss_family == AF_INET && S4_ADDR(daddr.ip_addr) == S4_ADDR(*tres->ai_addr)) ||
                    (daddr.ip_addr.ss_family == AF_INET6 &&
                        !memcmp(S6_ADDR(daddr.ip_addr), S6_ADDR(*tres->ai_addr), sizeof(S6_ADDR(daddr.ip_addr)))) ||
                    (daddr.ip_addr.ss_family == AF_INET && S4_ADDR(daddr.ip_addr) == S4_ADDR(*sres->ai_addr)) ||
                    (daddr.ip_addr.ss_family == AF_INET6 &&
                        !memcmp(S6_ADDR(daddr.ip_addr), S6_ADDR(*sres->ai_addr), sizeof(S6_ADDR(daddr.ip_addr)))) ||
                    (daddr.ip_addr.ss_family == AF_INET && S4_ADDR(daddr.ip_addr) == S4_ADDR(*hres->ai_addr)) ||
                    (daddr.ip_addr.ss_family == AF_INET6 &&
                        !memcmp(S6_ADDR(daddr.ip_addr), S6_ADDR(*hres->ai_addr), sizeof(S6_ADDR(daddr.ip_addr))))) {
                    /* Desination address:port is the same as ts-warp incominig (Taransparent, Socks or HTTP) ip:port,
                    i.e., a client contacted ts-warp dirctly: no NAT/redirection and TS-Warp is not defined as
                    proxy server */
                    printl(LOG_WARN, "Dropping loop connection with ts-warp");
                    close(csock);
                    exit(1);
                }

                /*  -- Direct connection with the destination address bypassing proxy ------------------------------- */
                printl(LOG_INFO, "Making direct connection with the destination: [%s]", inet2str(&daddr.ip_addr, buf));

                if ((ssock.s = connect_desnation(*(struct sockaddr *)&daddr.ip_addr)) == -1) {
                    printl(LOG_WARN, "Unable to connect with destination: [%s]", inet2str(&daddr.ip_addr, buf));
                    close(csock);
                    exit(1);
                }

                printl(LOG_INFO, "Succesfully connected with desination address: [%s]", inet2str(&daddr.ip_addr, buf));

                goto cfloop;

            } else
                /* -- Internal TS-Warp proxy servers ---------------------------------------------------------------- */
                if (isock == Ssock) {
                    /* -- Internal Socks5 server with AUTH_METHOD_NOAUTH support only ------------------------------- */
                    if (Tsock != -1) close(Tsock);
                    close(Ssock);
                    if (Hsock != -1) close(Hsock);
                    if ((daddr.ip_addr.ss_family == AF_INET && S4_ADDR(daddr.ip_addr) != S4_ADDR(*sres->ai_addr)) ||
                        (daddr.ip_addr.ss_family == AF_INET6 &&
                            memcmp(S6_ADDR(daddr.ip_addr), S6_ADDR(*sres->ai_addr), sizeof(S6_ADDR(daddr.ip_addr)))))
                                goto proxyforward;

                    /* Desination address:port is the same as ts-warp income ip:port, i.e., a client contacted
                    ts-warp directly: no NAT/redirection, but TS-Warp is indicated as Socks-server */
                    printl(LOG_INFO, "Serving the client with embedded TS-Warp Socks-server");

                    if (socks5_server_hello(csock) == AUTH_METHOD_NOACCEPT) {
                        printl(LOG_WARN, "Embedded TS-Warp Socks server does not accept connections");
                        close(csock);
                        exit(1);
                    }

                    if (!socks5_server_request(csock, &daddr)) {
                        printl(LOG_WARN, "Embedded TS-Warp Socks server lost connection with the client");
                        close(csock);
                        exit(1);
                    }

                    s_ini = ini_look_server(ini_root, daddr);
                    if (!s_ini || (s_ini && SA_FAMILY(s_ini->proxy_server) == AF_INET ? \
                        S4_ADDR(s_ini->proxy_server) == S4_ADDR(*sres->ai_addr) : \
                        S6_ADDR(s_ini->proxy_server) == S6_ADDR(*sres->ai_addr))) {

                        printl(LOG_INFO, "Serving request to: [%s] with Internal TS-Warp SOCKS server",
                            daddr.name[0] ? daddr.name : inet2str(&daddr.ip_addr, buf));

                        if ((ssock.s = connect_desnation(*(struct sockaddr *)&daddr.ip_addr)) == -1) {
                            printl(LOG_WARN, "Unable to connect with destination: [%s]",
                                daddr.name[0] ? daddr.name : inet2str(&daddr.ip_addr, buf));

                            /* Replying Error to Socks5 client */
                            printl(LOG_VERB, "Replying the client, Internal Socks5 can't reach desination: [%s]",
                                daddr.name[0] ? daddr.name : inet2str(&daddr.ip_addr, buf));
                            socks5_server_reply(csock, (struct sockaddr_storage *)(sres->ai_addr), SOCKS5_REPLY_KO);
                            close(csock);
                            exit(1);
                        } else {
                            /* Replying OK to Socks5 client */
                            printl(LOG_VERB, "Replying the client, Internal Socks5 can reach desination: [%s]",
                                daddr.name[0] ? daddr.name : inet2str(&daddr.ip_addr, buf));
                            socks5_server_reply(csock, (struct sockaddr_storage *)(sres->ai_addr), SOCKS5_REPLY_OK);
                        }

                        goto cfloop;

                    } else {
                        printl(LOG_INFO, "Serving request to [%s : %s] with external proxy server, section: [%s]",
                            daddr.name, inet2str(&daddr.ip_addr, buf), s_ini->section_name);

                        /* Replying OK to Socks5 client */
                            printl(LOG_VERB,
                                "Replying Socks5 client [OK], the desination: [%s] is managed by external proxy",
                                daddr.name[0] ? daddr.name : inet2str(&daddr.ip_addr, buf));
                        socks5_server_reply(csock, (struct sockaddr_storage *)(tres->ai_addr), SOCKS5_REPLY_OK);
                    }
                    /* Pass the client to external Socks-servers - proxy forwarding */
                } else
                    if (isock == Hsock) {
                        /* -- Internal HTTP server  ----------------------------------------------------------------- */
                        if (Tsock != -1) close(Tsock);
                        if (Ssock != -1) close(Ssock);
                        close(Hsock);
                        if ((daddr.ip_addr.ss_family == AF_INET && S4_ADDR(daddr.ip_addr) != S4_ADDR(*hres->ai_addr)) ||
                            (daddr.ip_addr.ss_family == AF_INET6 && memcmp(S6_ADDR(daddr.ip_addr),
                                S6_ADDR(*hres->ai_addr), sizeof(S6_ADDR(daddr.ip_addr)))))
                                    goto proxyforward;

                        /* Desination address:port is the same as ts-warp income ip:port, i.e., a client contacted
                        ts-warp directly: no NAT/redirection, but TS-Warp is indicated as HTTP-server */
                        printl(LOG_INFO, "Serving the client with embedded TS-Warp HTTP-server");

                        if (http_server_request(csock, &daddr)) {
                            printl(LOG_WARN, "Embedded TS-Warp HTTP server lost connection with the client");
                            close(csock);
                            exit(1);
                        }

                        s_ini = ini_look_server(ini_root, daddr);
                        if (!s_ini || (s_ini && SA_FAMILY(s_ini->proxy_server) == AF_INET ? \
                            S4_ADDR(s_ini->proxy_server) == S4_ADDR(*hres->ai_addr) : \
                            S6_ADDR(s_ini->proxy_server) == S6_ADDR(*hres->ai_addr))) {

                            printl(LOG_INFO, "Serving request to: [%s] with Internal TS-Warp HTTP server",
                                daddr.name[0] ? daddr.name : inet2str(&daddr.ip_addr, buf));

                            if ((ssock.s = connect_desnation(*(struct sockaddr *)&daddr.ip_addr)) == -1) {
                                printl(LOG_WARN, "Unable to connect with destination: [%s]",
                                    daddr.name[0] ? daddr.name : inet2str(&daddr.ip_addr, buf));
                                close(csock);
                                exit(1);
                            }

                            goto cfloop;

                        } else
                            printl(LOG_INFO,
                                "Serving request to [%s : %s] with external proxy server, section: [%s]",
                                daddr.name, inet2str(&daddr.ip_addr, buf), s_ini->section_name);
                    } else
                        if (isock == Tsock) {
                            close(Tsock);
                            if (Ssock != -1) close(Ssock);
                            if (Hsock != -1) close(Hsock);
                            if ((daddr.ip_addr.ss_family == AF_INET &&
                                    S4_ADDR(daddr.ip_addr) == S4_ADDR(*tres->ai_addr)) ||
                                (daddr.ip_addr.ss_family == AF_INET6 &&
                                    !memcmp(S6_ADDR(daddr.ip_addr), S6_ADDR(*tres->ai_addr),
                                        sizeof(S6_ADDR(daddr.ip_addr))))) {

                                /* Desination address:port is the same as ts-warp transparent income ip:port, i.e.,
                                a client contacted ts-warp directly: no NAT/redirection, no Proxy */

                                printl(LOG_WARN, "Prevent loop in request [%s : %s] as Transparent, section: [%s]",
                                    daddr.name, inet2str(&daddr.ip_addr, buf), s_ini->section_name);
                                exit(1);
                            }

                            printl(LOG_INFO, "Serving request to [%s : %s] as Transparent, section: [%s]",
                                daddr.name, inet2str(&daddr.ip_addr, buf), s_ini->section_name);
                        }
                        else
                            break;

            /* -- Start proxy forwarding ---------------------------------------------------------------------------- */
            proxyforward:
            if (s_ini->p_chain) {

                /* -- Proxy chains ---------------------------------------------------------------------------------- */
                struct proxy_chain *sc = s_ini->p_chain;

                printl(LOG_INFO, "Connecting a CHAIN: [%s] proxy server: [%s] type [%c]", sc->chain_member->section_name,
                    inet2str(&sc->chain_member->proxy_server, buf), sc->chain_member->proxy_type);

                /* Connect the first member of the chain */
                if ((ssock.s = connect_desnation(*(struct sockaddr *)&sc->chain_member->proxy_server)) == -1) {
                    printl(LOG_WARN, "Unable to connect with CHAIN proxy server: [%s] type [%c]",
                        inet2str(&sc->chain_member->proxy_server, buf), sc->chain_member->proxy_type);
                    close(csock);
                    exit(2);
                }

                while (sc) {
                    switch (sc->chain_member->proxy_type) {
                        case PROXY_PROTO_SOCKS_V5:
                            if (sc->chain_member->proxy_user)
                                auth_method = socks5_client_hello(ssock, AUTH_METHOD_NOAUTH, AUTH_METHOD_UNAME,
                                    AUTH_METHOD_NOACCEPT);
                            else
                                auth_method = socks5_client_hello(ssock, AUTH_METHOD_NOAUTH, AUTH_METHOD_NOACCEPT);

                            switch (auth_method) {
                                case AUTH_METHOD_NOAUTH:
                                    /* No authentication required */
                                break;

                                case AUTH_METHOD_UNAME:
                                    /* Perform user/password auth */
                                    if (socks5_client_auth(ssock, sc->chain_member->proxy_user,
                                        sc->chain_member->proxy_password)) {

                                            printl(LOG_WARN, "CHAIN Socks5 server rejected user: [%s]",
                                                sc->chain_member->proxy_user);
                                            close(csock);
                                            exit(2);
                                    }
                                break;

                                case AUTH_METHOD_NOACCEPT:
                                default:
                                    printl(LOG_WARN, "No (supported) auth methods were accepted by CHAIN Socks5 server");
                                    close(csock);
                                    exit(2);
                            }

                            if (sc->next) {
                                /* We want to connect with the next chain member */
                                printl(LOG_VERB, "Initiate CHAIN Socks5 protocol: request [%s] -> [%s]",
                                    inet2str(&sc->chain_member->proxy_server, suf),
                                    inet2str(&sc->next->chain_member->proxy_server, buf));

                                if (socks5_client_request(ssock, SOCKS5_CMD_TCPCONNECT,
                                    &sc->next->chain_member->proxy_server, NULL)) {
                                        printl(LOG_WARN, "CHAIN Socks5 server returned an error");
                                        close(csock);
                                        exit(2);
                                }
                            } else {
                                /* We are at the end of the chain, so connect with the section server */
                                printl(LOG_VERB, "Initiate CHAIN Socks5 protocol: request [%s] -> [%s]",
                                    inet2str(&sc->chain_member->proxy_server, suf),
                                    inet2str(&s_ini->proxy_server, buf));

                                if (socks5_client_request(ssock, SOCKS5_CMD_TCPCONNECT, &s_ini->proxy_server, NULL)) {
                                    printl(LOG_WARN, "CHAIN Socks5 server returned an error");
                                    close(csock);
                                    exit(2);
                                }

                                goto single_server;
                            }
                        break;

                        case PROXY_PROTO_SOCKS_V4:
                            if (sc->next) {
                                /* We want to connect with the next chain member */
                                printl(LOG_VERB, "Initiate CHAIN Socks4 protocol: request [%s] -> [%s]",
                                    inet2str(&sc->chain_member->proxy_server, suf),
                                    inet2str(&sc->next->chain_member->proxy_server, buf));

                                if (socks4_client_request(ssock, SOCKS4_CMD_TCPCONNECT,
                                        (struct sockaddr_in *)&sc->next->chain_member->proxy_server,
                                        sc->next->chain_member->proxy_user)) {

                                            printl(LOG_WARN, "CHAIN Socks4 server returned an error");
                                            close(csock);
                                            exit(2);
                                }
                            } else {
                                /* We are at the end of the chain, so connect with the section server */
                                printl(LOG_VERB, "Initiate CHAIN Socks4 protocol: request [%s] -> [%s]",
                                    inet2str(&sc->chain_member->proxy_server, suf),
                                    inet2str(&s_ini->proxy_server, buf));

                                if (socks4_client_request(ssock, SOCKS4_CMD_TCPCONNECT,
                                        (struct sockaddr_in *)&s_ini->proxy_server, s_ini->proxy_user)) {

                                            printl(LOG_WARN, "CHAIN Socks4 server returned an error");
                                            close(csock);
                                            exit(2);
                                }

                                goto single_server;
                            }
                        break;

                        case PROXY_PROTO_HTTP:
                            if (sc->next) {
                                /* We want to connect with the next chain member */
                                printl(LOG_VERB, "Initiate CHAIN HTTP protocol: request [%s] -> [%s]",
                                    inet2str(&sc->chain_member->proxy_server, suf),
                                    inet2str(&sc->next->chain_member->proxy_server, buf));

                                if (http_client_request(ssock, &sc->next->chain_member->proxy_server,
                                        sc->next->chain_member->proxy_user, sc->next->chain_member->proxy_password)) {

                                    printl(LOG_WARN, "CHAIN HTTP server returned an error");
                                    close(csock);
                                    exit(2);
                                }
                            } else {
                                /* We are at the end of the chain, so connect with the section server */
                                printl(LOG_VERB, "Initiate CHAIN HTTP protocol: request [%s] -> [%s]",
                                    inet2str(&sc->chain_member->proxy_server, suf),
                                    inet2str(&s_ini->proxy_server, buf));

                                if (http_client_request(ssock,
                                        &s_ini->proxy_server, s_ini->proxy_user, s_ini->proxy_password)) {

                                    printl(LOG_WARN, "CHAIN HTTP server returned an error");
                                    close(csock);
                                    exit(2);
                                }

                                goto single_server;
                            }
                        break;

                        #if (WITH_LIBSSH2)
                            case PROXY_PROTO_SSH2:
                                if (ssh2sess || ssh2ch) {
                                    printl(LOG_WARN, "Only ONE SSH2 proxy could be used per CHAIN");
                                    close(csock);
                                    exit(2);
                                }

                                if (!(ssh2sess = libssh2_session_init())) {
                                    printl(LOG_WARN, "Unable to initialize SSH2 session");
                                    close(csock);
                                    exit(2);
                                }

                                if (sc->next) {
                                    /* We want to connect with the next chain member */
                                    printl(LOG_VERB, "Initiate CHAIN SSH2 protocol: request: [%s] -> [%s] mid chain cell",
                                        inet2str(&sc->chain_member->proxy_server, suf),
                                        inet2str(&sc->next->chain_member->proxy_server, buf));

                                    p_server.ip_addr = sc->next->chain_member->proxy_server;
                                    memset(p_server.name, 0, sizeof(p_server.name));
                                    if (!(ssh2ch = ssh2_client_request(ssock.s, ssh2sess, &p_server,
                                        sc->chain_member->proxy_user, sc->chain_member->proxy_password,
                                        sc->chain_member->proxy_key, sc->chain_member->proxy_key_passphrase,
                                        sc->chain_member->proxy_ssh_force_auth))) {

                                        printl(LOG_WARN, "CHAIN SSH2 proxy server returned an error");
                                        close(csock);
                                        exit(2);
                                    }
                                } else {
                                    /* As the last link in the chain and we want to connect the section server */
                                    printl(LOG_VERB, "Initiate CHAIN SSH2 protocol: request [%s] -> [%s] last chain cell",
                                        inet2str(&sc->chain_member->proxy_server, suf),
                                        inet2str(&s_ini->proxy_server, buf));

                                    p_server.ip_addr = s_ini->proxy_server;
                                    memset(p_server.name, 0, sizeof(p_server.name));
                                    if (!(ssh2ch = ssh2_client_request(ssock.s, ssh2sess, &p_server,
                                        sc->chain_member->proxy_user, sc->chain_member->proxy_password,
                                        sc->chain_member->proxy_key, sc->chain_member->proxy_key_passphrase,
                                        sc->chain_member->proxy_ssh_force_auth))) {

                                        printl(LOG_WARN, "CHAIN SSH2 proxy server returned an error");
                                        close(csock);
                                        exit(2);
                                    }

                                    ssock.t = CHS_CHANNEL;
                                    ssock.c = ssh2ch;
                                    goto single_server;
                                }
                            break;
                        #endif

                        default:
                            /* Unreachable. Must be cleared already by read_ini() */
                            printl(LOG_WARN, "Detected unsupported CHAIN proxy type: [%c]",
                                s_ini->p_chain->chain_member->proxy_type);
                            close(csock);
                            exit(2);
                    }

                    sc = sc->next;
                }
            } else {
                /* -- Single Proxy-server connection (no chains) ---------------------------------------------------- */
                printl(LOG_INFO, "Connecting the proxy server: [%s] type [%c]",
                    inet2str(&s_ini->proxy_server, buf), s_ini->proxy_type);

                if ((ssock.s = connect_desnation(*(struct sockaddr *)&s_ini->proxy_server)) == -1) {
                    printl(LOG_WARN, "Unable to connect with the proxy server: [%s] type [%c]",
                        inet2str(&s_ini->proxy_server, buf), s_ini->proxy_type);
                    close(csock);
                    exit(2);
                }

                printl(LOG_INFO, "Succesfully connected with the proxy server: [%s] type [%c]",
                    inet2str(&s_ini->proxy_server, buf), s_ini->proxy_type);

                single_server:

                /* -- Perform NIT (IPv4 only!) Lookup --------------------------------------------------------------- */
                /* Should we do this for chains as well? */
                if (s_ini->nit_domain && daddr.ip_addr.ss_family == AF_INET &&
                    S4_ADDR(s_ini->proxy_server) != S4_ADDR(daddr.ip_addr) &&
                    (S4_ADDR(s_ini->nit_ipaddr) & S4_ADDR(s_ini->nit_ipmask)) == (S4_ADDR(daddr.ip_addr) &
                        S4_ADDR(s_ini->nit_ipmask))) {

                    printl(LOG_VERB, "Looking up NIT: [%s]", inet2str(&daddr.ip_addr, buf));
                    if (getnameinfo((const struct sockaddr *)&daddr.ip_addr, sizeof(daddr.ip_addr), daddr.name,
                        sizeof(daddr.name), 0, 0, NI_NAMEREQD)) {

                        printl(LOG_WARN, "Unable to resolve client destination address [%s] via NIT",
                            inet2str(&daddr.ip_addr, buf));
                        close(csock);
                        exit(2);
                    }
                    printl(LOG_VERB, "NIT Lookup resolved: [%s] to [%s]", inet2str(&daddr.ip_addr, buf), daddr.name);
                }

                switch (s_ini->proxy_type) {
                    case PROXY_PROTO_SOCKS_V5:
                        printl(LOG_VERB, "Initiate Socks5 protocol: hello: [%s]", inet2str(&s_ini->proxy_server, buf));

                        if (s_ini->proxy_user)
                            auth_method = socks5_client_hello(ssock, AUTH_METHOD_NOAUTH, AUTH_METHOD_UNAME,
                                AUTH_METHOD_NOACCEPT);
                        else
                            auth_method = socks5_client_hello(ssock, AUTH_METHOD_NOAUTH, AUTH_METHOD_NOACCEPT);

                        switch (auth_method) {
                            case AUTH_METHOD_NOAUTH:                    /* No authentication required */
                            break;

                            case AUTH_METHOD_UNAME:                     /* Perform user/password auth */
                                if (socks5_client_auth(ssock, s_ini->proxy_user, s_ini->proxy_password)) {
                                    printl(LOG_WARN, "Socks5 rejected user: [%s]", s_ini->proxy_user);
                                    close(csock);
                                    exit(2);
                                }
                            break;

                            case AUTH_METHOD_NOACCEPT:
                            default:
                                printl(LOG_WARN, "No (supported) auth methods were accepted by Socks5 server");
                                close(csock);
                                exit(2);
                        }

                        printl(LOG_VERB, "Initiate Socks5 protocol: request [%s] -> [%s]",
                            inet2str(&s_ini->proxy_server, suf), inet2str(&daddr.ip_addr, buf));

                        if (socks5_client_request(ssock, SOCKS5_CMD_TCPCONNECT, &daddr.ip_addr, daddr.name)) {
                            printl(LOG_CRIT, "Socks5 proxy server returned an error");
                            close(csock);
                            exit(2);
                        }
                    break;

                    case PROXY_PROTO_SOCKS_V4:
                        printl(LOG_VERB, "Initiate Socks4 protocol: request: [%s] -> [%s]",
                            inet2str(&s_ini->proxy_server, suf), inet2str(&daddr.ip_addr, buf));

                        if (socks4_client_request(ssock, SOCKS4_CMD_TCPCONNECT,
                                (struct sockaddr_in *)&daddr.ip_addr, s_ini->proxy_user) != SOCKS4_REPLY_OK) {

                            printl(LOG_WARN, "Socks4 proxy server returned an error");
                            close(csock);
                            exit(2);
                        }
                    break;

                    case PROXY_PROTO_HTTP:
                        printl(LOG_VERB, "Initiate HTTP protocol: request: [%s] -> [%s]",
                            inet2str(&s_ini->proxy_server, suf), inet2str(&daddr.ip_addr, buf));

                        if (http_client_request(ssock, &daddr.ip_addr, s_ini->proxy_user, s_ini->proxy_password)) {
                            printl(LOG_WARN, "HTTP proxy server returned an error");
                            close(csock);
                            exit(2);
                        }
                    break;

                    #if (WITH_LIBSSH2)
                        case PROXY_PROTO_SSH2:
                            if (ssh2sess || ssh2ch) {
                                printl(LOG_WARN, "Only ONE SSH2 proxy could be used per CHAIN/Connection");
                                close(csock);
                                exit(2);
                            }

                            if (!(ssh2sess = libssh2_session_init())) {
                                printl(LOG_WARN, "Unable to initialize SSH2 session");
                                close(csock);
                                exit(2);
                            }

                            printl(LOG_VERB, "Initiate SSH2 protocol: request: [%s] -> [%s]",
                                inet2str(&s_ini->proxy_server, suf), inet2str(&daddr.ip_addr, buf));

                            if (!(ssh2ch = ssh2_client_request(ssock.s, ssh2sess, &daddr, s_ini->proxy_user,
                            s_ini->proxy_password, s_ini->proxy_key, s_ini->proxy_key_passphrase,
                            s_ini->proxy_ssh_force_auth))) {
                                printl(LOG_WARN, "SSH2 proxy server returned an error");
                                close(csock);
                                exit(2);
                            }
                        break;
                    #endif

                    default:
                        /* Unreachable. Should be cleared already by read_ini() */
                        printl(LOG_WARN, "Detected unsupported proxy type: [%c]", s_ini->proxy_type);
                        close(csock);
                        exit(2);
                }
            }

            /* -- Forward connections ------------------------------------------------------------------------------- */
            cfloop:
            printl(LOG_VERB, "Starting connection-forward loop");

            /* Prepare IPC messages */
            tmessage.mtype = 1;
            memset(&tmessage.mtext, 0, sizeof(struct traffic_data));
            tmessage.mtext.pid = pid;
            tmessage.mtext.timestamp = 0;
            tmessage.mtext.caddr = caddr;
            tmessage.mtext.cbytes = 0;
            tmessage.mtext.daddr = daddr.ip_addr;
            tmessage.mtext.dbytes = 0;

            while (1) {
                #if (WITH_LIBSSH2)
                    if (ssh2ch) {
                        int wr = 0;

                        FD_ZERO(&rfd);
                        FD_SET(csock, &rfd);

                        tv.tv_sec = 0;
                        tv.tv_usec = 100000;
                        ret = select(csock + 1, &rfd, 0, 0, &tv);

                        if (ret < 0) break;
                        if (ret > 0) {
                            memset(buf, 0, BUF_SIZE);
                            tmessage.mtext.timestamp = time(NULL);                      /* Fill in traffic timestamp */
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

                                do {
                                    snd = libssh2_channel_write(ssh2ch, buf, rec);
                                    if (snd < 0) {
                                        printl(LOG_CRIT, "libssh2_channel_write() failue: [%d]", snd);
                                        break;
                                    }
                                    wr += snd;
                                } while(snd > 0 && wr < rec);

                                printl(rec != snd ? LOG_CRIT : LOG_VERB, "C:[%d] -> S:[%d] bytes", rec, wr);
                                tmessage.mtext.cbytes += rec;
                            }
                        }

                        while (1) {
                            /* Server writes */
                            rec = libssh2_channel_read(ssh2ch, buf, BUF_SIZE);
                            if (rec == LIBSSH2_ERROR_EAGAIN)
                                break;                  /* Let's try again */

                            if (rec < 0) {
                                printl(LOG_CRIT, "libssh2_channel_read() failure: [%d]", rec);
                                break;
                            }

                            wr = 0;
                            while (wr < rec) {
                                snd = send(csock, buf + wr, rec - wr, 0);
                                if (snd <= 0) {
                                    printl(LOG_CRIT, "Error sending data to proxy server");
                                    break;
                                }
                                wr += snd;
                            }

                            printl(rec != snd ? LOG_CRIT : LOG_VERB, "S:[%d] -> C:[%d] bytes", rec, wr);
                            tmessage.mtext.dbytes += rec;

                            if (libssh2_channel_eof(ssh2ch)) {
                                printl(LOG_VERB, "Connection closed by the server");
                                goto shutdown_ssh2;
                            }

                            if (msgid != -1) msgsnd(msgid, &tmessage, sizeof(struct traffic_message), IPC_NOWAIT);
                        }
                    } else {
                #endif
                    FD_ZERO(&rfd);
                    FD_SET(csock, &rfd);
                    FD_SET(ssock.s, &rfd);

                    tv.tv_sec = 0;
                    tv.tv_usec = 100000;
                    ret = select(ssock.s > csock ? ssock.s + 1: csock + 1, &rfd, 0, 0, &tv);

                    if (ret < 0) break;
                    if (ret == 0) continue;
                    if (ret > 0) {
                        memset(buf, 0, BUF_SIZE);
                        tmessage.mtext.timestamp = time(NULL);                          /* Fill in traffic timestamp */
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
                            while ((snd = send(ssock.s, buf, rec, 0)) == 0) {
                                printl(LOG_CRIT, "C:[0] -> S:[0] bytes");
                                usleep(100);                                /* 0.1 ms */
                                break;
                            }
                            if (snd == -1) {
                                printl(LOG_CRIT, "Error sending data to proxy server");
                                break;
                            }

                            printl(rec != snd ? LOG_CRIT : LOG_VERB, "C:[%d] -> S:[%d] bytes", rec, snd);
                            tmessage.mtext.cbytes += rec;
                        } else {
                            /* Server writes */
                            rec = recv(ssock.s, buf, BUF_SIZE, 0);
                            if (rec == 0) {
                                printl(LOG_INFO, "Connection closed by proxy server");
                                break;
                            }
                            if (rec == -1) {
                                printl(LOG_CRIT, "Error receving data from proxy server");
                                break;
                            }
                            while ((snd = send(csock, buf, rec, 0)) == 0) {
                                printl(LOG_CRIT, "S:[0] -> C:[0] bytes");
                                usleep(100);
                            }
                            if (snd == -1) {
                                printl(LOG_CRIT, "Error sending data to proxy server");
                                break;
                            }

                            printl(rec != snd ? LOG_CRIT : LOG_VERB, "S:[%d] -> C:[%d] bytes", rec, snd);
                            tmessage.mtext.dbytes += rec;
                        }
                        if (msgid != -1) msgsnd(msgid, &tmessage, sizeof(struct traffic_message), IPC_NOWAIT);
                    }
                #if (WITH_LIBSSH2)
                }
                #endif
            }

            #if (WITH_LIBSSH2)
                shutdown_ssh2:

                if (ssh2ch) libssh2_channel_free(ssh2ch);
                /* TODO: Should we: libssh2_session_disconnect() and libssh2_session_free() ? */
            #endif

            shutdown(csock, SHUT_RDWR);
            shutdown(ssock.s, SHUT_RDWR);
            printl(LOG_INFO, "The client finished operations");
            printl(LOG_INFO, "The client traffic summary: C: [%s]:[%llu], D: [%s]:[%llu]",
                inet2str(&caddr, suf), tmessage.mtext.cbytes, inet2str(&daddr.ip_addr, buf), tmessage.mtext.dbytes);

            #if (WITH_LIBSSH2)
                if(ssh2sess) {
                    libssh2_session_disconnect(ssh2sess, "Normal Shutdown");
                    libssh2_session_free(ssh2sess);
                }
                libssh2_exit();                                                 /* Deinitialize LIBSSH2 */
            #endif
            close(csock);
            close(ssock.s);
            exit(0);
        }
    }

    freeaddrinfo(tres);
    freeaddrinfo(sres);
    freeaddrinfo(hres);
    return 0;
}

/* ------------------------------------------------------------------------------------------------------------------ */
void trap_signal(int sig) {
    /* Signal handler */

    int	status;                                                     /* Client process status */
    pid_t cpid;


    switch (sig) {
        case SIGHUP:                                                /* Reload configuration from the INI-file */
            ini_root = delete_ini(ini_root);
            ini_root = read_ini(ifile_name);
            show_ini(ini_root, LOG_CRIT);
        break;

        case SIGINT:                                                /* Exit processes */
        case SIGQUIT:
        case SIGTERM:
            if (getpid() == mpid) {                                 /* The main daemon */
                if (Tsock != -1) {
                    shutdown(Tsock, SHUT_RDWR);
                    close(Tsock);
                }
                if (Ssock != -1) {
                    shutdown(Ssock, SHUT_RDWR);
                    close(Ssock);
                }
                if (Hsock != -1) {
                    shutdown(Hsock, SHUT_RDWR);
                    close(Hsock);
                }
                #if !defined(linux)
                    pf_close(pfd);
                #endif
                msgctl(msgid, IPC_RMID, NULL);
                mexit(0, pfile_name, tfile_name);
            } else {                                                /* A client process */
                shutdown(csock, SHUT_RDWR);
                shutdown(ssock.s, SHUT_RDWR);
                close(ssock.s);
                close(csock);
                printl(LOG_INFO, "Client exited");
                exit(0);
            }

        case SIGCHLD:
            /* Never use printf() in SIGCHLD processor, it causes SIGILL */
            while ((cpid = wait3(&status, WNOHANG, 0)) > 0) {
                pidlist_update_status(pids, cpid, status);
                cn--;
            }
        break;

        case SIGUSR1:
            show_ini(ini_root, LOG_CRIT);                           /* Display current configuration */
        break;

        case SIGUSR2:
            pidlist_show(pids, tfd);                                /* Display client's PIDs list: status and traffic */
        break;

        default:
            printl(LOG_INFO, "Got an unhandled signal: %d", sig);
        break;
    }
}

/* ------------------------------------------------------------------------------------------------------------------ */
void usage(int ecode) {
#if !defined(__APPLE__)
    printf("Usage:\n\
  ts-warp -T IP:Port -S IP:Port -H IP:Port -c file.ini -l file.log -v 0-4 -t file.act -d -p file.pid -f -u user -h\n\n\
Version:\n\
  %s-%s\n\n\
All parameters are optional:\n\
  -T IP:Port\t    Local IP address and port for incoming Transparent requests; set -T 0:0 to disables it\n\
  -S IP:Port\t    Local IP address and port for internal Socks5 server; set -S 0:0 to disables it\n\
  -H IP:Port\t    Local IP address and port for internal HTTP server; set -H 0:0 to disables it\n\
  -c file.ini\t    Configuration file, default: %s\n\
  \n\
  -l file.log\t    Main log filename, default: %s\n\
  -v 0..4\t    Log verbosity level: 0 - off, default %d\n\
  -t file.act\t    Active connections and traffic log\n\
  \n\
  -d\t\t    Daemon mode\n\
  -p file.pid\t    PID filename, default: %s\n\
  -f\t\t    Force start\n\
  \n\
  -u user\t    A user to run ts-warp, default: %s\n\
  \n\
  -h\t\t    This message\n\n",
    PROG_NAME, PROG_VERSION, INI_FILE_NAME, LOG_FILE_NAME, LOG_LEVEL_DEFAULT, PID_FILE_NAME, RUNAS_USER);
#else
    printf("Usage:\n\
  ts-warp -T IP:Port -S IP:Port -H IP:Port -c file.ini -l file.log -v 0-4 -t file.act -d -p file.pid -f -h\n\n\
Version:\n\
  %s-%s\n\n\
All parameters are optional:\n\
  -T IP:Port\t    Local IP address and port for incoming Transparent requests\n\
  -S IP:Port\t    Local IP address and port for internal Socks server\n\
  -H IP:Port\t    Local IP address and port for internal HTTP server\n\
  -c file.ini\t    Configuration file, default: %s\n\
  \n\
  -l file.log\t    Main log filename, default: %s\n\
  -v 0..4\t    Log verbosity level: 0 - off, default %d\n\
  -t file.act\t    Active connections and traffic log\n\
  \n\
  -d\t\t    Daemon mode\n\
  -p file.pid\t    PID filename, default: %s\n\
  -f\t\t    Force start\n\
  \n\
  -h\t\t    This message\n\n",
    PROG_NAME, PROG_VERSION, INI_FILE_NAME, LOG_FILE_NAME, LOG_LEVEL_DEFAULT, PID_FILE_NAME);
#endif
    exit(ecode);
}
