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

    sem1 = sem_open(semname1, O_CREAT, 0666, 0); // Initialize semaphore
    sem2 = sem_open(semname2, O_CREAT, 0666, 0); // Initialize semaphore

    srand(time(0));
    printf("RAND_MAX is %d\n", RAND_MAX);
    
    ret = fork();
    if (ret > 0) {
        // Parent process - P1
        mf_connect();
        mf_create(mqname1, 16); // Create mq; 16 KB
        qid = mf_open(mqname1);
        sem_post(sem1); // Signal child process that queue is ready
        
        for (sentcount = 1; sentcount <= totalcount; sentcount++) {
            // Prepare the message to be sent
            snprintf(sendbuffer, sizeof(sendbuffer), "Message number %d", sentcount);
            int n_sent = strlen(sendbuffer) + 1; // +1 for null terminator
            
            // Ensure message size is within bounds
            if (n_sent < MIN_DATALEN || n_sent > MAX_DATALEN) {
                fprintf(stderr, "Message size out of bounds.\n");
                continue; // Skip this message
            }
            
            // Send the message
            ret = mf_send(qid, (void *)sendbuffer, n_sent);
            if (ret != 0) {
                fprintf(stderr, "Failed to send message\n");
                break; // Exit the loop in case of failure
            }
            
            printf("\napp sent message, datalen=%d sentcount=%d\n", n_sent, sentcount);
        }
        
        mf_close(qid);
        sem_wait(sem2); // Wait for child process to finish receiving messages
        mf_remove(mqname1);   // Remove mq
        mf_disconnect();
    } 
    else if (ret == 0) {
        // Child process - P2
        sem_wait(sem1); // Wait for parent process to signal that queue is ready
        mf_connect();
        qid = mf_open(mqname1);
        
        while (receivedcount < totalcount) {
            int n_received = mf_recv(qid, (void *)recvbuffer, MAX_DATALEN);
            if (n_received > 0) {
                printf("app received message, datalen=%d\n", n_received);
                printf("%s\n", recvbuffer); // Display the received message
                receivedcount++;
            }
        }
        
        mf_close(qid);
        mf_disconnect();
        sem_post(sem2); // Signal parent process that messages have been received
    }
    return 0;
}