/*
 * Lab02 Part I - Sudoku Solution Validator
 *
 * Reads a 9x9 Sudoku solution (81 integers separated by whitespace) from
 * standard input and validates it with 11 worker threads:
 *   thread 0      checks that each row contains the digits 1-9
 *   thread 1      checks that each column contains the digits 1-9
 *   threads 2-10  each check one 3x3 subgrid (left to right, top to bottom)
 *
 * Build: mkdir -p build && gcc -std=c99 -O2 -Wall -fno-asm "sudoku.c"
 *        -o "build/sudoku" --static -lm
 * Usage: ./build/sudoku < puzzle.txt
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SIZE 9
#define NUM_THREADS 11

/* structure for passing data to threads */
typedef struct {
    int row;
    int column;
} parameters;

int sudoku[SIZE][SIZE];

/* result[i] is set by worker thread i: 1 = its region is valid, 0 = invalid */
int result[NUM_THREADS];

/* Return 1 if the height x width block whose top-left cell is (row, column)
 * contains each of the digits 1-9 exactly once, otherwise 0. */
int region_is_valid(int row, int column, int height, int width) {
    int seen[SIZE + 1] = {0};
    int i, j;

    for (i = row; i < row + height; i++) {
        for (j = column; j < column + width; j++) {
            int value = sudoku[i][j];

            if (value < 1 || value > SIZE || seen[value]) return 0;
            seen[value] = 1;
        }
    }
    return 1;
}

void* check_rows(void* param) {
    parameters* data = (parameters*)param;
    int valid = 1;
    int i;

    for (i = data->row; i < SIZE && valid; i++)
        valid = region_is_valid(i, 0, 1, SIZE);

    result[0] = valid;
    free(data);
    return NULL;
}

void* check_columns(void* param) {
    parameters* data = (parameters*)param;
    int valid = 1;
    int j;

    for (j = data->column; j < SIZE && valid; j++)
        valid = region_is_valid(0, j, SIZE, 1);

    result[1] = valid;
    free(data);
    return NULL;
}

void* check_subgrid(void* param) {
    parameters* data = (parameters*)param;
    int index = 2 + (data->row / 3) * 3 + data->column / 3;

    result[index] = region_is_valid(data->row, data->column, 3, 3);
    free(data);
    return NULL;
}

void create_worker(pthread_t* tid, void* (*routine)(void*), int row,
                   int column) {
    parameters* data = (parameters*)malloc(sizeof(parameters));
    int err;

    if (data == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    data->row = row;
    data->column = column;

    err = pthread_create(tid, NULL, routine, data);
    if (err != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
}

int main(void) {
    pthread_t workers[NUM_THREADS];
    int valid = 1;
    int i, j;

    for (i = 0; i < SIZE; i++) {
        for (j = 0; j < SIZE; j++) {
            if (scanf("%d", &sudoku[i][j]) != 1) {
                fprintf(stderr,
                        "Error: expected %d integers on standard input\n",
                        SIZE * SIZE);
                return EXIT_FAILURE;
            }
        }
    }

    create_worker(&workers[0], check_rows, 0, 0);
    create_worker(&workers[1], check_columns, 0, 0);
    for (i = 0; i < SIZE; i++)
        create_worker(&workers[2 + i], check_subgrid, (i / 3) * 3, (i % 3) * 3);

    for (i = 0; i < NUM_THREADS; i++) pthread_join(workers[i], NULL);

    for (i = 0; i < NUM_THREADS; i++) {
        printf("Thread %2d: %-7s  ", i, result[i] ? "valid" : "invalid");
        if (i == 0) {
            printf("all rows\n");
        } else if (i == 1) {
            printf("all columns\n");
        } else {
            int row = ((i - 2) / 3) * 3;
            int column = ((i - 2) % 3) * 3;

            printf("subgrid rows %d-%d, columns %d-%d\n", row + 1, row + 3,
                   column + 1, column + 3);
        }
        if (!result[i]) valid = 0;
    }

    printf("The Sudoku solution is %s.\n", valid ? "valid" : "invalid");
    return 0;
}
