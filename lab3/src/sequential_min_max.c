#include <stdio.h>
#include <stdlib.h>

#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
  // Проверка количества аргументов командной строки (должно быть 2 аргумента)
  if (argc != 3) {
    printf("Usage: %s seed arraysize\n", argv[0]);
    return 1;
  }

  // переводим первый аргумент в целое число
  int seed = atoi(argv[1]);
  if (seed <= 0) { // оно должно быть больше 0
    printf("seed is a positive number\n");
    return 1;
  }

  // переводим второй аргумент в целое число
  int array_size = atoi(argv[2]);
  if (array_size <= 0) {
    printf("array_size is a positive number\n");
    return 1;
  }

  // выделяем место под массив
  int *array = malloc(array_size * sizeof(int));
  // создаем массив случайных чисел Array, длинной в array_size с сидом seed
  GenerateArray(array, array_size, seed);
  // находим минимальное и максимальное
  struct MinMax min_max = GetMinMax(array, 0, array_size);
  // чистим память
  free(array);

  // выводим
  printf("min: %d\n", min_max.min);
  printf("max: %d\n", min_max.max);

  return 0;
}
