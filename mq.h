#ifndef _MQ_H_
#define _MQ_H_

typedef struct {
    size_t len;                     // Mesajın boyutu.
    char data[MAX_MESSAGE_SIZE];    // Mesajın içeriği.
} Message;

typedef struct {
    Message* messages;  // Mesajlar için pointer dizisi.
    size_t head;        // Kuyruğun başı.
    size_t tail;        // Kuyruğun sonu.
    size_t capacity;    // Kuyruk kapasitesi (mesaj sayısı olarak).
    size_t count;       // Kuyruktaki mevcut mesaj sayısı.
} MessageQueue;

// Mesaj kuyruğu oluşturur ve başlatır.
MessageQueue* create_message_queue(size_t capacity) {
    MessageQueue* queue = (MessageQueue*)malloc(sizeof(MessageQueue));
    queue->messages = (Message*)malloc(sizeof(Message) * capacity);
    queue->head = 0;
    queue->tail = 0;
    queue->capacity = capacity;
    queue->count = 0;
    return queue;
}

// Kuyruğa yeni bir mesaj ekler.
int enqueue_message(MessageQueue *queue, const char *data, size_t len) {
    if (queue->count >= queue->capacity) {
        return -1; // Kuyruk dolu.
    }

    // Yeni mesaj için yer hazırla.
    size_t index = (queue->head + queue->count) % queue->capacity;
    queue->messages[index].len = len;
    memcpy(queue->messages[index].data, data, len);
    
    // Kuyruk bilgilerini güncelle.
    queue->count++;
    
    return 0; // Mesaj başarıyla eklendi.
}

// Kuyruktan bir mesaj alır.
int dequeue_message(MessageQueue *queue, char *data, size_t *len) {
    if (queue->count == 0) {
        return -1; // Kuyruk boş.
    }

    // Kuyruktan mesajı kopyala.
    *len = queue->messages[queue->head].len;
    memcpy(data, queue->messages[queue->head].data, *len);
    
    // Kuyruk bilgilerini güncelle.
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;

    return 0; // Mesaj başarıyla alındı.
}

// Kuyruk belleğini serbest bırakır.
void destroy_message_queue(MessageQueue* queue) {
    free(queue->messages);
    free(queue);
}

#endif
