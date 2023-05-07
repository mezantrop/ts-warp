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


/* -- Logging ------------------------------------------------------------------------------------------------------- */
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>

#include "logfile.h"
#include "utility.h"
#include "pidfile.h"


/* ------------------------------------------------------------------------------------------------------------------ */
void printl(int level, char *fmt, ...) {
    /* Print to log */

    time_t timestamp;
    struct tm *tstamp;
    va_list ap;
    char mesg[STR_SIZE];
    
    if (level > loglevel || !fmt || !fmt[0]) return;
    if (!lfile) lfile = stderr;
    if (pid <= 0) pid = getpid();
    timestamp = time(NULL);
    tstamp = localtime(&timestamp);
    va_start(ap, fmt);
    vsnprintf(mesg, sizeof mesg , fmt, ap);
    va_end(ap);
    fprintf(lfile, "%04d.%02d.%02d %02d:%02d:%02d %s [%d]:\t%s\n", 
        tstamp->tm_year + 1900, tstamp->tm_mon + 1, tstamp->tm_mday, tstamp->tm_hour, tstamp->tm_min, tstamp->tm_sec, 
        LOG_LEVEL[level], pid, mesg);

    fflush(lfile);
}
