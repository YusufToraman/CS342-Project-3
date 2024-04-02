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
    Message* messages;  // Mesajlar için pointer dizisi.
    size_t head;        // Kuyruğun başı.
    size_t tail;        // Kuyruğun sonu.
    size_t capacity;    // Kuyruk kapasitesi (mesaj sayısı olarak).
    size_t count;       // Kuyruktaki mevcut mesaj sayısı.
    int qid;
} MessageQueue;

#endif
