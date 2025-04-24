#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

// Shared buffer and counters
int item_to_produce = 0, curr_buf_size = 0;
int total_items, max_buf_size, num_workers, num_masters;
int *buffer;

// Synchronization variables
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

int next_item_index = 0;  // For consumer
int items_consumed = 0;   // Global tracker

// Logging functions (DO NOT MODIFY)
void print_produced(int num, int master) {
  printf("Produced %d by master %d\n", num, master);
}

void print_consumed(int num, int worker) {
  printf("Consumed %d by worker %d\n", num, worker);
}

// Master thread function
void *generate_requests_loop(void *data) {
  int thread_id = *((int *)data);

  while (1) {
    pthread_mutex_lock(&mutex);

    if (item_to_produce >= total_items) {
      pthread_mutex_unlock(&mutex);
      break;
    }

    while (curr_buf_size == max_buf_size)
      pthread_cond_wait(&not_full, &mutex);

    if (item_to_produce >= total_items) {
      pthread_mutex_unlock(&mutex);
      break;
    }

    buffer[(curr_buf_size) % max_buf_size] = item_to_produce;
    print_produced(item_to_produce, thread_id);
    curr_buf_size++;
    item_to_produce++;

    pthread_cond_signal(&not_empty);
    pthread_mutex_unlock(&mutex);
  }

  return NULL;
}

// Worker thread function
void *consume_requests_loop(void *data) {
  int thread_id = *((int *)data);

  while (1) {
    pthread_mutex_lock(&mutex);

    while (curr_buf_size == 0 && items_consumed < total_items)
      pthread_cond_wait(&not_empty, &mutex);

    if (items_consumed >= total_items) {
      pthread_mutex_unlock(&mutex);
      break;
    }

    int item = buffer[next_item_index % max_buf_size];
    next_item_index++;
    curr_buf_size--;
    items_consumed++;

    print_consumed(item, thread_id);

    pthread_cond_signal(&not_full);
    pthread_mutex_unlock(&mutex);
  }

  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc < 5) {
    printf("./master-worker #total_items #max_buf_size #num_workers #masters\n");
    exit(1);
  }

  // Parse arguments
  total_items = atoi(argv[1]);
  max_buf_size = atoi(argv[2]);
  num_workers = atoi(argv[3]);
  num_masters = atoi(argv[4]);

  buffer = (int *)malloc(sizeof(int) * max_buf_size);

  // Create master threads
  int *master_thread_id = (int *)malloc(sizeof(int) * num_masters);
  pthread_t *master_thread = (pthread_t *)malloc(sizeof(pthread_t) * num_masters);

  for (int i = 0; i < num_masters; i++) {
    master_thread_id[i] = i;
    pthread_create(&master_thread[i], NULL, generate_requests_loop, &master_thread_id[i]);
  }

  // Create worker threads
  int *worker_thread_id = (int *)malloc(sizeof(int) * num_workers);
  pthread_t *worker_thread = (pthread_t *)malloc(sizeof(pthread_t) * num_workers);

  for (int i = 0; i < num_workers; i++) {
    worker_thread_id[i] = i;
    pthread_create(&worker_thread[i], NULL, consume_requests_loop, &worker_thread_id[i]);
  }

  // Join master threads
  for (int i = 0; i < num_masters; i++) {
    pthread_join(master_thread[i], NULL);
    printf("master %d joined\n", i);
  }

  // Join worker threads
  for (int i = 0; i < num_workers; i++) {
    pthread_join(worker_thread[i], NULL);
    printf("worker %d joined\n", i);
  }

  // Cleanup
  free(buffer);
  free(master_thread_id);
  free(master_thread);
  free(worker_thread_id);
  free(worker_thread);

  return 0;
}