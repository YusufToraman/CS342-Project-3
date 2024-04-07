#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "mf.h"
#include "mq.h"
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

#define MF_SUCCESS 0 
#define MF_ERROR -1

typedef struct {
    size_t start; // Offset to the start of the hole
    size_t size;  // Size of the hole
    size_t next;  // Offset to the next hole, 0 if this is the last
} Hole;

typedef struct {
    size_t firstHoleOffset; // Offset of the first hole in the list, 0 if no holes
} HoleManager;

typedef struct {
    char shmem_name[MAXFILENAME];
    size_t shmem_size;
    int max_msgs_in_queue;
    int max_queues_in_shmem;
} ConfigParams;

typedef struct {
    int mq_count;
    int unique_id;
    ConfigParams config;
    HoleManager holeManager;
} FixedPortion;

void *shmem = NULL;

static ConfigParams read_config(const char* filename);
static void* create_shared_memory(const char* name, size_t size);
MessageQueueHeader* find_mq_header_by_qid(int qid, void* shmem_base);
MessageQueueHeader* find_mq_header_by_name(const char* mqname);
size_t find_free_space_for_queue(FixedPortion* fixedPortion, size_t requestedSize);

size_t find_free_space_for_queue(FixedPortion* fixedPortion, size_t requestedSize) {
    size_t prevOffset = 0;
    size_t currentOffset = fixedPortion->holeManager.firstHoleOffset;
    
    while (currentOffset != 0) {
        Hole* hole = (Hole*)((char*)shmem + currentOffset);
        
        if (hole->size >= requestedSize) { 
            size_t allocatedSpaceStart = currentOffset; 
          
            if (hole->size > requestedSize) {
                size_t newHoleOffset = currentOffset + requestedSize;
                Hole* newHole = (Hole*)((char*)shmem + newHoleOffset);
                
                newHole->start = newHoleOffset;
                newHole->size = hole->size - requestedSize;
                newHole->next = hole->next;
                
                if (prevOffset == 0) {
                    fixedPortion->holeManager.firstHoleOffset = newHoleOffset;
                } else {
                    Hole* prevHole = (Hole*)((char*)shmem + prevOffset);
                    prevHole->next = newHoleOffset;
                }
                
            } else {
                // Eğer tam uyuyorsa, hole'u listeden çıkar
                if (prevOffset == 0) {
                    fixedPortion->holeManager.firstHoleOffset = hole->next;
                } else {
                    Hole* prevHole = (Hole*)((char*)shmem + prevOffset);
                    prevHole->next = hole->next;
                }
            }
            return allocatedSpaceStart; // Yeni MQ için yerin başlangıcı
        }
        
        prevOffset = currentOffset;
        currentOffset = hole->next;
    }
    
    return (size_t)-1; // Uygun bir yer bulunamadı
}

void initialize_hole_manager(FixedPortion* fixedPortion, size_t totalShmemSize) {
    size_t startOfFreeSpace = sizeof(FixedPortion);
    size_t firstHoleOffset = startOfFreeSpace;

    Hole* firstHole = (Hole*)((void*)shmem + firstHoleOffset);

    firstHole->start = firstHoleOffset; // Start of the hole in offset terms
    firstHole->size = totalShmemSize - startOfFreeSpace; // Size of the hole
    firstHole->next = 0; // Indicate that this is the only hole for now

    fixedPortion->holeManager.firstHoleOffset = firstHoleOffset;
}


int mf_init() {
    ConfigParams config = read_config(CONFIG_FILENAME);
    shmem = create_shared_memory(config.shmem_name, config.shmem_size);
    printf("LOG2: %p", (void*)shmem);
    fflush(stdout);
    if (!shmem) {
        return MF_ERROR;
    }
    
    FixedPortion* fixedPortion = (FixedPortion*)shmem;
    fixedPortion->config = config;
    fixedPortion->mq_count = 0; 
    fixedPortion->unique_id = 1;  
    initialize_hole_manager(fixedPortion, config.shmem_size);
    return MF_SUCCESS;
}


int mf_destroy() {
    if (shmem != NULL) {
        FixedPortion* fixedPortion = (FixedPortion*)shmem;
        
        char shmemName[MAXFILENAME];
        strncpy(shmemName, fixedPortion->config.shmem_name, MAXFILENAME);
        
        munmap(shmem, fixedPortion->config.shmem_size);
        shmem = NULL;
        
        if (shmemName[0] != '\0') {
            shm_unlink(shmemName);
        }
    }
    return MF_SUCCESS;
}


