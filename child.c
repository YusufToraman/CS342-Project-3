// child.c
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "mf.h"

#define COUNT 10
char *semname1 = "/semaphore1";
char *semname2 = "/semaphore2";
char *mqname1 = "msgqueue1";

int main() {
    int qid;
    char recvbuffer[MAX_DATALEN];
    int totalcount = COUNT;

    sem_t *sem1 = sem_open(semname1, O_CREAT, 0666, 0);
    sem_t *sem2 = sem_open(semname2, O_CREAT, 0666, 0);

    // Wait for parent to signal it's ready
    sem_wait(sem1);

    mf_connect();
    qid = mf_open(mqname1);

    for (int i = 0; i < totalcount; ++i) {
        int n_received = mf_recv(qid, recvbuffer, MAX_DATALEN);
        printf("Child received message, datalen=%d\n", n_received);
    }

    mf_close(qid);
    mf_disconnect();

    // Signal parent that all messages are received
    sem_post(sem2);

    return 0;
}
