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
#include <stdio.h>
#include <libgen.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* -------------------------------------------------------------------------- */
#define APP_NAME	"app"
#define DIRBUFSZ	256

/* -------------------------------------------------------------------------- */
int main(int argc, char *argv[]) {
    char buf[DIRBUFSZ]; 

    if (argc > 2 || (argc == 2 && !strncmp(argv[1], "-h", 2))) {
        fprintf(stdout,
	        "Run a program (default name: app) in the same directory\n\n"
                "Usage: %s [program]\n", basename(argv[0]));
	return 0;
    }

    setuid(geteuid());

    if (argv[0][0] == '/')
        strncpy(buf, dirname(argv[0]), DIRBUFSZ);
    else 
        getcwd(buf, DIRBUFSZ);
    sprintf(buf + strnlen(buf, DIRBUFSZ), "/%s", APP_NAME);

    return execl(buf, APP_NAME, NULL);
}