int mf_connect() {
    ConfigParams config = read_config(CONFIG_FILENAME);
    int shm_fd;
    shm_fd = shm_open(config.shmem_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Error accessing shared memory");
        return MF_ERROR;
    }

    // mmap ile paylaşılan belleği süreç adres alanına eşle
    shmem = mmap(NULL, config.shmem_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shmem == MAP_FAILED) {
        perror("Error mapping shared memory");
        close(shm_fd);
        return MF_ERROR;
    }
    close(shm_fd);
    return MF_SUCCESS;
}
   
int mf_disconnect() {
    FixedPortion* fixedPortion = (FixedPortion*)shmem;

    if (shmem != NULL) {
        if (munmap(shmem, fixedPortion->config.shmem_size) == -1) {
            perror("Error unmapping shared memory");
            return MF_ERROR;
        }
        shmem = NULL;
    }

    return MF_SUCCESS;
}

int mf_create(char *mqname, int mqsize) {
    mqsize  = mqsize *1024;

    FixedPortion* fixedPortion = (FixedPortion*)shmem;
    size_t requiredSize = sizeof(MessageQueueHeader) + mqsize;
    size_t offset = find_free_space_for_queue(fixedPortion, requiredSize);

    if (offset == (size_t)-1) {
        return MF_ERROR;
    }
    
    MessageQueueHeader* mqHeader = (MessageQueueHeader*)((char*)shmem + offset);
    strncpy(mqHeader->mq_name, mqname, MAX_MQNAMESIZE);
    mqHeader->mq_name[MAX_MQNAMESIZE - 1] = '\0'; 
    mqHeader->start_pos_of_queue = offset + sizeof(MessageQueueHeader); // Headerin arkasında queue olarak kullanacağımız yer
    mqHeader->end_pos_of_queue = mqHeader->start_pos_of_queue + mqsize; // bu da başlangıç + data size yani mqsize
    mqHeader->mq_data_size = mqsize; 
    mqHeader->in = mqHeader->start_pos_of_queue;
    mqHeader->out = mqHeader->start_pos_of_queue; 
    mqHeader->circularity = mqHeader->end_pos_of_queue;
    mqHeader->max_messages_allowed = fixedPortion->config.max_msgs_in_queue; 
    mqHeader->total_message_no = 0;
    mqHeader->qid = fixedPortion->unique_id++;

    for (int i = 0; i < 2; i++) {
        mqHeader->processes[i] = -1; //initalize empty processes.
    }

    if (sem_init(&mqHeader->QueueSem, 1, 1) != 0) {
        perror("Failed to initialize QueueSem semaphore");
        exit(EXIT_FAILURE);
    }
    
    if (sem_init(&mqHeader->SpaceSem, 1, 0) != 0) {
        perror("Failed to initialize SpaceSem semaphore");
        exit(EXIT_FAILURE);
    }

    if (sem_init(&mqHeader->ZeroSem, 1, 0) != 0) {
        perror("Failed to initialize SpaceSem semaphore");
        exit(EXIT_FAILURE);
    }

    fixedPortion->mq_count++; 
    return mqHeader->qid;
    }


int mf_remove(char *mqname) {
    FixedPortion* fixedPortion = (FixedPortion*)shmem;
    MessageQueueHeader* mqHeader = find_mq_header_by_name(mqname);
    if (mqHeader == NULL) {
        fprintf(stderr, "Message queue not found: %s\n", mqname);
        return -1; // Queue not found
    }
    
    // BU GEREKLİ Mİİ

    /*  if (mqHeader->active_count > 0) {
        fprintf(stderr, "Message queue is still in use: %s\n", mqname);
        return -1; // Queue is still in use
    } */
    
    // BU METHOD YAZILMASI LAZIM YANİ BUNUN YERİNİ HOLE LİST E GERİ KOYCAZ.
    //mark_space_as_available(mqHeader);

    // BURASI memorynin o kısmını 0 lıyor. Mantıklı ama bunu üstteki için yazacağımız fonskiyonun 
    //içinde de yapabiliriz. Yani hole olarak eklerken sıfırlayıp ekler.
    memset(mqHeader, 0, sizeof(MessageQueueHeader) + mqHeader->mq_data_size);

    fixedPortion->mq_count--;

    return MF_SUCCESS;
}



