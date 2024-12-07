#include <getopt.h>
#include <limits.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

struct FactorialArgs {
  uint64_t begin; // начало диапазона для вычисления факториала
  uint64_t end;   // конец диапазона для вычисления факториала
  uint64_t mod;   // модуль для вычисления остатка
};

uint64_t Factorial(const struct FactorialArgs *args) {
  uint64_t ans = 1;
  // вычисление факториала в указанном диапазоне с использованием модуля
  for (uint64_t i = args->begin; i <= args->end; i++) {
    ans = MultModulo(ans, i, args->mod);
  }
  return ans;
}

void *ThreadFactorial(void *args) {
  struct FactorialArgs *fargs = (struct FactorialArgs *)args;
  uint64_t *result = malloc(sizeof(uint64_t));
  *result = Factorial(fargs); // вычисление факториала для данного диапазона
  return (void *)result;      // возвращаем результат
}

int main(int argc, char **argv) {
  int tnum = -1; // количество потоков
  int port = -1; // порт сервера

  while (true) {
    static struct option options[] = {{"port", required_argument, 0, 0},
                                      {"tnum", required_argument, 0, 0},
                                      {0, 0, 0, 0}}; // обработка аргументов командной строки

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0: {
        switch (option_index) {
          case 0:
            port = atoi(optarg); // парсинг значения порта
            break;
          case 1:
            tnum = atoi(optarg); // парсинг количества потоков
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
      } break;
      case '?':
        printf("Unknown argument\n");
        break;
      default:
        fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (port == -1 || tnum == -1) {
    fprintf(stderr, "Using: %s --port 20000 --tnum 4\n", argv[0]);
    return 1; // проверка обязательных параметров
  }

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    fprintf(stderr, "Can not create server socket!\n");
    return 1; // создание сокета сервера
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET; // настройка параметров сервера
  server.sin_port = htons((uint16_t)port);
  server.sin_addr.s_addr = htonl(INADDR_ANY);

  int opt_val = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

  int err = bind(server_fd, (struct sockaddr *)&server, sizeof(server));
  if (err < 0) {
    fprintf(stderr, "Can not bind to socket!\n");
    return 1; // привязка сокета к адресу
  }

  err = listen(server_fd, 128);
  if (err < 0) {
    fprintf(stderr, "Could not listen on socket\n");
    return 1; // ожидание подключений
  }

  printf("Server listening at %d\n", port);

  while (true) {
    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len);

    if (client_fd < 0) {
      fprintf(stderr, "Could not establish new connection\n");
      continue; // обработка подключения клиента
    }

    while (true) {
      unsigned int buffer_size = sizeof(uint64_t) * 3;
      char from_client[buffer_size];
      ssize_t read_bytes = recv(client_fd, from_client, buffer_size, 0);

      if (!read_bytes) break; // завершение передачи
      if (read_bytes < 0) {
        fprintf(stderr, "Client read failed\n");
        break;
      }
      if ((size_t)read_bytes < buffer_size) {
        fprintf(stderr, "Client send wrong data format\n");
        break;
      }

      pthread_t threads[tnum];
      uint64_t begin = 0;
      uint64_t end = 0;
      uint64_t mod = 0;
      memcpy(&begin, from_client, sizeof(uint64_t));
      memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t));
      memcpy(&mod, from_client + 2 * sizeof(uint64_t), sizeof(uint64_t));

      fprintf(stdout, "Receive: %lu %lu %lu\n", begin, end, mod);

      // разделение работы между потоками
      struct FactorialArgs args[tnum];
      uint64_t chunk_size = (end - begin + 1) / (uint64_t)tnum;
      uint64_t remainder = (end - begin + 1) % (uint64_t)tnum;
      uint64_t current_begin = begin;

      for (int i = 0; i < tnum; i++) {
        args[i].begin = current_begin;
        args[i].end = current_begin + chunk_size - 1;
        if (i == tnum - 1) {
          args[i].end += remainder; // добавление остатка последнему потоку
        }
        args[i].mod = mod;
        current_begin = args[i].end + 1;

        if (pthread_create(&threads[i], NULL, ThreadFactorial,
                           (void *)&args[i])) {
          printf("Error: pthread_create failed!\n");
          return 1; // создание потоков
        }
      }

      uint64_t total = 1;
      for (int i = 0; i < tnum; i++) {
        uint64_t *thread_result;
        pthread_join(threads[i], (void **)&thread_result); // ожидание завершения потоков
        total = MultModulo(total, *thread_result, mod);    // вычисление результата
        free(thread_result);
      }

      printf("Total: %lu\n", total);

      // отправка результата клиенту
      char buffer[sizeof(total)];
      memcpy(buffer, &total, sizeof(total));
      err = send(client_fd, buffer, sizeof(total), 0);
      if (err < 0) {
        fprintf(stderr, "Can't send data to client\n");
        break;
      }
    }

    shutdown(client_fd, SHUT_RDWR); // завершение соединения с клиентом
    close(client_fd);
  }

  close(server_fd); // закрытие сокета сервера
  return 0;
}
