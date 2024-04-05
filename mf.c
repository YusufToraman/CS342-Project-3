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

size_t find_free_space_for_queue(FixedPortion* fixedPortion, size_t requestedSize) {
    size_t prevOffset = 0;
    size_t currentOffset = fixedPortion->holeManager.firstHoleOffset;
    
    while (currentOffset != 0) {
        Hole* hole = (Hole*)((void*)shmem + currentOffset);
        
        if (hole->size >= requestedSize) {
            // Check if the hole is large enough to be split
            // hole = 102 queue 100 size of hole 5
            if (hole->size > requestedSize + sizeof(Hole)) {
                size_t newHoleOffset = currentOffset + sizeof(Hole) + requestedSize;
                Hole* newHole = (Hole*)((char*)shmem + newHoleOffset);
                newHole->start = hole->start + requestedSize;
                newHole->size = hole->size - requestedSize - sizeof(Hole);
                newHole->next = hole->next;
                
                hole->size = requestedSize; // Adjust the current hole size
                
                if (prevOffset == 0) {
                    // This was the first hole, update the manager
                    fixedPortion->holeManager.firstHoleOffset = newHoleOffset;
                } else {
                    // Link the previous hole to the new hole
                    Hole* prevHole = (Hole*)((char*)shmem + prevOffset);
                    prevHole->next = newHoleOffset;
                }
                return hole->start; // Return the start of the allocated space
            } else {
                // Use the hole as-is, remove it from the list
                if (prevOffset == 0) {
                    fixedPortion->holeManager.firstHoleOffset = hole->next;
                } else {
                    Hole* prevHole = (Hole*)((char*)shmem + prevOffset);
                    prevHole->next = hole->next;
                }
                return hole->start;
            }
        }
        
        prevOffset = currentOffset;
        currentOffset = hole->next;
    }
    
    return (size_t)-1; // Failed to find a suitable space
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
    if (!shmem) {
        return MF_ERROR;
    }
    
    FixedPortion* fixedPortion = (FixedPortion*)shmem;
    fixedPortion->config = config; 
    fixedPortion->mq_count = 0; 
    fixedPortion->unique_id = 1;  
    initialize_hole_manager(&fixedPortion, config.shmem_size);
    
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
    FixedPortion* fixedPortion = (FixedPortion*)shmem;
    int shm_fd;
    shm_fd = shm_open(fixedPortion->config.shmem_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Error accessing shared memory");
        return MF_ERROR;
    }

    // mmap ile paylaşılan belleği süreç adres alanına eşle
    shmem = mmap(NULL, fixedPortion->config.shmem_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
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

    FixedPortion* fixedPortion = (FixedPortion*)shmem;
    size_t requiredSize = sizeof(MessageQueueHeader) + mqsize;
    
    size_t offset = find_free_space_for_queue(fixedPortion->config.shmem_size, requiredSize);
    if (offset == (size_t)-1) {
        return MF_ERROR;
    }
    
    MessageQueueHeader* mqHeader = (MessageQueueHeader*)((char*)shmem + offset);
    
    strncpy(mqHeader->mq_name, mqname, MAX_MQNAMESIZE);
    mqHeader->mq_name[MAX_MQNAMESIZE - 1] = '\0'; 
    //bu isim kopyalama böyle oluyomus 

    mqHeader->start_pos_of_queue = offset + sizeof(MessageQueueHeader); // Headerin arkasında queue olarak kullanacağımız yer
    mqHeader->end_pos_of_queue = mqHeader->start_pos_of_queue + mqsize; // bu da başlangıç + data size yani mqsize
    mqHeader->mq_data_size = mqsize; 
    mqHeader->in = mqHeader->start_pos_of_queue;
    mqHeader->out = mqHeader->start_pos_of_queue; 
    mqHeader->max_messages_allowed = fixedPortion->config.max_msgs_in_queue; 
    mqHeader->total_message_no = 0;
    mqHeader->qid = fixedPortion->unique_id++;

    char semName[64]; // Adjust size as necessary
    sprintf(semName, "/mq_%d_sem", mqHeader->qid); // Unique semaphore name based on the queue ID

    // Create and initialize the named semaphore
    mqHeader->sem = sem_open(semName, O_CREAT | O_EXCL, 0644, 1); // Initial value of 1
    if (mqHeader->sem == SEM_FAILED) {
        perror("Failed to open semaphore");
        // Handle semaphore creation failure, possibly by cleaning up already allocated resources
        return MF_ERROR;
    }

    fixedPortion->mq_count++;
    
    return mqHeader->qid;
}


int mf_remove(char *mqname)
{
    return (0);
}


int mf_open(char *mqname)
{
    return (0);
}

int mf_close(int qid)
{
    return(0);
}


int mf_send (int qid, void *bufptr, int datalen)
{
    printf ("mf_send called\n");
    return (0);
}

int mf_recv (int qid, void *bufptr, int bufsize)
{
    printf ("mf_recv called\n");
    return (0);
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
