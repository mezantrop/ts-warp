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


/* ------------------------------------------------------------------------------------------------------------------ */
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

#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "ts-warp.h"
#include "utility.h"
#include "network.h"
#include "logfile.h"
#include "pidfile.h"


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

/* ------------------------------------------------------------------------------------------------------------------ */
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

/* ------------------------------------------------------------------------------------------------------------------ */
void mexit(int status, char *pid_file) {
    /* Exit program */

    kill(0, SIGTERM);
    printl(LOG_WARN, "Clients requested to exit");
    while (wait3(&status, WNOHANG, 0) > 0) ;
    printl(LOG_CRIT, "Program finished");
    if (pid_file) {
        if (unlink(pid_file)) truncate(pid_file, 0);
        printl(LOG_WARN, "PID file removed/PID erased");
    }
    exit(status);
}
