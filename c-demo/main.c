#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define INPUT_SIZE 10000000

typedef struct {
    const int* input;
    int* output;
    size_t start;
    size_t end;
    int (*func)(int);
} ThreadData;

void* thread_worker(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    for (size_t i = data->start; i < data->end; ++i) {
        data->output[i] = data->func(data->input[i]);
    }
    return NULL;
}

int* par_map(const int* input, size_t len, int (*func)(int), size_t num_threads) {
    pthread_t* threads = malloc(sizeof(pthread_t) * num_threads);
    ThreadData* thread_data = malloc(sizeof(ThreadData) * num_threads);
    int* output = malloc(sizeof(int) * len);
    size_t chunk_size = (len + num_threads - 1) / num_threads;

    for (size_t t = 0; t < num_threads; ++t) {
        size_t start = t * chunk_size;
        size_t end = (start + chunk_size < len) ? start + chunk_size : len;

        thread_data[t] = (ThreadData){
            .input = input,
            .output = output,
            .start = start,
            .end = end,
            .func = func
        };

        pthread_create(&threads[t], NULL, thread_worker, &thread_data[t]);
    }

    for (size_t t = 0; t < num_threads; ++t) {
        pthread_join(threads[t], NULL);
    }

    free(threads);
    free(thread_data);

    return output;
}

int add_one(int x) {
    return x + 1;
}

int* seq_map(const int* input, size_t len, int (*func)(int)) {
    int* output = malloc(sizeof(int) * len);
    for (size_t i = 0; i < len; ++i) {
        output[i] = func(input[i]);
    }
    return output;
}

double millis(clock_t start, clock_t end) {
    return ((double)(end - start) * 1000.0) / CLOCKS_PER_SEC;
}

int main() {
    int* input = malloc(sizeof(int) * INPUT_SIZE);
    for (int i = 0; i < INPUT_SIZE; i++) {
        input[i] = i;
    }

    clock_t start2, end2;

    // Sequential
    start2 = clock();
    int* output_seq = seq_map(input, INPUT_SIZE, add_one);
    end2 = clock();
    printf("Sequential took %.2f ms\n", millis(start2, end2));
    free(output_seq);

    // Parallel
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    int* output_par = par_map(input, INPUT_SIZE, add_one, 8);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                    (end.tv_nsec - start.tv_nsec) / 1e6;
    printf("Parallel (8 threads) took %.2f ms\n", elapsed_ms);
    free(output_par);

    free(input);
    return 0;
}

