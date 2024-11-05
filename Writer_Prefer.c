#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_WRITERS 10

sem_t writer_queue;       // Queue for readers
sem_t reader_queue;       // Queue for writers
pthread_mutex_t write_count_lock; // Mutex to protect writer_count
pthread_mutex_t reader_lock; // Mutex to ensure only one reader at a time
int writer_count = 0;     // Current number of writers
int write_count_timer = MAX_WRITERS; // Counter to limit writers

void *writer(void *arg) {
    int id = *(int *)arg;
    
    // Wait in writer queue for access
    sem_wait(&writer_queue);

    pthread_mutex_lock(&write_count_lock);
    writer_count++;
    write_count_timer--;

    // If this is the first writer, block readers
    if (writer_count == 1) {
        sem_wait(&reader_queue);
    }
    
    printf("Writer %d accessed the resource. (Total writers: %d, Timer: %d)\n", id, writer_count, write_count_timer);
    pthread_mutex_unlock(&write_count_lock);

    // Simulate writing
    sleep(1);

    pthread_mutex_lock(&write_count_lock);
    writer_count--;

    // If no writers are left, allow readers to access
    if (writer_count == 0) {
        sem_post(&reader_queue);
    }
    pthread_mutex_unlock(&write_count_lock);

    // Reset timer and allow reader access if the writer limit is reached
    if (write_count_timer == 0) {
        write_count_timer = MAX_WRITERS; // Reset the timer
        sem_post(&reader_queue);         // Allow readers to access
        sleep(1);
        write_count_timer = MAX_WRITERS; 
    }
     pthread_mutex_unlock(&reader_lock);
    // Allow the next writer in the queue
    sem_post(&writer_queue);

    return NULL;
}

void *reader(void *arg) {
    int id = *(int *)arg;

    // Wait in the reader queue for access
    sem_wait(&reader_queue);
    
    // Only one reader can access at a time
    pthread_mutex_lock(&reader_lock);
    printf("Reader %d accessed the resource.\n", id);
    sleep(1); // Simulate reading
    pthread_mutex_unlock(&reader_lock);

    // Allow the next reader in the queue
    sem_post(&reader_queue);

    return NULL;
}

int main() {
    int num_readers, num_writers;

    printf("Enter the number of writers: ");
    scanf("%d", &num_writers);

    printf("Enter the number of readers: ");
    scanf("%d", &num_readers);

    pthread_t readers[num_readers], writers[num_writers];
    int reader_ids[num_readers], writer_ids[num_writers];

    // Initialize semaphores and mutexes
    sem_init(&writer_queue, 0, MAX_WRITERS);       // Only one reader can request at a time
    sem_init(&reader_queue, 0, 1);       // Only one writer allowed at a time
    pthread_mutex_init(&write_count_lock, NULL);
    pthread_mutex_init(&reader_lock, NULL);

    // Create writer threads
    for (int i = 0; i < num_writers; i++) {
        writer_ids[i] = i + 1;
        pthread_create(&writers[i], NULL, writer, &writer_ids[i]);
    }
    // Create reader threads
    for (int i = 0; i < num_readers; i++) {
        reader_ids[i] = i + 1;
        pthread_create(&readers[i], NULL, reader, &reader_ids[i]);
    }

    // Wait for all writer threads to complete
    for (int i = 0; i < num_writers; i++) {
        pthread_join(writers[i], NULL);
    }

    // Wait for all reader threads to complete
    for (int i = 0; i < num_readers; i++) {
        pthread_join(readers[i], NULL);
    }

    // Destroy semaphores and mutexes
    sem_destroy(&writer_queue);
    sem_destroy(&reader_queue);
    pthread_mutex_destroy(&write_count_lock);
    pthread_mutex_destroy(&reader_lock);

    return 0;
}
