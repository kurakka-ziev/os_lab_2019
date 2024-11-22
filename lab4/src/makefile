CC=gcc
CFLAGS=-I.

all: sequential_min_max parallel_min_max run_smm process_memory parallel_sum

sequential_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o sequential_min_max find_min_max.o utils.o sequential_min_max.c $(CFLAGS)

run_smm: utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o run_smm utils.o find_min_max.o run_smm.c $(CFLAGS)

parallel_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

utils.o : utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o : utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

process_memory :
	$(CC) -o process_memory process_memory.c -w

parallel_sum : utils.o utils.h
	$(CC) -o parallel_sum parallel_sum.c utils.o -lpthread $(CFLAGS)

clean :
	rm utils.o find_min_max.o sequential_min_max parallel_min_max run_smm process_memory parallel_sum