int mf_open(char* mqname) {
    MessageQueueHeader* mqHeader = find_mq_header_by_name(mqname);
    if (mqHeader == NULL) {
        fprintf(stderr, "Message queue not found: %s\n", mqname);
        return -1;
    }
    
    pid_t pid = getpid();
    printf("LOG2: %d", pid);
    fflush(stdout);
    // Check if this PID is already in the list of active processes
    for (int i = 0; i < 2; i++) {
        if (mqHeader->processes[i] == pid) {
            return mqHeader->qid;
        }
    }

    for (int i = 0; i < 2; i++) {
        if (mqHeader->processes[i] == -1) {
            mqHeader->processes[i] = pid;
            return mqHeader->qid;
        }
    }
    fprintf(stderr, "Cannot connect process to message queue : %s, It already have sender and receiver.\n", mqname);
    return -1;
}

int mf_close(int qid) {
    MessageQueueHeader* mqHeader = find_mq_header_by_qid(qid, shmem);
    if (!mqHeader) {
        fprintf(stderr, "Message queue not found for QID: %d\n", qid);
        return -1;
    }
    
    pid_t pid = getpid();
    for (int i = 0; i < 2; i++) {
        if (mqHeader->processes[i] == pid) {
            // Found the PID, remove it from the list of active processes
            mqHeader->processes[i] = -1;
            return MF_SUCCESS; 
        }
    }
    
    fprintf(stderr, "Process (PID: %d) did not have MQ open.\n", pid);
    return -1; // PID was not found in the list of active processes
}


int mf_send(int qid, void *bufptr, int datalen) {
    if (datalen > MAX_DATALEN) {
        perror("Message is too big.");
        return -1;
    }
    if (datalen < MIN_DATALEN) {
        perror("Message is too small.");
        return -1;
    }

    MessageQueueHeader* mqHeader = find_mq_header_by_qid(qid, shmem);
    // MQHeader da yer alan processlerden birisi mi? check

    if (!mqHeader) return MF_ERROR;

    // Wait for exclusive access
    if (sem_wait(&mqHeader->QueueSem) == -1) {
        perror("Error waiting for QueueSem");
        return MF_ERROR;
    }

    // Check if there's enough space for the new message
    //printf("\nActual space: %ld \n", mqHeader->end_pos_of_queue - mqHeader->start_pos_of_queue);
    size_t requiredSpace = sizeof(Message) + datalen;
    //printf("\nRequired space: %ld \n", requiredSpace);
    size_t availableSpace = calculate_available_space(mqHeader, requiredSpace);
    //printf("\nAvailable space: %ld \n", availableSpace);

    // If not enough space, wait on SpaceSem and check again
    while ((int)requiredSpace > (int)availableSpace) {
        printf("\033[43mBEKLIYOR MUUYUMMM\033[0m \n");
        fflush(stdout);
        sem_post(&mqHeader->QueueSem); // Release exclusive access
        sem_wait(&mqHeader->SpaceSem); // Wait for space to be available should wait first all the time

        // Re-acquire exclusive access to check space again
        if (sem_wait(&mqHeader->QueueSem) == -1) {
            perror("Error re-waiting for QueueSem");
            return MF_ERROR;
        }
        availableSpace = calculate_available_space(mqHeader, requiredSpace);
    }

    // At this point, there's guaranteed enough space. Enqueue the message.
    enqueue_message(mqHeader, bufptr, datalen, shmem);

    printf("Enqueued: %d \n", datalen);

    mqHeader->total_message_no++;

    sem_post(&mqHeader->ZeroSem);
    sem_post(&mqHeader->QueueSem);

    return MF_SUCCESS;
}


