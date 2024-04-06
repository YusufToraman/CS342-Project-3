#include "mq.h"

size_t calculate_remaining_space(MessageQueueHeader* mqHeader) {
    //END YANLIŞ SANIRIM 
    size_t lastPos = mqHeader->end_pos_of_queue;

    if (mqHeader->in >= mqHeader->out) {
        return lastPos - mqHeader->in;
    } else {
        // in out'un arkasına düştüyse 
        return mqHeader->out - mqHeader->in;
    }
}

size_t calculate_available_space(MessageQueueHeader* mqHeader, size_t totalMessageSize) {
    // Ensure there's enough space for the message
    if (calculate_remaining_space(mqHeader) < totalMessageSize) {
        if( mqHeader->in > mqHeader->out && mqHeader->out != mqHeader->start_pos_of_queue)
        {
            mqHeader->in = mqHeader->start_pos_of_queue;
            return calculate_remaining_space(mqHeader);
        }
        else if (mqHeader->in = mqHeader->out)
        {
            mqHeader->in = mqHeader->start_pos_of_queue;
            mqHeader->out = mqHeader->start_pos_of_queue;
            return calculate_remaining_space(mqHeader);
        }
        else{
            return -1; //no space
        }
    }
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
    memcpy(newMessage->data, data, dataSize); // Copy the message data
    
    mqHeader->in = mqHeader->in + totalMessageSize;

    mqHeader->total_message_no++; // Increment the message count
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

    mqHeader->out = mqHeader->out + totalMessageSize;
    mqHeader->total_message_no--;

    return message->messageSize;
}
