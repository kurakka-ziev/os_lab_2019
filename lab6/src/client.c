#include <errno.h>
#include <getopt.h>
#include <netdb.h>
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

// структура для хранения данных сервера
struct Server {
  char ip[255];
  int port;
};

// структура для аргументов потока
struct ThreadArgs {
  struct Server server; // данные сервера
  uint64_t begin;       // начало диапазона
  uint64_t end;         // конец диапазона
  uint64_t mod;         // модуль
  uint64_t result;      // результат вычислений
};

// функция для конвертации строки в uint64_t
bool ConvertStringToUI64(const char* str, uint64_t* val) {
  char* end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }
  if (errno != 0) return false;
  *val = i;
  return true;
}

// функция обработки соединения с сервером
void* ProcessServerConnection(void* arg) {
  struct ThreadArgs* args = (struct ThreadArgs*)arg;
  
  // получение имени хоста
  struct hostent* hostname = gethostbyname(args->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", args->server.ip);
    pthread_exit(NULL);
  }

  // настройка адреса сервера
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(args->server.port);
  server.sin_addr.s_addr = *((unsigned long*)hostname->h_addr);

  // создание сокета
  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    pthread_exit(NULL);
  }

  // подключение к серверу
  if (connect(sck, (struct sockaddr*)&server, sizeof(server)) < 0) {
    fprintf(stderr, "Connection failed\n");
    close(sck);
    pthread_exit(NULL);
  }

  // подготовка и отправка задания
  char task[sizeof(uint64_t) * 3];
  memcpy(task, &args->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &args->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &args->mod, sizeof(uint64_t));

  if (send(sck, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send failed\n");
    close(sck);
    pthread_exit(NULL);
  }

  // получение результата
  char response[sizeof(uint64_t)];
  if (recv(sck, response, sizeof(response), 0) < 0) {
    fprintf(stderr, "Receive failed\n");
    close(sck);
    pthread_exit(NULL);
  }

  // сохранение результата
  memcpy(&args->result, response, sizeof(uint64_t));
  close(sck);
  pthread_exit(NULL);
}

// функция для чтения данных серверов из файла
struct Server* ReadServersFile(const char* filename, unsigned int* servers_num) {
  FILE* file = fopen(filename, "r");
  if (!file) {
    fprintf(stderr, "Cannot open server file\n");
    return NULL;
  }

  *servers_num = 0;
  char line[256];

  // подсчет количества серверов
  while (fgets(line, sizeof(line), file)) {
    (*servers_num)++;
  }

  struct Server* servers = malloc(sizeof(struct Server) * (*servers_num));
  if (!servers) {
    fclose(file);
    return NULL;
  }

  rewind(file); // возврат к началу файла
  int i = 0;
  
  // чтение данных серверов
  while (fgets(line, sizeof(line), file)) {
    char* ip = strtok(line, ":");
    char* port = strtok(NULL, "\n");
    if (ip && port) {
      strncpy(servers[i].ip, ip, sizeof(servers[i].ip) - 1);
      servers[i].port = atoi(port);
      i++;
    }
  }

  fclose(file);
  return servers;
}

int main(int argc, char** argv) {
  uint64_t k = 0; // значение k
  uint64_t mod = 0; // значение модуль
  char servers_file[255] = {'\0'}; // путь к файлу серверов
  bool k_set = false; // флаг установки k
  bool mod_set = false; // флаг установки модуля

  // разбор аргументов командной строки
  while (true) {
    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0: {
        switch (option_index) {
          case 0:
            if (ConvertStringToUI64(optarg, &k)) k_set = true;
            break;
          case 1:
            if (ConvertStringToUI64(optarg, &mod)) mod_set = true;
            break;
          case 2:
            memcpy(servers_file, optarg, strlen(optarg));
            break;
          default:
            printf("Index %d is out of options\n", option_index);
        }
      } break;
      case '?':
        printf("Arguments error\n");
        break;
      default:
        fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  // проверка обязательных аргументов
  if (!k_set || !mod_set || !strlen(servers_file)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  // получение серверов из файла
  unsigned int servers_num = 0;
  struct Server* servers = ReadServersFile(servers_file, &servers_num);
  if (!servers || servers_num == 0) {
    fprintf(stderr, "Failed to read servers from file\n");
    return 1;
  }

  // создание потоков
  pthread_t* threads = malloc(sizeof(pthread_t) * servers_num);
  struct ThreadArgs* thread_args =
      malloc(sizeof(struct ThreadArgs) * servers_num);

  // разделение работы между потоками
  uint64_t chunk_size = k / servers_num;
  uint64_t remainder = k % servers_num;
  uint64_t current_begin = 1;

  for (unsigned int i = 0; i < servers_num; i++) {
    thread_args[i].server = servers[i];
    thread_args[i].begin = current_begin;
    thread_args[i].end = current_begin + chunk_size - 1;
    if (i == servers_num - 1) {
      thread_args[i].end += remainder;
    }
    thread_args[i].mod = mod;
    current_begin = thread_args[i].end + 1;

    // создание потока
    if (pthread_create(&threads[i], NULL, ProcessServerConnection,
                       &thread_args[i])) {
      fprintf(stderr, "Failed to create thread\n");
      free(threads);
      free(thread_args);
      free(servers);
      return 1;
    }
  }

  uint64_t final_result = 1;

  // ожидание завершения потоков и вычисление результата
  for (unsigned int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    final_result = MultModulo(final_result, thread_args[i].result, mod);
  }

  printf("Final result: %lu\n", final_result);

  // освобождение памяти
  free(threads);
  free(thread_args);
  free(servers);

  return 0;
}
