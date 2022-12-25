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


/* -- Main code ----------------------------------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <signal.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pwd.h>

#include "dns.h"
#include "ns-warp.h"
#include "logfile.h"
#include "pidfile.h"
#include "utility.h"
#include "network.h"
#include "ns-inifile.h"

/* ------------------------------------------------------------------------------------------------------------------ */
char *ifile_name = NS_INI_FILE_NAME;
char *pfile_name = NS_PID_FILE_NAME;
char *lfile_name = NS_LOG_FILE_NAME;
FILE *lfile = NULL;
int loglevel = LOG_LEVEL_DEFAULT;

pid_t pid, cpid;                                                        /* Current process id, clients PID */

int isock;                                                              /* Incoming socket */
int ssock;                                                              /* Outcoming server socket */

struct nit *nit_root;                                                   /* The list of NIT structures */


/* ------------------------------------------------------------------------------------------------------------------ */
int main (int argc, char* argv[]) {
    int flg;                                                            /* Command-line options flag */
    char *iaddr = LISTEN_DEFAULT;                                       /* Our (incomming) address and... */
    char *iport = NS_LISTEN_PORT;                                       /* ...a port to accept clients */
    char *saddr = NULL;                                                 /* DNS server address and... */
    char *sport = DNS_PORT;                                             /* ...a port to forward requests to */
    int l_flg = 0;                                                      /* User didn't set the log file */
    int d_flg = 0;                                                      /* Daemon mode */
    int f_flg = 0;                                                      /* Force start */

    struct addrinfo ihints, *ires = NULL;                               /* NS-Warp incoming address info structures */
    struct addrinfo shints, *sres = NULL;
    struct sockaddr caddr;                                              /* Client address and ... */
    socklen_t caddrlen = sizeof caddr;                                  /* ... its length */
    struct sockaddr oaddr;                                              /* Address for outgoing requests */
    struct sockaddr q_ip;
    char q_name[STR_SIZE];

    fd_set rfd;
    struct timeval tv;
    
    int rec, snd;                                                       /* received/sent bytes */
    unsigned char dns_buf[2 * STR_SIZE];                                /* DNS messages buffer */
    dns_header *dnsh;                                                   /* Pointer to DNS message header */
    dns_question dnsq;                                                  /* DNS message question section */
    unsigned char *dnsq_raw;                                            /* The raw query part of the DNS message */
    int dnsq_siz;                                                       /* Size of the raw query part */

    char str_buf[STR_SIZE];                                             /* Multipurpose buffer */
    char *qname;
    int ret;                                                            /* Various function return codes */

    char *runas_user = RUNAS_USER;                                      /* A user to run ts-warp */


    while ((flg = getopt(argc, argv, "i:s:c:l:v:dp:fh")) != -1)
            switch(flg) {
                case 'i':                                               /* Our IP/name */
                    iaddr = strsep(&optarg, ":");                       /* IP:PORT */
                    if (optarg) iport = optarg;
                    break;
                case 's':                                               /* Remote DNS server IP/name */
                    saddr = strsep(&optarg, ":");                       /* IP:PORT */
                    if (optarg) sport = optarg;
                    break;
                case 'c':                                               /* INI-file */
                    ifile_name = optarg; break;
                case 'l': 
                    l_flg = 1; lfile_name = optarg; break;              /* Logfile */
                case 'v':                                               /* Log verbosity */
                    loglevel = (uint8_t)toint(optarg); break;
                case 'd':                                               /* Daemon mode */
                    d_flg = 1; break;
                case 'p':
                    pfile_name = optarg; break;                         /* PID-file */
                case 'f':                                               /* Force start */
                    f_flg = 1; break;
                case 'h':                                               /* Help */
                default:
                    (void)usage(0);
            }

    if (!iaddr[0]) iaddr = LISTEN_DEFAULT;
    if (!iport[0]) iport = NS_LISTEN_PORT;
    if (!sport[0]) sport = DNS_PORT;

    /* Open log-file */
    if (!d_flg && !l_flg) {
        lfile = stdout;
        printl(LOG_INFO, "Log file: [STDOUT], verbosity level: [%d]", loglevel);
    } else if (!(lfile = fopen(lfile_name, "a"))) {
        printl(LOG_WARN, "Unable to open log: [%s], now trying: [%s]", lfile_name, NS_LOG_FILE_NAME);
        lfile_name = NS_LOG_FILE_NAME;
        if (!(lfile = fopen(lfile_name, "a"))) {
            printl(LOG_CRIT, "Unable to open the default log: [%s]", lfile_name);
            exit(1);
        }
        printl(LOG_INFO, "Log file: [%s], verbosity level: [%d]", lfile_name, loglevel);
    }
    printl(LOG_INFO, "ns-warp incoming address: [%s:%s]", iaddr, iport);

    struct passwd *pwd = getpwnam(runas_user);

    if (d_flg) {
        /* -- Daemonizing ------------------------------------------------------------------------------------------- */
        /* signal(SIGHUP, trap_signal); */
        signal(SIGINT, trap_signal);
        signal(SIGQUIT, trap_signal);
        signal(SIGTERM, trap_signal);
        signal(SIGCHLD, trap_signal);
        /* signal(SIGUSR1, trap_signal); */

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

        printl(LOG_CRIT, "%s-%s daemon started", NS_PROG_NAME, NS_PROG_VERSION);
        pid = mk_pidfile(pfile_name, f_flg, pwd ? pwd->pw_uid : 0, pwd ? pwd->pw_gid : 0);
    }

    nit_root = read_ini(ifile_name);
    show_ini(nit_root);

    /* -- Try validating our address for incoming connections ------------------------------------------------------- */
    memset(&ihints, 0, sizeof ihints);
    ihints.ai_family = PF_UNSPEC;
    ihints.ai_socktype = SOCK_DGRAM;
    ihints.ai_flags = AI_PASSIVE;
    if ((ret = getaddrinfo(iaddr, iport, &ihints, &ires)) > 0) {
        printl(LOG_CRIT, "Error resolving ns-warp address [%s]: %s", iaddr, gai_strerror(ret));
        exit(1);
    }
    printl(LOG_INFO, "ns-warp address [%s] succesfully resolved to [%s]", iaddr, inet2str(ires->ai_addr, str_buf));

    /* -- Create socket for the incoming requests ------------------------------------------------------------------- */
    if ((isock = socket(ires->ai_family, ires->ai_socktype, ires->ai_protocol)) == -1) {
        printl(LOG_CRIT, "Error creating a socket for incoming requests");
        exit(1);
    }
    printl(LOG_VERB, "Socket for incoming requests created");

    if (bind(isock, ires->ai_addr, ires->ai_addrlen) < 0) {
        printl(LOG_CRIT, "Error binding socket for the incoming requests");
        close(isock);
        exit(1);
    }
    printl(LOG_VERB, "The socket for incoming requests succesfully bound");

    /* -- Create socket for outgoing requests ----------------------------------------------------------------------- */
    if ((ssock = socket(ires->ai_family, SOCK_DGRAM, 0)) == -1) {
		printl(LOG_CRIT, "Error creating a socket for outgoing DNS-requests: [%s]", strerror(errno));
        exit(1);
    }
    printl(LOG_VERB, "Socket for outgoing DNS-requests created");

	memset(&oaddr, 0, sizeof(struct sockaddr));
    //oaddr.sa_family = ires->ai_family;
    SA_FAMILY(oaddr) = ires->ai_family;
	if (bind(ssock, (struct sockaddr *)&oaddr, sizeof(oaddr)) != 0) {
		printl(LOG_CRIT, "Error binding socket for outgoing DNS-requests: [%s]", strerror(errno));
        exit(1);
    }
    printl(LOG_VERB, "Socket for outgoing DNS-requests succesfully bound");

    /* TODO: Implement */
    /* if (setuid(pwd->pw_uid) && setgid(pwd->pw_gid)) {
        printl(LOG_CRIT, "Failed to lower privilege level to UID:GID [%d:%d]", pwd->pw_uid, pwd->pw_gid);
        exit(1);
    } */

    /* -- Try validating DNS address to forward requests to --------------------------------------------------------- */
    memset(&shints, 0, sizeof shints);
    shints.ai_family = AF_INET;
    shints.ai_socktype = SOCK_DGRAM;
    if ((ret = getaddrinfo(saddr, sport, &shints, &sres)) > 0) {
        printl(LOG_CRIT, "Error resolving DNS-server address: [%s]:[%s] %s", saddr, sport, gai_strerror(ret));
        exit(1);
    }

    printl(LOG_INFO, "DNS-server address [%s] succesfully resolved to [%s]", saddr, inet2str(sres->ai_addr, str_buf));
    
    /* -- Process requests ------------------------------------------------------------------------------------------ */
    while (1) {
        FD_ZERO(&rfd);
        FD_SET(isock, &rfd);
        FD_SET(ssock, &rfd);

        tv.tv_sec = 0;
        tv.tv_usec = 10000;

        ret = select(ssock > isock ? ssock + 1: isock + 1, &rfd, 0, 0, &tv);
        
        if (ret < 0) break;   
        if (ret == 0) continue;
        if (ret > 0) {
            memset(dns_buf, 0, sizeof(dns_buf));
            if (FD_ISSET(isock, &rfd)) {
                /* -- Client writes --------------------------------------------------------------------------------- */
                printl(LOG_VERB, "Client sends a message");

                rec = recvfrom(isock, dns_buf, sizeof(dns_buf), 0, (struct sockaddr *)&caddr, &caddrlen);
                if (rec == 0) {
                    printl(LOG_VERB, "Client sent zero data");
                    continue;
                }
                if (rec == -1) {
                    printl(LOG_CRIT, "Error receving data from the client");
                    continue;
                }
                if (rec < 12) {                                              /* DNS packet minimum length */
                    printl(LOG_CRIT, "DNS query too short from %s", inet2str(&caddr, str_buf));
                    continue;
                }

                /* -------------------------------------------------------------------------------------------------- */
                /*  1. Parse the clients request
                    2. Seek NIT for IP or Name depending on the request type: A/AAAA/PTR
                    3. If found in NIT, build a new reply with Name or IP from NIT
                    4. If NOT found in NIT, forward request to the DNS server */
                /* -------------------------------------------------------------------------------------------------- */

                /* Parse DNS header */
                dnsh = (dns_header *)dns_buf;
                if ((ntohs(dnsh->flags) & NS_FLAGS_QR_RESPONSE) == NS_FLAGS_QR_QUERY) {
                    /* Parse only QUERIES */

                    /* Parse DNS questions section */
                    memset(str_buf, 0, sizeof(str_buf));
                    qname = (char*)dns_buf + sizeof(dns_header);
                    while (*qname) {
                        strncat(str_buf, qname + 1, *qname);
                        if (*(qname + *qname + 1) != 0) strcat(str_buf, ".");
                        qname += *qname + 1;
                    }
                    dnsq.name = strdup(str_buf);
                    dnsq.type = (uint16_t)(*(qname += 2));
                    dnsq.classc = (uint16_t)(*(qname += 2));

                    /* Save the raw query part to send it back in a reply */
                    dnsq_siz = rec - sizeof(dns_header);
                    dnsq_raw = malloc(dnsq_siz);
                    memcpy(dnsq_raw, dns_buf + sizeof(dns_header), dnsq_siz);

                    printl(LOG_VERB, 
                        "DNS Query ID: [%04X] HEADER Flags: [%04X] Qdcount: [%04X] QUESTION Name: [%s] Type: [%04X] Class [%04X]", 
                        ntohs(dnsh->id), ntohs(dnsh->flags), ntohs(dnsh->qdcount), dnsq.name, dnsq.type, dnsq.classc);

                    if (dnsq.classc == 0x0001) {                        /* != IN */
                        switch (dnsq.type) {
                            case NS_MESSAGE_TYPE_A:
                                switch (nit_lookup_name(nit_root, dnsq.name, AF_INET, &q_ip)) {
                                    case 0:
                                        printl(LOG_VERB, "Found the Name: [%s] in NIT has the IP: [%s]", 
                                            dnsq.name, inet2str(&q_ip, str_buf));
                                        if (!(rec = dns_reply_a(dnsh->id, dnsq_raw, dnsq_siz, &q_ip, dns_buf)))
                                            continue;
                                        free(dnsq.name);
                                        free(dnsq_raw);
                                        goto snd_client;
                                        break;
                                    case 2:
                                        printl(LOG_VERB, "[%s] is found in NIT but not in IPv4 range", dnsq.name);
                                        rec = dns_reply_nfound(dnsh->id, htons(dnsq.type), dnsq_raw, dnsq_siz, dns_buf);
                                        free(dnsq.name);
                                        free(dnsq_raw);
                                        goto snd_client;
                                        break;
                                    case 1:
                                    default:
                                        printl(LOG_VERB, "[%s] is not found in NIT", dnsq.name);
                                        break;
                                }
                                break;

                            case NS_MESSAGE_TYPE_AAAA:
                                switch (nit_lookup_name(nit_root, dnsq.name, AF_INET6, &q_ip)) {
                                    case 0:
                                        printl(LOG_VERB, "Found the Name: [%s] in NIT has the IP: [%s]", 
                                            dnsq.name, inet2str(&q_ip, str_buf));
                                        
                                        if (!(rec = dns_reply_a(dnsh->id, dnsq_raw, dnsq_siz, &q_ip, dns_buf)))
                                            continue;
                                        free(dnsq.name);
                                        free(dnsq_raw);
                                        goto snd_client;
                                        break;
                                    case 2:
                                        printl(LOG_VERB, "[%s] is found in NIT but not in IPv6 range", dnsq.name);
                                        rec = dns_reply_nfound(dnsh->id, htons(dnsq.type), dnsq_raw, dnsq_siz, dns_buf);
                                        free(dnsq.name);
                                        free(dnsq_raw);
                                        goto snd_client;
                                        break;
                                    case 1:
                                    default:
                                        printl(LOG_VERB, "The name: [%s] is not found in NIT", dnsq.name);
                                        break;
                                }
                                break;

                            case NS_MESSAGE_TYPE_PTR:
                                q_ip = forward_ip(dnsq.name);
                                switch (nit_lookup_ip(nit_root, &q_ip, q_name)) {
                                    case 0:
                                        printl(LOG_VERB, "Found the Name: [%s] in NIT has the IP: [%s]", 
                                            q_name, inet2str(&q_ip, str_buf));
                                        rec = dns_reply_ptr(dnsh->id, dnsq_raw, dnsq_siz, q_name, dns_buf);
                                        free(dnsq.name);
                                        free(dnsq_raw);
                                        goto snd_client;
                                        break;
                                    case 2:
                                        printl(LOG_VERB, "The name: [%s] is not (yet) registered with NIT", dnsq.name);
                                        rec = dns_reply_nfound(dnsh->id, htons(dnsq.type), dnsq_raw, dnsq_siz, dns_buf);
                                        free(dnsq.name);
                                        free(dnsq_raw);
                                        goto snd_client;
                                        break;
                                    case 1:
                                    default:
                                        printl(LOG_VERB, "The name: [%s] is not found in NIT", dnsq.name);
                                        break;
                                }
                                break;

                            default:
                                printl(LOG_VERB, "Bypassing DNS query type [%d]", dnsq.type);
                                break;
                        }
                    }

                    free(dnsq.name);
                    free(dnsq_raw);

                    /* Debug */
                    /* show_ini(nit_root); */
                }
                
                /* -------------------------------------------------------------------------------------------------- */
                snd = sendto(ssock, dns_buf, rec, 0, sres->ai_addr, sres->ai_addrlen);
                if (snd == -1) {
                    printl(LOG_CRIT, "Error forwarding data to DNS-server: [%s]", strerror(errno));
                    continue;
                }

                if (rec != snd)
                    printl(LOG_CRIT, "C:[%d] -> S:[%d] bytes", rec, snd);
                else
                    printl(LOG_VERB, "C:[%d] -> S:[%d] bytes", rec, snd);                    
            } else {
                /* -- DNS server writes ----------------------------------------------------------------------------- */
                printl(LOG_VERB, "DNS server sends a message");

                rec = recvfrom(ssock, dns_buf, sizeof(dns_buf), 0, sres->ai_addr, &sres->ai_addrlen);
                if (rec == 0) {
                    printl(LOG_VERB, "DNS server sent zero data");
                    continue;
                }
                if (rec == -1) {
                    printl(LOG_CRIT, "Error receving data from the DNS-server");
                    continue;
                }
                if (rec < 12) {                                              /* DNS packet minimum length */
                    printl(LOG_CRIT, "DNS reply too short from %s", inet2str(sres->ai_addr, str_buf));
                    continue;
                }
snd_client:
                /* Everything is OK, just forward the message to the client */
                snd = sendto(isock, dns_buf, rec, 0, &caddr, sizeof(struct sockaddr));
                if (snd == -1) {
                    printl(LOG_CRIT, "Error forwarding data to the client");
                    break;
                }

                if (rec != snd)
                    printl(LOG_CRIT, "S:[%d] -> C:[%d] bytes", rec, snd);
                else
                    printl(LOG_VERB, "S:[%d] -> C:[%d] bytes", rec, snd);
            }
        }
    }

    return 0;
}

