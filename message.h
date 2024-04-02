#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include <stdio.h>
#include <string.h>
#include "mf.h"

typedef struct {
    size_t len;                     // Mesajın boyutu.
    char data[MAX_DATALEN];    // Mesajın içeriği.
} Message;

#endif