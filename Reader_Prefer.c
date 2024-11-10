
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_READERS 10

sem_t writer_queue;       // Queue for readers
sem_t reader_queue;       // Queue for writers
pthread_mutex_t read_count_lock; // Mutex to protect writer_count, tránh tình trạng race condition của các thread
pthread_mutex_t writer_lock; // Mutex to ensure only one reader at a time
pthread_mutex_t read_timer_lock;
int reader_count = 0;     // Current number of writers
int read_count_timer = MAX_READERS; // Counter to limit writers

void *reader(void *arg) {
    int id = *(int *)arg;
    /*
    Entry section
    */
    // Wait in reader queue for access
    sem_wait(&reader_queue);

    /*
    Critical section
    */
    pthread_mutex_lock(&read_count_lock);
    pthread_mutex_lock(&read_timer_lock);
    reader_count++;
    read_count_timer--;

    // If this is the first writer, block readers
    if (reader_count == 1) {
        sem_wait(&writer_queue);
    }
    /*
    Exit the critial
    */
    printf("Reader %d accessed the resource. (Total readers: %d, Timer: %d)\n", id, reader_count, read_count_timer);
    pthread_mutex_unlock(&read_count_lock);
    pthread_mutex_unlock(&read_timer_lock);
    // Simulate reading
    sleep(1);

    /*
    Critial section
    */
    pthread_mutex_lock(&read_count_lock);
    reader_count--;

    // If no readers are left, allow writers to access
    if (reader_count == 0) {
        sem_post(&writer_queue);
    }
    pthread_mutex_unlock(&read_count_lock);

    /*
    Control Limit
    */
    // Reset timer and allow reader access if the writer limit is reached
    if (read_count_timer == 0) {
        pthread_mutex_lock(&read_timer_lock);
        read_count_timer = MAX_READERS; // Reset the timer
        sem_post(&writer_queue);         // Allow readers to access
        sleep(1);
        pthread_mutex_unlock(&read_timer_lock);
    }
    /*
    Exit critial section
    */
    pthread_mutex_unlock(&writer_lock);
    // Allow the next reader in the queue
    sem_post(&reader_queue);

    return NULL;
}

void *writer(void *arg) {
    int id = *(int *)arg;
    /*
    Entry section
    */
    // Wait in the writer queue for access
    sem_wait(&writer_queue);
    
    /*
    Critial section
    */
    // Only one writer can access at a time
    pthread_mutex_lock(&writer_lock);
    printf("Writer %d accessed the resource.\n", id);
    sleep(10); // Simulate writing
    pthread_mutex_unlock(&writer_lock);

    /*
    Exit critial
    */
    // Allow the next writer in the queue if no reader need resource
    sem_post(&writer_queue);

    return NULL;
}

int main() {
    int num_readers, num_writers;

    printf("Enter the number of readers: ");
    scanf("%d", &num_readers);

    printf("Enter the number of writers: ");
    scanf("%d", &num_writers);

    pthread_t readers[num_readers], writers[num_writers];
    int reader_ids[num_readers], writer_ids[num_writers];

    // Initialize semaphores and mutexes
    sem_init(&reader_queue, 0, MAX_READERS);      
    sem_init(&writer_queue, 0, 1);
    pthread_mutex_init(&read_count_lock, NULL);
    pthread_mutex_init(&writer_lock, NULL);
    pthread_mutex_init(&read_timer_lock, NULL);

    // Create reader threads
    for (int i = 0; i < num_readers; i++) {
        reader_ids[i] = i + 1;
        pthread_create(&readers[i], NULL, reader, &reader_ids[i]);
    }

        // Create writer threads
    for (int i = 0; i < num_writers; i++) {
        writer_ids[i] = i + 1;
        pthread_create(&writers[i], NULL, writer, &writer_ids[i]);
    }

    // Wait for all reader threads to complete
    for (int i = 0; i < num_readers; i++) {
        pthread_join(readers[i], NULL);
    }

    
    // Wait for all writer threads to complete
    for (int i = 0; i < num_writers; i++) {
        pthread_join(writers[i], NULL);
    }

    // Destroy semaphores and mutexes
    sem_destroy(&writer_queue);
    sem_destroy(&reader_queue);
    pthread_mutex_destroy(&read_count_lock);
    pthread_mutex_destroy(&writer_lock);
    pthread_mutex_destroy(&read_timer_lock);

    return 0;
}
