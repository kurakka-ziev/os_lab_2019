#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "utils.h"

struct SumArgs {
  int *array;
  int begin;
  int end;
};

// сумма
int Sum(const struct SumArgs *args) {
  int sum = 0;
  for (int i = args->begin; i < args->end; i++) {
    sum += args->array[i];
  }
  return sum;
}

// вызов функции Sum в потоке
void *ThreadSum(void *args) {
  struct SumArgs *sum_args = (struct SumArgs *)args;
  return (void *)(size_t)Sum(sum_args);
}

int main(int argc, char **argv) {
  if (argc != 7) {
    printf("Usage: ./psum --threads_num num --seed num --array_size num\n");
    return 1;
  }

  // аргументы
  uint32_t threads_num = 0;
  uint32_t array_size = 0;
  uint32_t seed = 0;

  for (int i = 1; i < argc; i += 2) {
    if (strcmp(argv[i], "--threads_num") == 0) {
      threads_num = atoi(argv[i + 1]);
    } else if (strcmp(argv[i], "--seed") == 0) {
      seed = atoi(argv[i + 1]);
    } else if (strcmp(argv[i], "--array_size") == 0) {
      array_size = atoi(argv[i + 1]);
    } else {
      printf("Invalid argument: %s\n", argv[i]);
      return 1;
    }
  }

  // проверяем корректность аргументов
  if (threads_num == 0 || array_size == 0) {
    printf("threads_num and array_size must be greater than 0\n");
    return 1;
  }

  // выделяем память под массив
  int *array = malloc(sizeof(int) * array_size);
  if (!array) {
    printf("Error: unable to allocate memory for the array\n");
    return 1;
  }

  GenerateArray(array, array_size, seed);

  // создаем массив аргументов для потоков
  struct SumArgs args[threads_num];
  pthread_t threads[threads_num];

  // разбиваем массив на части для потоков
  int chunk_size = array_size / threads_num;
  int remaining = array_size % threads_num;

  // начинаем счет времени
  struct timespec start_time, end_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);

  for (uint32_t i = 0; i < threads_num; i++) {
    args[i].array = array;
    args[i].begin = i * chunk_size;
    args[i].end = args[i].begin + chunk_size;
    if (i == threads_num - 1) {
      args[i].end += remaining; // последний поток берет остаток
    }

    if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i]) != 0) {
      printf("Error: pthread_create failed!\n");
      free(array);
      return 1;
    }
  }

  int total_sum = 0;
  for (uint32_t i = 0; i < threads_num; i++) {
    int sum = 0;
    pthread_join(threads[i], (void **)&sum);
    total_sum += sum;
  }

  // заканчиваем счет времени
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  double elapsed_time = (end_time.tv_sec - start_time.tv_sec) +
                        (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

  printf("Total sum: %d\n", total_sum);
  printf("Time taken: %.6f seconds\n", elapsed_time);

  free(array);
  return 0;
}
