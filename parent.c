// parent.c
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include "mf.h"

#define COUNT 10
char *semname1 = "/semaphore1";
char *semname2 = "/semaphore2";
char *mqname1 = "msgqueue1";

int main(int argc, char **argv) {
    int i, qid;
    char sendbuffer[MAX_DATALEN];
    int totalcount = COUNT;
    if (argc == 2) totalcount = atoi(argv[1]);

    sem_t *sem1 = sem_open(semname1, O_CREAT, 0666, 0);
    sem_t *sem2 = sem_open(semname2, O_CREAT, 0666, 0);
    srand(time(0));

    mf_connect();
    mf_create(mqname1, 16);
    qid = mf_open(mqname1);

    // Signal child process it can start
    sem_post(sem1);

    for (i = 0; i < totalcount; ++i) {
        int n_sent = rand() % MAX_DATALEN;
        mf_send(qid, sendbuffer, n_sent);
        printf("Parent sent message, datalen=%d\n", n_sent);
    }

    mf_close(qid);
    // Wait for child to finish receiving messages
    sem_wait(sem2);

    mf_remove(mqname1);
    mf_disconnect();

    return 0;
}
