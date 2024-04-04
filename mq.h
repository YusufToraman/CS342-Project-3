#ifndef _MQ_H_
#define _MQ_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <semaphore.h>
#include "mf.h"

int qid = 0;

typedef struct {
    size_t messageSize;                    
    char data[]; //the size of the data section will be stored in messageSize
} Message;

typedef struct {
    char mq_name[MAX_MQNAMESIZE];
    size_t start_pos_of_queue;       
    size_t end_pos_of_queue; 
    size_t mq_data_size; //this is the max bytes that can all messages totally hold
    size_t in;       
    size_t out; 
    size_t max_messages_allowed; //total messages allowed
    int total_message_no; //current messages number
    int qid;
    sem_t* sem;
} MessageQueueHeader;

#endif
