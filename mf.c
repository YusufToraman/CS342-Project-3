#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>
#include <string.h>
#include "mf.h"
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <semaphore.h>

#define MF_SUCCESS 0 
#define MF_ERROR -1

typedef struct {
    int mq_count;
    //mq_array
} FixedPortion;

typedef struct {
    char shmem_name[MAXFILENAME];
    size_t shmem_size;
    int max_msgs_in_queue;
    int max_queues_in_shmem;
} ConfigParams;


//Global de yapabiliriz bence sorun yok ikisi de okey
void *shmem = NULL;
ConfigParams config;

static ConfigParams read_config(const char* filename);
static void* create_shared_memory(const char* name, size_t size);

int mf_init() {
    config = read_config(CONFIG_FILENAME);
    shmem = create_shared_memory(config.shmem_name, config.shmem_size);
    if (!shmem) {
        return MF_ERROR;
    }
    
    //BU SHAREDMEMORYMETA KISMINA BİRLİKTE BAKALIM
    SharedMemoryMeta* meta = (SharedMemoryMeta*)shmem;
    meta->mq_count = 0;

    // Senkronizasyon objelerini kur
    if (setup_synchronization_objects(meta, config) != 0) {
        munmap(shmem, config.shmem_size);
        return MF_ERROR;
    }

    return MF_SUCCESS;
}

int mf_destroy() {
    if (shmem != NULL) {
        munmap(shmem, config.shmem_size);
        shmem = NULL;
    }
    
    if (config.shmem_name[0] != '\0') {
        shm_unlink(config.shmem_name);
        config.shmem_name[0] = '\0';  // İsmi temizle
    }

    // Semaforları kapat ve sil
    for (int i = 0; i < config.max_queues_in_shmem; i++) {
        char sem_name[256];
        sprintf(sem_name, "/sem_queue_%d", i);
        sem_unlink(sem_name);
    }
    
    return MF_SUCCESS;
}

int mf_connect()
{
    return (0);
}
   
int mf_disconnect()
{
    return (0);
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

//PRINT İLE DOĞRU OKUYOR MU DİYE TEST ET !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
static ConfigParams read_config(const char* filename) {
    FILE* file = fopen(filename, "r");

    strcpy(config.shmem_name, "/default_shmem");
    config.shmem_size = MIN_SHMEMSIZE;
    config.max_msgs_in_queue = 10;
    config.max_queues_in_shmem = 5;
    
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
    printf("name: %s", name);
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



