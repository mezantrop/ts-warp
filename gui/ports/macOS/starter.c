/* -------------------------------------------------------------------------- */
/* Trivial starter program to launch other applications                       */
/* -------------------------------------------------------------------------- */

/*
  "THE BEER-WARE LICENSE" (Revision 42):
  zmey20000@yahoo.com wrote this file. As long as you retain this notice you
  can do whatever you want with this stuff. If we meet some day, and you think
  this stuff is worth it, you can buy me a beer in return Mikhail Zakharov
*/

/* -------------------------------------------------------------------------- */

/*
  2024.08.29    v1.0    Initial release
  2024.09.01    v1.1    Handle environment variables
  2024.09.04    v1.2    Pass original starter name as the started app name
  2024.09.06    v1.3    Pass app args; allow app handling UID/EUID; Drop environ
*/


/* -------------------------------------------------------------------------- */
#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include <unistd.h>

/* -------------------------------------------------------------------------- */
#define APP_NAME        "/app"
#define DIRBUFSZ        256

/* -------------------------------------------------------------------------- */
int main(int argc, char *argv[]) {
    char buf[DIRBUFSZ];

   if (argc == 2 && !strncmp(argv[1], "-h", 2)) {
        fprintf(stdout, "Run a program named app from the same directory\n");
        return 0;
    }

    if (argv[0][0] == '/')
        strncpy(buf, dirname(argv[0]), DIRBUFSZ);
    else
        getcwd(buf, DIRBUFSZ);
    strncat(buf, APP_NAME, DIRBUFSZ);
    argv[0] = basename(argv[0]);

    return execv(buf, argv);
}
