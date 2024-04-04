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
    char shmem_name[MAXFILENAME];
    size_t shmem_size;
    int max_msgs_in_queue;
    int max_queues_in_shmem;
} ConfigParams;

typedef struct {
    int mq_count;
    ConfigParams config;
    //hole_manager
} FixedPortion;

void *shmem = NULL;

static ConfigParams read_config(const char* filename);
static void* create_shared_memory(const char* name, size_t size);

int mf_init() {
    ConfigParams config = read_config(CONFIG_FILENAME);
    
    shmem = create_shared_memory(config.shmem_name, config.shmem_size);
    if (!shmem) {
        return MF_ERROR;
    }
    
    FixedPortion* fixedPortion = (FixedPortion*)shmem;
    fixedPortion->config = config; 
    fixedPortion->mq_count = 0;   
    
    // If using a hole manager or similar, initialize it here
    // initialize_hole_manager(&fixedPortion->hole_manager); // Adjust based on your implementation
    
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

int mf_create(char *mqname, int mqsize)
{
    return (0);
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



