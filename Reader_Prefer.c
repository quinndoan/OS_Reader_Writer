#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX_READERS 10

sem_t reader_queue;   // Hàng đợi cho reader
sem_t writer_queue;   // Hàng đợi cho writer
pthread_mutex_t read_count_lock; // Mutex để bảo vệ biến reader_count
pthread_mutex_t writer_lock; // Mutex để đảm bảo chỉ có một writer vào tại một thời điểm
int reader_count = 0; // Số lượng reader hiện tại
int read_count_timer = MAX_READERS; // Biến timer để đếm số lượng reader vào

void *reader(void *arg) {
    int id = *(int *)arg;
    
    // Đợi trong hàng đợi reader
    sem_wait(&reader_queue);
    
    // Khóa để thay đổi reader_count và read_count_timer
    pthread_mutex_lock(&read_count_lock);
    reader_count++;
    read_count_timer--;
    
    // Nếu là reader đầu tiên, chặn writer
    if (reader_count == 1) {
        sem_wait(&writer_queue);
    }
    
    // In thông báo khi một reader đã truy cập tài nguyên
    printf("Reader %d accessed the resource. (Total readers: %d, Timer: %d)\n", id, reader_count, read_count_timer);
    pthread_mutex_unlock(&read_count_lock); // Mở khóa sau khi thay đổi reader_count và read_count_timer
    
    // Đọc dữ liệu (giả lập bằng sleep)
    sleep(1);
    
    // Khóa để thay đổi reader_count
    pthread_mutex_lock(&read_count_lock);
    reader_count--;
    
    // Nếu không còn reader nào, mở khóa cho writer
    if (reader_count == 0) {
        sem_post(&writer_queue);
    }
    pthread_mutex_unlock(&read_count_lock); // Mở khóa sau khi thay đổi reader_count
    
    // Kiểm tra nếu timer là 0 và đặt lại timer, cho phép writer
    pthread_mutex_lock(&writer_lock); 
    if (read_count_timer == 0) {
       
        sem_post(&writer_queue); // Cho phép một writer vào
        sleep(1);                 // Để writer có thời gian truy cập
        read_count_timer = MAX_READERS; // Đặt lại timer về MAX_READERS
    }
    pthread_mutex_unlock(&writer_lock); // Mở khóa sau khi cho phép một writer vào và reset timer
    
    // Cho phép reader tiếp theo vào hàng đợi
    sem_post(&reader_queue);
    
    return NULL;
}

void *writer(void *arg) {
    int id = *(int *)arg;

    // Đợi trong hàng đợi writer
    sem_wait(&writer_queue);
    
    // Kiểm soát writer lock để chắc chắn chỉ một writer vào
    pthread_mutex_lock(&writer_lock);
    printf("Writer %d accessed the resource.\n", id);
    sleep(1); // Giả lập thời gian ghi
    pthread_mutex_unlock(&writer_lock); // Giải phóng quyền truy cập của writer
    
    // Cho phép writer tiếp theo vào hàng đợi
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
    
    // Khởi tạo semaphore và mutex
    sem_init(&reader_queue, 0, MAX_READERS); // Giới hạn số reader đồng thời
    sem_init(&writer_queue, 0, 1); // Giới hạn một writer
    pthread_mutex_init(&read_count_lock, NULL);
    pthread_mutex_init(&writer_lock, NULL);

    // Tạo các reader
    for (int i = 0; i < num_readers; i++) {
        reader_ids[i] = i + 1;
        pthread_create(&readers[i], NULL, reader, &reader_ids[i]);
    }

    // Tạo các writer
    for (int i = 0; i < num_writers; i++) {
        writer_ids[i] = i + 1;
        pthread_create(&writers[i], NULL, writer, &writer_ids[i]);
    }

    // Đợi các thread hoàn tất
    for (int i = 0; i < num_readers; i++) {
        pthread_join(readers[i], NULL);
    }
    for (int i = 0; i < num_writers; i++) {
        pthread_join(writers[i], NULL);
    }

    // Hủy các semaphore và mutex
    sem_destroy(&reader_queue);
    sem_destroy(&writer_queue);
    pthread_mutex_destroy(&read_count_lock);
    pthread_mutex_destroy(&writer_lock);

    return 0;
}
