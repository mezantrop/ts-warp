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
#include <stdint.h>


/* Program name and version */
#define PROG_NAME       "TS-Warp"
#define PROG_NAME_SHORT "TSW"
#define PROG_VERSION    "1.0.6"
#define PROG_NAME_FULL  PROG_NAME " " PROG_VERSION
#define PROG_NAME_CODE  PROG_NAME_SHORT PROG_VERSION

/* Log verbosity levels */
#define LOG_NONE    0
#define LOG_CRIT    1
#define LOG_WARN    2
#define LOG_INFO    3
#define LOG_VERB    4
#define LOG_LEVEL_DEFAULT   LOG_INFO
#define LOG_LEVEL ((char const*[]){"NONE", "CRIT", "WARN", "INFO", "VERB"})

#define BUF_SIZE        1024 * 1024
#define STR_SIZE        255

/* -- Global variables ------------------------------------------------------ */
extern uint8_t loglevel;
extern FILE *lfile;
extern int pid;
extern char *pfile_name;

/* -- Function prototypes --------------------------------------------------- */
void usage(int ecode);
void printl(int level, char *fmt, ...);
long toint(char *str);
char *init_xcrypt(int xkey_len);
void mexit(int status, char *pid_file);
