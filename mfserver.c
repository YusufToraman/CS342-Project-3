#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "mf.h"
#include <signal.h>

// write the signal handler function
// it will call mf_destroy()
//
void signal_handler(int sig) {
    printf("Signal %d received, cleaning up...\n", sig);
    mf_destroy();
    exit(0);
}

int main(int argc, char *argv[])
{
    printf("mfserver pid=%d\n", (int) getpid());

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    mf_init();
    while (1) {
        sleep(1000);
    }

    return 0;
}


