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
#include <sys/types.h>
#include <sys/wait.h>
#include "mf.h"

#define COUNT 10

int totalcount = COUNT;

void test_messageflow_2p1mq();

int main(int argc, char **argv)
{
    totalcount = COUNT;
    if (argc != 2) {
        printf ("usage: app2 numberOfMessages\n");
        exit(1);
    }
    if (argc == 2)
        totalcount = atoi(argv[1]);

    srand(time(0));

    test_messageflow_2p1mq();

	return 0;
}


void test_messageflow_2p1mq()
{
    int ret1,   qid;
    char sendbuffer[MAX_DATALEN];
    int n_sent;
    char recvbuffer[MAX_DATALEN];
    int sentcount = 0; 
    int receivedcount = 0; 
    int i;
    
    mf_connect();
    mf_create ("mq1", 16); //  create mq;  size in KB

    ret1 = fork();
    if (ret1 ==  0) {
        //  process - P1
        //  will create a message queue
        mf_connect();
        qid = -1;
        while (1) {
            while(1){
                n_sent = rand() % MAX_DATALEN;
                if(n_sent >= MIN_DATALEN && n_sent <= MAX_DATALEN) break;
            }
            mf_send(qid, (void *) sendbuffer, n_sent);
            sentcount++;
            if (sentcount == totalcount)
                break;
        }
        mf_close(qid);
        mf_disconnect();
        exit(0);
    }
    ret1  = fork();
    if (ret1 == 0) {
        //  process - P2
        mf_connect();
        qid = mf_open("mq1");
        while (1) {
            mf_recv(qid, (void *) recvbuffer, MAX_DATALEN);
            receivedcount++;
            if (receivedcount == totalcount)
                break;
        }
        mf_close(qid);
        mf_disconnect();
        exit(0);
    }
    
    for (i = 0; i < 2; ++i)
        wait(NULL);
    
    mf_remove("mq1");
    mf_disconnect(); 
}
