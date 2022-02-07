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

#include <time.h>
#include <netinet/in.h>
#include "utility.h"

/* ------------------------------------------------------------------------------------------------------------------ */
typedef struct traffic_data {
    pid_t pid;                                              /* TS-Warp child PID serving the client */
    time_t timestamp;
    struct sockaddr_storage caddr;                          /* Client data address - usually the TS-Warp host */
    unsigned long long cbytes;                              /* Client data volume */
    struct sockaddr_storage daddr;                          /* Destination address */
    unsigned long long dbytes;                              /* Destination data volume */
} traffic_data;

typedef struct pid_list {
    pid_t pid;                                              /* Client PID */
    int status;                                             /* Status code: -1 running, Exit: 0 - OK, >=1 - KO */
    char *section_name;                                     /* Section used by the client's process */
    struct traffic_data traffic;                            /* Traffic counters */
    struct pid_list *next;                                  /* Link to the next p_list */
} pid_list;

typedef struct traffic_message {                            /* IPC message to pass info about traffic */
    long mtype;
    struct traffic_data mtext;
} traffic_message;

/* -- Function prototypes ------------------------------------------------------------------------------------------- */
struct pid_list *pidlist_add(struct pid_list *root, char *section_name, pid_t pid,
    struct sockaddr_storage caddr, struct sockaddr_storage daddr);
int pidlist_update_status(struct pid_list *root, pid_t pid, int status);
int pidlist_update_traffic(struct pid_list *root, struct traffic_data traffic);
void pidlist_show(struct pid_list *root, int tfd);
