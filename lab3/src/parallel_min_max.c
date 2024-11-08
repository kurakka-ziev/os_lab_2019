#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include "find_min_max.h"
#include "utils.h"

int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    bool with_files = false;

    // Разбор аргументов командной строки
    while (true) {
        static struct option options[] = {
            {"seed", required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"pnum", required_argument, 0, 0},
            {"by_files", no_argument, 0, 'f'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "f", options, &option_index);

        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        seed = atoi(optarg);
                        break;
                    case 1:
                        array_size = atoi(optarg);
                        break;
                    case 2:
                        pnum = atoi(optarg);
                        break;
                    case 3:
                        with_files = true;
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
                break;
            case 'f':
                with_files = true;
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
        }
    }

    if (optind < argc) {
        printf("Has at least one no option argument\n");
        return 1;
    }

    if (seed == -1 || array_size == -1 || pnum == -1) {
        printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\"\n", argv[0]);
        return 1;
    }

    // Выделяем память под массив
    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);
    int active_child_processes = 0;

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // Массив для хранения файлов/pipe
    FILE **files = NULL;
    int pipefds[2 * pnum][2]; // pipe для каждого процесса

    for (int i = 0; i < pnum; i++) {
        pid_t child_pid = fork();
        if (child_pid >= 0) {
            active_child_processes += 1;
            if (child_pid == 0) {
                // Дочерний процесс
                int start_index = i * (array_size / pnum);
                int end_index = (i == pnum - 1) ? array_size : (i + 1) * (array_size / pnum);
                struct MinMax min_max = GetMinMax(array, start_index, end_index);

                if (with_files) {
                    // Запись в файл (каждый процесс записывает в свой файл)
                    char filename[20];
                    sprintf(filename, "result_%d.txt", i);
                    FILE *file = fopen(filename, "w");
                    fprintf(file, "Min: %d\nMax: %d\n", min_max.min, min_max.max);
                    fclose(file);
                } else {
                    // pipe
                    write(pipefds[i][1], &min_max, sizeof(min_max));
                    close(pipefds[i][1]);
                }

                free(array); // Освобождаем память в дочернем процессе
                exit(0); // Завершаем дочерний процесс
            }
        } else {
            printf("Fork failed!\n");
            return 1;
        }
    }

    // Родительский процесс
    struct MinMax min_max;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    for (int i = 0; i < pnum; i++) {
        // Ожидаем завершения всех дочерних процессов
        wait(NULL);

        if (with_files) {
            // Чтение из файлов
            char filename[20];
            sprintf(filename, "result_%d.txt", i);
            FILE *file = fopen(filename, "r");
            if (file != NULL) {
                int min, max;
                fscanf(file, "Min: %d\nMax: %d\n", &min, &max);
                if (min < min_max.min) min_max.min = min;
                if (max > min_max.max) min_max.max = max;
                fclose(file);
            }
        } else {
            // Чтение из pipe
            struct MinMax child_min_max;
            read(pipefds[i][0], &child_min_max, sizeof(child_min_max));
            if (child_min_max.min < min_max.min) min_max.min = child_min_max.min;
            if (child_min_max.max > min_max.max) min_max.max = child_min_max.max;
            close(pipefds[i][0]);
        }
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);

    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);

    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
    printf("Elapsed time: %fms\n", elapsed_time);

    return 0;
}
