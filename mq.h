#ifndef _MQ_H_
#define _MQ_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "mf.h"
#include "message.h"

int qid = 0;

typedef struct {
    Message* messages;
    size_t mqSize;
    size_t in;       
    size_t out; 
    size_t capacity;
    size_t totalMessages;
    int qid;
} MessageQueue;

#endif
