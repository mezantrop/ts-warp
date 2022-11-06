/* ------------------------------------------------------------------------------------------------------------------ */
/* TS-Warp - Transparent SOCKS protocol Wrapper                                                                       */
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


/* ------------------------------------------------------------------------------------------------------------------ */
#include "version.h"

/* ------------------------------------------------------------------------------------------------------------------ */
#if !defined (PREFIX)
   #define PREFIX "/usr/local"
#endif

#define INI_FILE_NAME   PREFIX"/etc/ts-warp.ini"
#define LOG_FILE_NAME   PREFIX"/var/log/ts-warp.log"
#define PID_FILE_NAME   PREFIX"/var/run/ts-warp.pid"

#define RUNAS_USER      "nobody"


/* -- Function prototypes ------------------------------------------------------------------------------------------- */
void trap_signal(int sig);
void usage(int ecode);
