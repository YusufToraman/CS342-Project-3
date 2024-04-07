#include "mq.h"

size_t calculate_remaining_space(MessageQueueHeader* mqHeader) {
    size_t lastPos = mqHeader->end_pos_of_queue;

    if ((int)mqHeader->in >= (int)mqHeader->out) {
        return (int)lastPos - (int)mqHeader->in;
    } else {
        // in out'un arkasına düştüyse 
        return (int)mqHeader->out - (int)mqHeader->in;
    }
}

size_t calculate_available_space(MessageQueueHeader* mqHeader, size_t totalMessageSize) {
    if (calculate_remaining_space(mqHeader) < totalMessageSize) {
        if((int) mqHeader->in > (int) mqHeader->out && mqHeader->out != mqHeader->start_pos_of_queue)
        {
            mqHeader->circularity = mqHeader->in;
            mqHeader->in = mqHeader->start_pos_of_queue;
            return calculate_remaining_space(mqHeader);
        }
        else if ((int)mqHeader->in == (int)mqHeader->out)
        {
            mqHeader->in = mqHeader->start_pos_of_queue;
            mqHeader->out = mqHeader->start_pos_of_queue;
            return calculate_remaining_space(mqHeader);
        }
        else{
            printf("\ncalculate available space: no space\n");
            return -1; //no space
        }
    }
    return calculate_remaining_space(mqHeader);
}

void enqueue_message(MessageQueueHeader* mqHeader, const void* data, size_t dataSize, void* shmem) {
    size_t totalMessageSize = sizeof(Message) + dataSize;

    if (calculate_available_space(mqHeader, totalMessageSize) == -1)
    {
        fprintf(stderr, "Insufficient space for message.\n");
        return; // Or handle the error as appropriate
    }

    Message* newMessage = (Message*)((void*)shmem + mqHeader->in);
    newMessage->messageSize = dataSize;
    memcpy(newMessage->data, data, dataSize);
    
    mqHeader->in = mqHeader->in + totalMessageSize;

    mqHeader->total_message_no++;
}

int dequeue_message(MessageQueueHeader* mqHeader, void* bufptr, size_t bufsize, void* shmem) {
    if (mqHeader->total_message_no <= 0) {
        fprintf(stderr, "Queue is empty.\n");
        return -1; // Queue is empty
    }

    Message* message = (Message*)((void*)shmem + mqHeader->out);

    if (message->messageSize > bufsize) {
        fprintf(stderr, "Buffer size too small to hold the message.\n");
        return -1; // Provided buffer is too small
    }

    memcpy(bufptr, message->data, message->messageSize);

    size_t totalMessageSize = sizeof(Message) + message->messageSize;

    //memset(message, 0, sizeof(Message) + message->messageSize);

    mqHeader->out = mqHeader->out + totalMessageSize;

    if((int)mqHeader->out == (int)mqHeader->circularity){
        mqHeader->out = mqHeader->start_pos_of_queue;
    }

    mqHeader->total_message_no--;

    return message->messageSize;
}
