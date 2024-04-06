#ifndef _MQ_H_
#define _MQ_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include "mf.h"

typedef struct {
    size_t messageSize;                    
    char data[]; //the size of the data section will be stored in messageSize
} Message;

typedef struct {
    char mq_name[MAX_MQNAMESIZE];
    pid_t processes[2]; //sender and receiver
    size_t start_pos_of_queue;       
    size_t end_pos_of_queue; 
    size_t mq_data_size; //this is the max bytes that can all messages totally hold
    size_t in;       
    size_t out; 
    size_t max_messages_allowed; //total messages allowed
    int total_message_no; //current messages number
    int qid;
    sem_t QueueSem; //will be initalized to 1
    sem_t SpaceSem; //will be initalized to 0
    sem_t ZeroSem; //will be initalized to 0 
} MessageQueueHeader;

//MessageQueueHeader* find_mq_header_by_qid(int qid, void* shmem_base);
//MessageQueueHeader* find_mq_header_by_name(const char* mqname);
void enqueue_message(MessageQueueHeader* mqHeader, const void* data, size_t dataSize, void* shmem);
int dequeue_message(MessageQueueHeader* mqHeader, void* bufptr, size_t bufsize, void* shmem);
size_t calculate_available_space(MessageQueueHeader* mqHeader, size_t totalMessageSize);
size_t calculate_remaining_space(MessageQueueHeader* mqHeader);

#endif
