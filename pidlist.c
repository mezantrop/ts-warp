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


/* -- Managing list of active clients ------------------------------------------------------------------------------- */
#include <stdlib.h>

#include "inifile.h"
#include "logfile.h"
#include "pidlist.h"


/* ------------------------------------------------------------------------------------------------------------------ */
struct pid_list *pidlist_add(struct pid_list *root, struct ini_section *section, pid_t pid) {

    struct pid_list *n = NULL, *c = NULL;


    /* Create a new pidlist record structure */
    n = (struct pid_list *)malloc(sizeof(struct pid_list)); 
    n->pid = pid;
    n->section = section;
    n->next = NULL;
    
    c = root;
    if (!root) 
        root = n;
    else {
        while (c->next) c = c->next;
        c->next = n;
    }

    printl(LOG_INFO, "Clients list. Added PID: [%d], Section: [%s]", n->pid, n->section->section_name);
    return root;
}

/* ------------------------------------------------------------------------------------------------------------------ */
ini_section *pidlist_del(struct pid_list **root, pid_t pid) {

    struct pid_list *d = NULL, *c = NULL;
    ini_section *ini = NULL;

    c = *root;
    if (c->pid == pid) {
        *root = c->next;
        ini = c->section;
        free(c);
    } else {
        while (c->next) {
            if (c->next->pid == pid) {
                d = c->next;
                c->next = d->next;
                ini = d->section;
                free(d);
                return ini;
            }
            c = c->next;
        }
        return NULL;
    }
    return ini;
}

/* ------------------------------------------------------------------------------------------------------------------ */
void pidlist_show(struct pid_list *root) {

    struct pid_list *c = NULL;

    c = root;
    printl(LOG_VERB, "Client PID list");
    while (c) {
        printl(LOG_VERB, "PID: [%d],\tSection: [%s]", c->pid, c->section->section_name);
        c = c->next;
    }
}