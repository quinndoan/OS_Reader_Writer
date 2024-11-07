#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#define SIZE 1000
#define MAX 10

int readcount=1;

sem_t resource;                    // controls access (read/write) to the resource
sem_t rmutex;                      // for syncing changes to shared variable readcount
sem_t serviceQueue;                // FAIRNESS: preserves ordering of requests (FIFO)

sem_t reader_queue;   // Hàng đợi cho reader
sem_t writer_queue;   // Hàng đợi cho writer

pthread_mutex_t write_count_lock; // Mutex to protect writer_count
pthread_mutex_t reader_lock; // Mutex to ensure only one reader at a time

int writer_count = 0;     // Current number of writers
int write_count_timer = MAX; // Counter to limit writers

pthread_mutex_t read_count_lock; // Mutex để bảo vệ biến reader_count
pthread_mutex_t writer_lock; // Mutex để đảm bảo chỉ có một writer vào tại một thời điểm

int reader_count = 0; // Số lượng reader hiện tại
int read_count_timer = MAX; // Biến timer để đếm số lượng reader vào

pthread_t readers[SIZE], writers[SIZE];  // Mảng lưu thread reader và writer

//char client[][10] =
//{
//    "reader", "writer", "reader", "reader", "reader", "reader", "writer",
//    "reader", "reader", "reader", "reader", "reader", "writer", "reader",
//    "reader", "reader", "reader", "reader", "writer", "reader", "writer",
//    "writer", "writer", "reader"
//};

//char client[][10] =
//{
//    "reader", "reader", "reader", "reader", "reader", "reader", "writer",
//    "reader", "reader", "reader", "reader", "reader", "writer", "reader",
//    "reader", "reader", "reader", "reader", "reader", "reader", "reader",
//    "reader", "reader", "reader"
//};
//
//char client[][10] =
//{
//    "reader", "writer", "writer", "writer", "writer", "writer", "writer",
//    "reader", "writer", "writer", "writer", "writer", "writer", "writer",
//    "writer", "writer", "writer", "writer", "writer", "writer", "writer",
//    "writer", "writer"
//};

int num_readers = 0;
int num_writers = 0;

// Reader function
void *reader_fairness(void *arg)
{
//    printf("Reader %ld is in queue\n", (long)arg);
    // ENTRY Section
    sem_wait(&serviceQueue);       // wait in line to be serviced
    sem_wait(&rmutex);             // request exclusive access to readcount
    readcount++;                   // update count of active readers
    if (readcount == 1)            // if I am the first reader
    {
        sem_wait(&resource);       // request resource access for readers (writers blocked)
    }
    sem_post(&serviceQueue);       // let next in line be serviced
    sem_post(&rmutex);             // release access to readcount

    // CRITICAL Section
    printf("Reader %ld is reading\n", (long)arg);

    // EXIT Section
    sem_wait(&rmutex);             // request exclusive access to readcount
    readcount--;                   // update count of active readers
    if (readcount == 0)            // if there are no readers left
    {
        sem_post(&resource);       // release resource access for all
    }
    sem_post(&rmutex);             // release access to readcount
    return NULL;
}

// Writer function
void *writer_fairness(void *arg)
{
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
    // Initialize semaphores
    sem_init(&resource, 0, 1);     // Binary semaphore for resource (1 for read/write access)
    sem_init(&rmutex, 0, 1);       // Semaphore for protecting readcount (mutex)
    sem_init(&serviceQueue, 0, 1); // Binary semaphore for FIFO queue handling

    // Seed for random number generation
    srand(time(NULL));

    // Create reader and writer threads in random order
    int total_threads = num_readers + num_writers;

    // Mảng để theo dõi các thread đã tạo
    pthread_t all_threads[total_threads];

    for (int i = 0; i < total_threads; i++) {
        // Chọn ngẫu nhiên giữa Reader (0) và Writer (1)
        int is_reader = rand() % 2; // 0: Reader, 1: Writer

        if (is_reader && num_readers > 0) {
            num_readers--; // Giảm số lượng readers
            pthread_create(&all_threads[i], NULL, reader_fairness, (void *)(long)i);
        }
        else if (!is_reader && num_writers > 0) {
            num_writers--; // Giảm số lượng writers
            pthread_create(&all_threads[i], NULL, writer_fairness, (void *)(long)i);
        } else {
            // Nếu không còn Reader hay Writer nào để tạo, giảm i và thử lại
            i--;
        }
    }

    // Đợi tất cả các thread hoàn thành
    for (int i = 0; i < total_threads; i++) {
        pthread_join(all_threads[i], NULL);
    }

    // Destroy semaphores
    sem_destroy(&resource);
    sem_destroy(&rmutex);
    sem_destroy(&serviceQueue);

}

void *reader_1(void *arg) {
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
        read_count_timer = MAX; // Đặt lại timer về MAX_READERS
    }
    pthread_mutex_unlock(&writer_lock); // Mở khóa sau khi cho phép một writer vào và reset timer

    // Cho phép reader tiếp theo vào hàng đợi
    sem_post(&reader_queue);

    return NULL;
}

void *writer_1(void *arg) {
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

void reader_prefer()
{
    pthread_t readers[num_readers], writers[num_writers];
    int reader_ids[num_readers], writer_ids[num_writers];

    // Khởi tạo semaphore và mutex
    sem_init(&reader_queue, 0, MAX); // Giới hạn số reader đồng thời
    sem_init(&writer_queue, 0, 1); // Giới hạn một writer
    pthread_mutex_init(&read_count_lock, NULL);
    pthread_mutex_init(&writer_lock, NULL);

    // Tạo các reader
    for (int i = 0; i < num_readers; i++) {
        reader_ids[i] = i + 1;
        pthread_create(&readers[i], NULL, reader_1, &reader_ids[i]);
    }

    // Tạo các writer
    for (int i = 0; i < num_writers; i++) {
        writer_ids[i] = i + 1;
        pthread_create(&writers[i], NULL, writer_1, &writer_ids[i]);
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
}

void *writer_2(void *arg) {
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
        write_count_timer = MAX; // Reset the timer
        sem_post(&reader_queue);         // Allow readers to access
        sleep(1);
        write_count_timer = MAX;
    }
     pthread_mutex_unlock(&reader_lock);
    // Allow the next writer in the queue
    sem_post(&writer_queue);

    return NULL;
}

void *reader_2(void *arg) {
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

void writer_prefer()
{
    pthread_t readers[num_readers], writers[num_writers];
    int reader_ids[num_readers], writer_ids[num_writers];

    // Initialize semaphores and mutexes
    sem_init(&writer_queue, 0, MAX);       // Only one reader can request at a time
    sem_init(&reader_queue, 0, 1);       // Only one writer allowed at a time
    pthread_mutex_init(&write_count_lock, NULL);
    pthread_mutex_init(&reader_lock, NULL);

    // Create writer threads
    for (int i = 0; i < num_writers; i++) {
        writer_ids[i] = i + 1;
        pthread_create(&writers[i], NULL, writer_2, &writer_ids[i]);
    }
    // Create reader threads
    for (int i = 0; i < num_readers; i++) {
        reader_ids[i] = i + 1;
        pthread_create(&readers[i], NULL, reader_2, &reader_ids[i]);
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

}
int main()
{
    printf("Enter the number of readers: ");
    scanf("%d", &num_readers);
    printf("Enter the number of writers: ");
    scanf("%d", &num_writers);

    if(num_readers/10 >= num_writers) reader_prefer();
    else if(num_writers/10 >= num_readers) writer_prefer();
    else fairness();

    return 0;
}
