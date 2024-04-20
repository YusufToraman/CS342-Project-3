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
void test_messageflow_4p2mq();


int
main(int argc, char **argv)
{
    totalcount = COUNT;
    if (argc != 2) {
        printf ("usage: app2 numberOfMessages\n");
        exit(1);
    }
    if (argc == 2)
        totalcount = atoi(argv[1]);

    srand(time(0));

    //test_messageflow_2p1mq();
    test_messageflow_4p2mq();

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
        qid = mf_open("mq1");
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


void test_messageflow_4p2mq()
{
    int ret1, qid;
    char sendbuffer[MAX_DATALEN];
    int n_sent;
    char recvbuffer[MAX_DATALEN];
    int sentcount = 0;
    int receivedcount = 0;
    int i;
    
    mf_connect();
    mf_create ("mq1", 16); //  create mq;  size in KB
    mf_create ("mq2", 16); //  create mq;  size in KB

    ret1 = fork();
    if (ret1 == 0) {
        printf("\nChild 1\n");
        // P1
        // P1 will send
        srand(time(0));
        mf_connect();
        qid = mf_open("mq1");
        while (1) {
            while(1){
                n_sent = rand() % MAX_DATALEN;
                if(n_sent >= MIN_DATALEN && n_sent <= MAX_DATALEN) break;
            }
            mf_send(qid, (void *) sendbuffer, n_sent);
            printf("\nsentcount:%d\n", sentcount);
            sentcount++;
            if (sentcount == totalcount)
                break;
        }
        printf("\nNERDESIN PANKO 1\n");
        mf_close(qid);
        printf("\nCLOSED 1\n");
        mf_disconnect();
        exit(0);
    }
    ret1 = fork();
    if (ret1 == 0) {
        printf("\nChild 2\n");
        // P2
        // P2 will receive
        srand(time(0));
        mf_connect();
        qid = mf_open("mq1");
        while (1) {
            mf_recv(qid, (void *) recvbuffer, MAX_DATALEN);
            receivedcount++;
            printf("\nreceivedcount:%d\n", receivedcount);
            if (receivedcount == totalcount)
                break;
        }
        printf("\nNERDESIN PANKO 2\n");
        mf_close(qid);
        printf("\nCLOSED 2\n");
        mf_disconnect();
        exit(0);
    }
    

    ret1 = fork();
    if (ret1 == 0) {
        printf("\nChild 3\n");
        // P3
        // P3 will send
        srand(time(0));
        mf_connect();
        qid = mf_open("mq2");
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
        printf("\nNERDESIN PANKO 3\n");
        mf_close(qid);
        printf("\nCLOSED 3\n");
        mf_disconnect();
        exit(0);
    }
    ret1 = fork();
    if (ret1 == 0) {
        printf("\nChild 4\n");
        // P4
        // P4 will receive
        srand(time(0));
        mf_connect();
        qid = mf_open("mq2");
        while (1) {
            mf_recv(qid, (void *) recvbuffer, MAX_DATALEN);
            receivedcount++;
            if (receivedcount == totalcount)
                break;
        }
        printf("\nNERDESIN PANKO 4\n");
        mf_close(qid);
        printf("\nCLOSED 4\n");
        mf_disconnect();
        exit(0);
    }
    printf("\nHADI BAKIM\n");
    for (i = 0; i < 4; ++i)
        wait(NULL);
    
    mf_remove("mq1");
    mf_remove("mq2");
    mf_disconnect();
}