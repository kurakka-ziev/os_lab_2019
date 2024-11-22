#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// создаем два мьютекса
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;

// функция для первого потока
void *thread_func1(void *arg) {
    // блокируем первый мьютекс
    printf("Поток 1: блокирует mutex1\n");
    pthread_mutex_lock(&mutex1);
    sleep(1); // симуляция работы, чтобы дать время второму потоку заблокировать mutex2

    printf("Поток 1: пытается заблокировать mutex2\n");
    pthread_mutex_lock(&mutex2); // дедлок ------
    printf("Поток 1: оба мьютекса заблокированы\n");

    pthread_mutex_unlock(&mutex2);
    pthread_mutex_unlock(&mutex1);

    return NULL;
}

// второй поток
void *thread_func2(void *arg) {
    // блокируем второй мьютекс
    printf("Поток 2: блокирует mutex2\n");
    pthread_mutex_lock(&mutex2);
    sleep(1); // симуляция работы, чтобы дать время первому потоку заблокировать mutex1

    printf("Поток 2: пытается заблокировать mutex1\n");
    pthread_mutex_lock(&mutex1); // дедлок --------
    printf("Поток 2: оба мьютекса заблокированы\n");

    pthread_mutex_unlock(&mutex1);
    pthread_mutex_unlock(&mutex2);

    return NULL;
}

int main() {
    pthread_t thread1, thread2;

    // создаем два потока
    pthread_create(&thread1, NULL, thread_func1, NULL);
    pthread_create(&thread2, NULL, thread_func2, NULL);

    // ждем завершения
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    printf("Program's end\n"); // это не должно вывестись из-за дедлока

    return 0;
}
