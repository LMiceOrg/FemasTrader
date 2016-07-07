#include <lmspi.h>              /* LMICED spi */

#include <eal/lmice_trace.h>    /* EAL tracing */

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

/* print usage of app */
static void print_usage(void);

/* Daemon */
static int init_daemon(int silent);


int main(int argc, char* argv[]) {
    int i;
    (void)argc;
    (void)argv;

    /** Process cmdline */
    for(i=1; i<argc; ++i) {

    }

    return 0;
}

void print_usage(void) {
    printf("Logger:\t\t-- a log app --\n"
           "\t-h, --help\t\tShow this message\n"
           "\t-c, --console\t\tShow in console mode\n"
           "\t-f, --file\t\tWrite log in files\n"
           "\t-d, --dir\t\tSet log files folder\n"
           );
}
