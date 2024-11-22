#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct FactorialArgs {
    long long start;  // начало диапазона 
    long long end;    // конец диапазона
    long long mod;    // модуль
};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // мьюинг
long long result = 1; // результат

// вычисления в диапазонах для тредов
void *compute_factorial(void *args) {
    struct FactorialArgs *factorial_args = (struct FactorialArgs *)args;
    long long partial_result = 1;

    for (long long i = factorial_args->start; i <= factorial_args->end; i++) {
        partial_result = (partial_result * i) % factorial_args->mod;
    }

    // мьютекс
    pthread_mutex_lock(&mutex);
    result = (result * partial_result) % factorial_args->mod;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 7) {
        fprintf(stderr, "Usage: %s -k <k> --pnum <pnum> --mod <mod>\n", argv[0]);
        return 1;
    }

    // парсим аргументы командной строки
    long long k = 0, pnum = 0, mod = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0) {
            k = atol(argv[++i]);
        } else if (strcmp(argv[i], "--pnum") == 0) {
            pnum = atol(argv[++i]);
        } else if (strcmp(argv[i], "--mod") == 0) {
            mod = atol(argv[++i]);
        }
    }

    if (k <= 0 || pnum <= 0 || mod <= 0) {
        fprintf(stderr, "Invalid input values.\n");
        return 1;
    }

    // создаем массив тредов
    pthread_t threads[pnum];

    // считанм диапазоны для каждого треда
    long long range_size = k / pnum;
    long long remainder = k % pnum;

    long long start = 1;
    for (long long i = 0; i < pnum; i++) {
        long long end = start + range_size - 1;
        if (i < remainder) {
            end++;
        }

        struct FactorialArgs *args = (struct FactorialArgs *)malloc(sizeof(struct FactorialArgs));
        args->start = start;
        args->end = end;
        args->mod = mod;

        pthread_create(&threads[i], NULL, compute_factorial, (void *)args);

        start = end + 1;
    }

    // ожидаем завершения всех тредов
    for (long long i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Factorial of %lld mod %lld is: %lld\n", k, mod, result);

    return 0;
}
