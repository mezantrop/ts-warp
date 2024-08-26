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


/* -- Managing list of active clients ------------------------------------------------------------------------------- */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "logfile.h"
#include "network.h"
#include "pidlist.h"


/* ------------------------------------------------------------------------------------------------------------------ */
struct pid_list *pidlist_add(struct pid_list *root, char *section_name, pid_t pid,
    struct sockaddr_storage caddr, struct sockaddr_storage daddr) {

    struct pid_list *n = NULL, *c = NULL;


    /* Create a new pidlist record structure */
    n = (struct pid_list *)malloc(sizeof(struct pid_list));
    n->pid = pid;
    n->status = -1;
    n->section_name = strdup(section_name);
    n->traffic.pid = pid;
    n->traffic.caddr = caddr;
    n->traffic.cbytes = 0;
    n->traffic.daddr = daddr;
    n->traffic.dbytes = 0;
    n->next = NULL;

    c = root;
    if (!root)
        root = n;
    else {
        while (c->next) c = c->next;
        c->next = n;
    }

    printl(LOG_INFO, "Clients list. Added PID: [%d], Section: [%s]", n->pid, n->section_name);
    return root;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int pidlist_update_status(struct pid_list *root, pid_t pid, int status) {

    struct pid_list *c = NULL;

    c = root;
    while (c) {
        if (c && c->pid == pid) {
            c->status = status;
            return 0;
        }
        c = c->next;
    }

    return 1;
}

/* ------------------------------------------------------------------------------------------------------------------ */
int pidlist_update_traffic(struct pid_list *root, struct traffic_data traffic) {

    struct pid_list *c = NULL;

    c = root;
    while (c) {
        if (c && c->pid == traffic.pid) {
            c->traffic.timestamp = traffic.timestamp;
            c->traffic.cbytes = traffic.cbytes;
            c->traffic.daddr = traffic.daddr;
            c->traffic.dbytes = traffic.dbytes;
            return 0;
        }
        c = c->next;
    }

    return 1;
}

/* ------------------------------------------------------------------------------------------------------------------ */
void pidlist_show(struct pid_list *root, int tfd) {
    char tbuf[24], buf1[STR_SIZE], buf2[STR_SIZE];
    struct pid_list *c = NULL;
    struct tm ts;

    c = root;
    dprintf(tfd, "Time,PID,Status,Section,Client,Client bytes,Target,Target bytes\n");
    while (c) {
        ts = *localtime(&c->traffic.timestamp);
        strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", &ts);

        dprintf(tfd, "%s,%d,%s,%s,%s,%llu,%s,%llu\n",
            tbuf, c->pid, c->status == -1 ? "Active" : "Finished", c->section_name,
            inet2str(&c->traffic.caddr, buf1), c->traffic.cbytes,
            inet2str(&c->traffic.daddr, buf2), c->traffic.dbytes);
        c = c->next;
    }
    (void)!write(tfd, "\n", 1);                 /* Empty line indicates end of data. (void)! - just to make GCC happy */
}
