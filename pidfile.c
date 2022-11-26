/* ------------------------------------------------------------------------------------------------------------------ */
/* TS-Warp - Transparent SOCKS proxy Wrapper                                                                          */
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


/* -- PID-file handling --------------------------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "pidfile.h"
#include "logfile.h"
#include "utility.h"

extern char *pfile_name;

/* ------------------------------------------------------------------------------------------------------------------ */
char *rd_pidfile(char *file_name) {
    FILE *pfile;
    static char pid[9];                                                 /* Could PID be of long long type? */
     if ((pfile = fopen(file_name, "r")))
        if (fgets(pid, sizeof pid, pfile)) {
            printl(LOG_VERB, "Daemon running, pid: [%s]", pid);
            fclose(pfile);
            return pid;
        }
    printl(LOG_VERB, "No PID-file found");
    if (pfile) fclose(pfile);
    return NULL;
}

/* ------------------------------------------------------------------------------------------------------------------ */
pid_t wr_pidfile(char *file_name, uid_t owner, uid_t group) {
    FILE *pfile;
    pid_t pid = 0;
     
    if ((pfile = fopen(file_name, "w"))) {
        pid = getpid();
        printl(LOG_INFO, "PID file: [%s], PID: [%d]", file_name, pid);
        if (fprintf(pfile, "%d\n", pid) == -1) {
            printl(LOG_CRIT, "Unable to write the PID file: [%s]", file_name);
            mexit(1, pfile_name);
        }
    } else {
        printl(LOG_CRIT, "Unable to open the PID file: [%s]", file_name);
        mexit(1, pfile_name);
    }

    if (chown(file_name, owner, group)) {
        printl(LOG_CRIT, "Unable to chown(%d%d) the PID file: [%s]", owner, group, file_name);
        exit(1);
    }
    fclose(pfile);
    return pid;
}

/* ------------------------------------------------------------------------------------------------------------------ */
pid_t mk_pidfile(char *file_name, int f_flg, uid_t owner, uid_t group) {
    /* Create a file and write PID there */

    if (f_flg)
        printl(LOG_WARN, "Force start!");
    else
        if (rd_pidfile(file_name)) {
            printl(LOG_CRIT, "Unable to start. Daemon is already running!");
            mexit(1, pfile_name);
        }

    return wr_pidfile(file_name, owner, group);
}

/* ------------------------------------------------------------------------------------------------------------------ */
int rm_pidfile(char *file_name) {
    /* Just a wrapper for unlink() */
    return unlink(file_name);
}