int mf_recv(int qid, void *bufptr, int bufsize) {
    if (bufsize < MIN_DATALEN) {
        perror("Buffer size too small");
        return -1;
    }
    if (bufsize > MAX_DATALEN) {
        perror("Message is too big.");
        return -1;
    }

    MessageQueueHeader* mqHeader = find_mq_header_by_qid(qid, shmem); 
        if (!mqHeader) {
        perror("Queue not found");
        return -1;
    }

    // Wait for exclusive access to the queue
    if (sem_wait(&mqHeader->QueueSem) == -1) {
        perror("Error waiting for QueueSem");
        return -1;
    }

    // Check if the queue has messages
    while (mqHeader->total_message_no <= 0) {
        // The queue is empty, release the QueueSem and wait for messages to become available
        sem_post(&mqHeader->QueueSem);
        sem_wait(&mqHeader->ZeroSem); 
        
        if (sem_wait(&mqHeader->QueueSem) == -1) {
            perror("Error re-waiting for QueueSem");
            return -1;
        }
    }

    int msgSize = dequeue_message(mqHeader, bufptr, bufsize, shmem);
        if (msgSize == -1) {
        sem_post(&mqHeader->QueueSem);
        return -1;
    }

    mqHeader->total_message_no--;

    sem_post(&mqHeader->QueueSem);
    sem_post(&mqHeader->SpaceSem);

    return msgSize;
}

int mf_print()
{
    return (0);
}


static ConfigParams read_config(const char* filename) {
    ConfigParams config;
    FILE* file = fopen(filename, "r");
    
    if (!file) {
        perror("Unable to open the config file");
        return config;
    }

    char line[256], temp_shmem_name[256];;
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#') continue;
        if (sscanf(line, "SHMEM_NAME \"%255[^\"]\"", temp_shmem_name) == 1) {
            strcpy(config.shmem_name, temp_shmem_name);
            continue;
        }
        if (sscanf(line, "SHMEM_SIZE %zu", &config.shmem_size) == 1) continue;
        if (sscanf(line, "MAX_MSGS_IN_QUEUE %d", &config.max_msgs_in_queue) == 1) continue;
        if (sscanf(line, "MAX_QUEUES_IN_SHMEM %d", &config.max_queues_in_shmem) == 1) continue;
    }

    printf("SHMEM_NAME: %s\nSHMEM_SIZE: %zu\nMAX_MSGS_IN_QUEUE: %d\nMAX_QUEUES_IN_SHMEM: %d", 
    config.shmem_name, config.shmem_size, config.max_msgs_in_queue, config.max_queues_in_shmem);
    fclose(file);
    config.shmem_size *= 1024;
    return config;
}

static void* create_shared_memory(const char* name, size_t size) {
    int shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return NULL;
    }
    if (ftruncate(shm_fd, size) == -1) {
        perror("ftruncate");
        close(shm_fd);
        return NULL;
    }
    void* addr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        return NULL;
    }
    return addr;
}

//SHMEM ZATEN GLOBAL, PARAMETRE VERMEYEBEİLİRZ. TEST EDERKEN İŞİMİZ KOLAYLAŞIR KAlSIN DEDİM
MessageQueueHeader* find_mq_header_by_qid(int qid, void* shmem_base) {
    // Başlangıçta fixed portion'un boyutunu atlıyoruz
    char* current_position = (char*)shmem_base + sizeof(FixedPortion);
    char* end_of_shmem = (char*)shmem_base + ((FixedPortion*)shmem_base)->config.shmem_size;

    while (current_position < end_of_shmem) {
        MessageQueueHeader* mqHeader = (MessageQueueHeader*)current_position;

        if (mqHeader->qid == qid) {
            return mqHeader;
        }
        // Geçerli queue'un bitişinden sonra yeni bir queue başlayabilir
        // mqHeader->end_pos_of_queue + 1 ile sonraki potansiyel başlangıç noktasına geç
        current_position = (char*)shmem_base + mqHeader->end_pos_of_queue + 1; //+1 neden???
    }
    return NULL;
}

MessageQueueHeader* find_mq_header_by_name(const char* mqname) {
    char* current_position = (void*)shmem + sizeof(FixedPortion);
    char* end_of_shmem = (void*)shmem + ((FixedPortion*)shmem)->config.shmem_size;

    while (current_position < end_of_shmem) {
        MessageQueueHeader* mqHeader = (MessageQueueHeader*)current_position;

        if (strncmp(mqHeader->mq_name, mqname, MAX_MQNAMESIZE) == 0) {
            return mqHeader; 
        }
        current_position += sizeof(MessageQueueHeader) + mqHeader->mq_data_size;
    }
    return NULL;
}