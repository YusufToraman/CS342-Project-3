#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <time.h>
#include "mf.h"

#define COUNT 10
char *semname1 = "/semaphore1";
char *semname2 = "/semaphore2";
sem_t *sem1, *sem2;
char *mqname1 = "msgqueue1";

int main(int argc, char **argv) {
    int ret, qid;
    char sendbuffer[MAX_DATALEN];
    char recvbuffer[MAX_DATALEN];
    int sentcount = 0;
    int receivedcount = 0;
    int totalcount = COUNT;
    
    if (argc == 2)
        totalcount = atoi(argv[1]);

    sem1 = sem_open(semname1, O_CREAT, 0666, 0);
    sem2 = sem_open(semname2, O_CREAT, 0666, 0);

    srand(time(0));
    printf("RAND_MAX is %d\n", RAND_MAX);
    
    ret = fork();
    if (ret > 0) {
        mf_connect();
        int mq1 = mf_create(mqname1, 16);
        if(mq1 != 0)
        {
            fprintf(stderr, "MF Create Error.\n");
            return -1;
        }
        qid = mf_open(mqname1);
        sem_post(sem1);
        
        for (sentcount = 1; sentcount <= totalcount; sentcount++) {
            snprintf(sendbuffer, sizeof(sendbuffer), "Message number %d", sentcount);
            int n_sent = strlen(sendbuffer) + 1;
            
            if (n_sent < MIN_DATALEN || n_sent > MAX_DATALEN) {
                fprintf(stderr, "Message size out of bounds.\n");
                continue;
            }
            
            ret = mf_send(qid, (void *)sendbuffer, n_sent);
            if (ret != 0) {
                fprintf(stderr, "Failed to send message\n");
                break;
            }
            
            printf("\napp sent message, datalen=%d sentcount=%d\n", n_sent, sentcount);
        }
        
        mf_close(qid);
        sem_wait(sem2);
        mf_remove(mqname1);
        mf_disconnect();
    } 
    else if (ret == 0) {
        sem_wait(sem1);
        mf_connect();
        qid = mf_open(mqname1);
        
        while (receivedcount < totalcount) {
            int n_received = mf_recv(qid, (void *)recvbuffer, MAX_DATALEN);
            if (n_received > 0) {
                printf("app received message, datalen=%d\n", n_received);
                printf("%s\n", recvbuffer);
                receivedcount++;
            }
        }
        
        mf_close(qid);
        mf_disconnect();
        sem_post(sem2);
    }
    return 0;
}