/* ------------------------------------------------------------------------------------------------------------------ */
void trap_signal(int sig) {
    /* Signal handler */
    pid_t cpid;
    int status;


    switch (sig) {
        /* case SIGHUP: */                                          /* TODO: implement */
            /* ini_root = delete_ini(nit_root);
            nit_root = read_ini(ifile_name);
            show_ini(nit_root);
            break; */
        case SIGINT:                                                /* Exit processes */
        case SIGQUIT:
        case SIGTERM:
            shutdown(isock, SHUT_RDWR);
            shutdown(ssock, SHUT_RDWR);
            close(isock);
            close(ssock);
            if (pfile_name) {
                if (unlink(pfile_name)) truncate(pfile_name, 0);
                printl(LOG_WARN, "PID file removed/PID erased");
            }
            exit(0);
            break;

        case SIGCHLD:
            /* Never use printf() in SIGCHLD processor, it causes SIGILL */
            while ((cpid = wait3(&status, WNOHANG, 0)) > 0) ;
            break;

        /* case SIGUSR1: */                                         /* TODO: Show current configuration */
        /*  show_ini(nit_root);
            break;*/

        default:
            printl(LOG_INFO, "Got unhandled signal: %d", sig);
            break;
    }
}

/* ------------------------------------------------------------------------------------------------------------------ */
void usage(int ecode) {
    printf("Usage:\n\
  ns-warp -i IP:Port -s IP:Port -c file.ini -l file.log -v 0-4 -d -p file.pid -h\n\n\
All parameters are optional:\n\
  -i IP:Port\t    NS-Warp server IP address and port\n\
  -s IP:Port\t    Original DNS server IP address and port\n\
  \n\
  -c file.ini\t    Configuration INI-file, default: %s\n\
  \n\
  -l file.log\t    Log filename, default: %s\n\
  -v 0..4\t    Log verbosity level: 0 - off, default %d\n\
  \n\
  -d\t\t    Daemon mode\n\
  -p file.pid\t    PID filename, default: %s\n\
  \n\
  -h\t\t    This message\n\n", NS_INI_FILE_NAME, NS_LOG_FILE_NAME, LOG_LEVEL_DEFAULT, NS_PID_FILE_NAME);

    exit(ecode);
}
