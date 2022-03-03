/* -------------------------------------------------------------------------- */ 
/* TS-Warp - Transparent SOCKS protocol Wrapper                               */
/* -------------------------------------------------------------------------- */ 

/* Copyright (c) 2021, 2022, Mikhail Zakharov <zmey20000@yahoo.com>

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


/* -------------------------------------------------------------------------- */
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
#include "pidfile.h"

/* -------------------------------------------------------------------------- */
void usage(int ecode) {
    printf("Usage:\n\
\tts-warp [-I IP] [-i port] [-l file.log] [-v 0-4] [-d] [-c file.ini] [-h]\n\n\
Options:\n\
\t-I IP\t\tIncoming local IP address and ...\n\
\t-i port\t\t... a port number to accept connections on\n\n\
\t-l file.log\tLog filename\n\
\t-v 0..4\t\tLog verbosity level: 0 - off, default 2\n\n\
\t-d\t\tDaemon mode\n\
\t-f\t\tForce start\n\n\
\t-c file.ini\tConfiguration file\n\n\
\t-h\t\tThis message\n\n");

    exit(ecode);
}

/* -------------------------------------------------------------------------- */
void printl(int level, char *fmt, ...) {
    /* Print to log */

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
            mexit(1, pfile_name);
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
        mexit(1, pfile_name);
    }
        
    if (free_mem) free(hints);
    return res->ai_addr;
}

/* -------------------------------------------------------------------------- */
void mexit(int status, char *pid_file) {
    /* Exit program */

    kill(0, SIGTERM);
    printl(LOG_WARN, "Clients requested to exit");
    while (wait3(&status, WNOHANG, 0) > 0) ;
    printl(LOG_CRIT, "Program finished");
    if (pid_file) {
        unlink(pid_file);
        printl(LOG_WARN, "PID file removed");
    }
    exit(status);
}
