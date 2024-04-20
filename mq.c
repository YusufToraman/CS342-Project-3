#include "mq.h"

size_t calculate_remaining_space(MessageQueueHeader* mqHeader) {
    return mqHeader->end_pos_of_queue - mqHeader->in;
}

size_t calculate_available_space(MessageQueueHeader* mqHeader, size_t totalMessageSize) {
    size_t remainingSpace = calculate_remaining_space(mqHeader);
    if (remainingSpace < totalMessageSize) {
        return 0;
    }
    return remainingSpace;
}


void enqueue_message(MessageQueueHeader* mqHeader, const void* data, size_t dataSize, void* shmem) {
    size_t paddedDataSize = (dataSize + 3) & ~0x03;
    size_t totalMessageSize = sizeof(Message) + paddedDataSize;
    if (calculate_available_space(mqHeader, totalMessageSize) == 0) {
        fprintf(stderr, "Insufficient space for message.\n");
        return;
    }

    Message* newMessage = (Message*)((char*)shmem + mqHeader->in);
    newMessage->messageSize = dataSize;
    memcpy(newMessage->data, data, dataSize);
    memset(newMessage->data + dataSize, 0, paddedDataSize - dataSize);
    //out......................in
    //in
    mqHeader->in += totalMessageSize;
    mqHeader->total_message_no++;
}


int dequeue_message(MessageQueueHeader* mqHeader, void* bufptr, size_t bufsize, void* shmem) {
    if (mqHeader->total_message_no == 0) {
        fprintf(stderr, "Queue is empty.\n");
        return -1;
    }

    Message* message = (Message*)((char*)shmem + mqHeader->out);

    if (message->messageSize > bufsize) {
        fprintf(stderr, "Buffer size too small to hold the message. Required: %zu, Provided: %zu\n", message->messageSize, bufsize);
        return -1;
    }

    memcpy(bufptr, message->data, message->messageSize);

    size_t paddedDataSize = (message->messageSize + 3) & ~0x03;
    size_t totalMessageSize = sizeof(Message) + paddedDataSize;
    //out ..................in
    mqHeader->out += totalMessageSize;
    if(mqHeader->out == mqHeader->in){
        mqHeader->in = mqHeader->start_pos_of_queue;
        mqHeader->out = mqHeader->start_pos_of_queue;
    }
    mqHeader->total_message_no--;
    return message->messageSize;
}