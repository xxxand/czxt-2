/*
 * Lab02 Part II - Multithreaded Sorting Application
 *
 * Reads a list of integers (separated by whitespace) from standard input
 * until end of file and sorts it with three worker threads:
 *   thread 0  sorts the first half of the list
 *   thread 1  sorts the second half of the list
 *   thread 2  merges the two sorted halves into a second array
 * The parent thread prints the original list, waits for the threads, then
 * prints each sorted half and the final sorted list.
 *
 * Build: gcc -Wall -o sort sort.c -pthread
 * Usage: ./sort < list.txt
 *        echo 7 12 19 3 18 4 2 6 15 8 | ./sort
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_THREADS 3

/* structure for passing data to threads */
typedef struct
{
    int start;  /* index of the first element */
    int end;    /* index one past the last element */
} parameters;

int *list;      /* the input list; each sorting thread sorts one half of it */
int *sorted;    /* the merging thread merges the two sorted halves into here */
int count;      /* number of integers in list (and in sorted) */

/* Sorting thread: sort list[start, end) in place with insertion sort. */
void *sorter(void *param)
{
    parameters *data = (parameters *) param;
    int i, j;

    for (i = data->start + 1; i < data->end; i++) {
        int value = list[i];

        for (j = i - 1; j >= data->start && list[j] > value; j--)
            list[j + 1] = list[j];
        list[j + 1] = value;
    }

    free(data);
    return NULL;
}

/* Merging thread: list[start, end) was split at its middle and each half was
 * sorted by a sorting thread; merge the two halves into sorted[start, end). */
void *merger(void *param)
{
    parameters *data = (parameters *) param;
    int middle = (data->start + data->end) / 2;
    int i = data->start;    /* next element of the first half */
    int j = middle;         /* next element of the second half */
    int k = data->start;    /* next free position in sorted */

    while (i < middle && j < data->end) {
        if (list[i] <= list[j])
            sorted[k++] = list[i++];
        else
            sorted[k++] = list[j++];
    }
    while (i < middle)
        sorted[k++] = list[i++];
    while (j < data->end)
        sorted[k++] = list[j++];

    free(data);
    return NULL;
}

void create_worker(pthread_t *tid, void *(*routine)(void *), int start, int end)
{
    parameters *data = (parameters *) malloc(sizeof(parameters));
    int err;

    if (data == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    data->start = start;
    data->end = end;

    err = pthread_create(tid, NULL, routine, data);
    if (err != 0) {
        fprintf(stderr, "pthread_create: %s\n", strerror(err));
        exit(EXIT_FAILURE);
    }
}

/* Read integers from standard input until end of file into list, enlarging
 * list as needed, and set count. */
void read_list(void)
{
    int capacity = 0;
    int value;
    int status;

    while ((status = scanf("%d", &value)) == 1) {
        if (count == capacity) {
            capacity = (capacity == 0) ? 16 : capacity * 2;
            list = (int *) realloc(list, capacity * sizeof(int));
            if (list == NULL) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
        }
        list[count++] = value;
    }

    if (status != EOF || count == 0) {
        fprintf(stderr,
                "Error: expected integers separated by whitespace on standard input\n");
        exit(EXIT_FAILURE);
    }
}

/* Print label followed by array[start, end) on one line. */
void print_list(const char *label, const int *array, int start, int end)
{
    int i;

    printf("%-17s", label);
    for (i = start; i < end; i++)
        printf(" %d", array[i]);
    printf("\n");
}

int main(void)
{
    pthread_t workers[NUM_THREADS];
    int middle;

    read_list();
    sorted = (int *) malloc(count * sizeof(int));
    if (sorted == NULL) {
        perror("malloc");
        return EXIT_FAILURE;
    }
    print_list("Original list:", list, 0, count);

    /* split the list in two; if count is odd, the second half is one longer */
    middle = count / 2;
    create_worker(&workers[0], sorter, 0, middle);
    create_worker(&workers[1], sorter, middle, count);
    pthread_join(workers[0], NULL);
    pthread_join(workers[1], NULL);

    /* the merging thread may start only after both halves are sorted */
    create_worker(&workers[2], merger, 0, count);
    pthread_join(workers[2], NULL);

    print_list("Sorting thread 0:", list, 0, middle);
    print_list("Sorting thread 1:", list, middle, count);
    print_list("Sorted list:", sorted, 0, count);

    free(list);
    free(sorted);
    return 0;
}
