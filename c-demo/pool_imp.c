#include <bits/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>  // for clock_gettime

#define INPUT_SIZE 1000000
#define MAX_TASKS 128

typedef struct {
    const int* input;
    int* output;
    size_t start;
    size_t end;
    int (*func)(int);
} Task;

typedef struct {
    Task tasks[MAX_TASKS];
    size_t count;
    size_t head;
    size_t tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool shutdown;
} TaskQueue;

typedef struct {
    TaskQueue* queue;
} ThreadArg;

TaskQueue task_queue;
pthread_t* thread_pool;
size_t num_threads;

void* worker(void* arg) {
    TaskQueue* queue = ((ThreadArg*)arg)->queue;
    free(arg);

    while (true) {
        pthread_mutex_lock(&queue->mutex);
        while (queue->count == 0 && !queue->shutdown) {
            pthread_cond_wait(&queue->cond, &queue->mutex);
        }

        if (queue->shutdown && queue->count == 0) {
            pthread_mutex_unlock(&queue->mutex);
            break;
        }

        Task task = queue->tasks[queue->head];
        queue->head = (queue->head + 1) % MAX_TASKS;
        queue->count--;
        pthread_mutex_unlock(&queue->mutex);

        for (size_t i = task.start; i < task.end; ++i) {
            task.output[i] = task.func(task.input[i]);
        }
    }

    return NULL;
}

void thread_pool_init(size_t threads) {
    num_threads = threads;
    thread_pool = malloc(sizeof(pthread_t) * threads);
    task_queue = (TaskQueue){.count = 0, .head = 0, .tail = 0, .shutdown = false};
    pthread_mutex_init(&task_queue.mutex, NULL);
    pthread_cond_init(&task_queue.cond, NULL);

    for (size_t i = 0; i < threads; ++i) {
        ThreadArg* arg = malloc(sizeof(ThreadArg));
        arg->queue = &task_queue;
        pthread_create(&thread_pool[i], NULL, worker, arg);
    }
}

void thread_pool_submit(Task task) {
    pthread_mutex_lock(&task_queue.mutex);

    while (task_queue.count == MAX_TASKS) {
        // optional: implement blocking or drop strategy
    }

    task_queue.tasks[task_queue.tail] = task;
    task_queue.tail = (task_queue.tail + 1) % MAX_TASKS;
    task_queue.count++;
    pthread_cond_signal(&task_queue.cond);

    pthread_mutex_unlock(&task_queue.mutex);
}

void thread_pool_shutdown() {
    pthread_mutex_lock(&task_queue.mutex);
    task_queue.shutdown = true;
    pthread_cond_broadcast(&task_queue.cond);
    pthread_mutex_unlock(&task_queue.mutex);

    for (size_t i = 0; i < num_threads; ++i) {
        pthread_join(thread_pool[i], NULL);
    }

    free(thread_pool);
    pthread_mutex_destroy(&task_queue.mutex);
    pthread_cond_destroy(&task_queue.cond);
}

int* par_map(const int* input, size_t len, int (*func)(int)) {
    int* output = malloc(sizeof(int) * len);
    size_t chunk_size = (len + num_threads - 1) / num_threads;

    for (size_t t = 0; t < num_threads; ++t) {
        size_t start = t * chunk_size;
        size_t end = (start + chunk_size < len) ? start + chunk_size : len;

        if (start >= end) break;

        Task task = {
            .input = input,
            .output = output,
            .start = start,
            .end = end,
            .func = func
        };
        thread_pool_submit(task);
    }

    // Wait for all work to finish (simple sync strategy)
    thread_pool_shutdown();

    return output;
}

int* seq_map(const int* input, size_t len, int (*func)(int)) {
    int* output = malloc(sizeof(int) * len);
    for (size_t i = 0; i < len; ++i) {
        output[i] = func(input[i]);
    }
    return output;
}



int cube(int x) {
    return x * x * x;
}
int main() {
    int* input = malloc(sizeof(int) * INPUT_SIZE);
    for (int i = 0; i < INPUT_SIZE; ++i) {
        input[i] = i*2;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    thread_pool_init(4);
    int* output = par_map(input, INPUT_SIZE, cube);

    clock_gettime(CLOCK_MONOTONIC, &end);

    // Uncomment this if you want to print the results
    // for (int i = 0; i < INPUT_SIZE; ++i) {
    //     printf("%d ", output[i]);
    // }
    // printf("\n");

    free(output);

    double duration = (end.tv_sec - start.tv_sec) +
                      (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Parallel execution time: %.6f seconds\n", duration);
    struct timespec start_seq, end_seq;
    clock_gettime(CLOCK_MONOTONIC, &start_seq);

    int* output_seq = seq_map(input, INPUT_SIZE, cube);
    clock_gettime(CLOCK_MONOTONIC, &end_seq);
    double duration_seq = (end_seq.tv_sec - start_seq.tv_sec) +
                      (end_seq.tv_nsec - start_seq.tv_nsec) / 1e9;

    printf("SEQ execution time: %.6f seconds\n", duration_seq);


    return 0;
}
