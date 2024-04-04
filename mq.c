#include "mq.h"

// Mesaj kuyruğu oluşturur ve başlatır.
MessageQueue* create_message_queue(size_t capacity) {
    MessageQueue* queue = (MessageQueue*)malloc(sizeof(MessageQueue));
    queue->qid = qid++;
    queue->messages = (Message*)malloc(sizeof(Message) * capacity);
    queue->head = 0;
    queue->tail = 0; //BU KULLANILMAMIŞ
    queue->capacity = capacity;
    queue->count = 0;
    return queue;
}

// Kuyruğa yeni bir mesaj ekler.
int enqueue_message(MessageQueue *queue, Message message) {
    if (queue->count >= queue->capacity) {
        return -1; // Kuyruk dolu.
    }

    // Yeni mesaj için yer hazırla.
    size_t index = (queue->head + queue->count) % queue->capacity;
    queue->messages[index] = message;  // Yapıyı kopyala.

    // Kuyruk bilgilerini güncelle.
    queue->count++;

    return 0;
}

// Kuyruktan bir mesaj alır.
Message* dequeue_message(MessageQueue *queue) {
    if (queue->count == 0) {
        return NULL; // Kuyruk boş.
    }

    // Kuyruktan mesajı al.
    Message* message = &queue->messages[queue->head];

    // Kuyruk bilgilerini güncelle.
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;

    return message; // Mesaj başarıyla alındı.
}

// Kuyruk belleğini serbest bırakır.
void destroy_message_queue(MessageQueue* queue) {
    free(queue->messages);
    free(queue);
}