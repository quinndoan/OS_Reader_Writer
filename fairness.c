#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

int readcount = 0;                // number of readers currently accessing resource
sem_t resource;                    // controls access (read/write) to the resource
sem_t rmutex;                      // for syncing changes to shared variable readcount
sem_t serviceQueue;                // FAIRNESS: preserves ordering of requests (FIFO)
pthread_t readers[5], writers[5];

// Reader function
void *reader_fairness(void *arg) {
//    printf("Reader %ld is in queue\n", (long)arg);
    // ENTRY Section
    sem_wait(&serviceQueue);       // wait in line to be serviced
    sem_wait(&rmutex);             // request exclusive access to readcount
    readcount++;                   // update count of active readers
    if (readcount == 1) {          // if I am the first reader
        sem_wait(&resource);       // request resource access for readers (writers blocked)
    }
    sem_post(&serviceQueue);       // let next in line be serviced
    sem_post(&rmutex);             // release access to readcount

    // CRITICAL Section
    printf("Reader %ld is reading\n", (long)arg);

    // EXIT Section
    sem_wait(&rmutex);             // request exclusive access to readcount
    readcount--;                   // update count of active readers
    if (readcount == 0) {          // if there are no readers left
        sem_post(&resource);       // release resource access for all
    }
    sem_post(&rmutex);             // release access to readcount

    return NULL;
}

// Writer function
void *writer_fairness(void *arg) {
//    printf("Writer %ld is in queue\n", (long)arg);
    // ENTRY Section
    sem_wait(&serviceQueue);       // wait in line to be serviced
    sem_wait(&resource);           // request exclusive access to resource
    sem_post(&serviceQueue);       // let next in line be serviced

    // CRITICAL Section
    printf("Writer %ld is writing\n", (long)arg);
    // EXIT Section
    sem_post(&resource);           // release resource access for next reader/writer

//    printf("Writer %ld finishes writing\n", (long)arg);
    return NULL;
}

void fairness()
{
    for (long i = 0; i < 5; i++) {
        pthread_create(&readers[i], NULL, reader_fairness, (void *)i);
        pthread_create(&writers[i], NULL, writer_fairness, (void *)i);
    }

    // Join reader and writer threads
    for (int i = 0; i < 5; i++) {
        pthread_join(readers[i], NULL);
        pthread_join(writers[i], NULL);
    }
}

int main() {
    // Initialize semaphores
    sem_init(&resource, 0, 1);     // Binary semaphore for resource
    sem_init(&rmutex, 0, 1);       // Binary semaphore for readcount access
    sem_init(&serviceQueue, 0, 1); // Binary semaphore for service queue fairness

//    client[] = {"reader 1","writer 8","reader 5", "reader 6",....}
    fairness();

    // Create reader and writer threads


    // Destroy semaphores
    sem_destroy(&resource);
    sem_destroy(&rmutex);
    sem_destroy(&serviceQueue);

    return 0;
}
