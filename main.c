#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

#define MAX_SIZE 100000000
#define MAX_WORKERS 100
#define MAX_PIVOTS 20

struct Pivot
{
    int index;
    int value;
};

const int g_sortCutoff = 100;
int g_maxSize;
int g_arrayData[MAX_SIZE];
int g_maxWorkers;
pthread_t workers[MAX_WORKERS];

struct WorkerData
{
    int id;
    int *start;
    int n;
    int size;
};

struct WorkerData g_workerData[MAX_WORKERS];
pthread_attr_t g_attr;

int g_activeWorkers = 0;
pthread_mutex_t g_lock;

double g_startTime;
double g_finalTime;

/* function prototypes */
void initWorkerData();
void generate(int *start, int *end);
bool isSorted(int *start, int *end);
int compare(const void *a, const void *b);
void *startThread(void *data);
void parallelQuicksort(int *start, int n, int size);
int getPivot(int *start, int n);
int comparePivot(const void *a, const void *b);
void swap(int *a, int *b);
void printArray(int *start, int *end);
double readTimer();

int main(int argc, const char *argv[])
{
    g_maxSize = 10000000;
    g_maxWorkers = 8;

    if (g_maxSize > MAX_SIZE)
        g_maxSize = MAX_SIZE;
    if (g_maxWorkers > MAX_WORKERS)
        g_maxWorkers = MAX_WORKERS;

    initWorkerData();
    generate(&g_arrayData[0], &g_arrayData[g_maxSize]);

    pthread_attr_init(&g_attr);
    pthread_attr_setscope(&g_attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setdetachstate(&g_attr, PTHREAD_CREATE_JOINABLE);

    pthread_mutex_init(&g_lock, NULL);

    g_startTime = readTimer();
    parallelQuicksort(&g_arrayData[0], g_maxSize, sizeof(int));
    g_finalTime = readTimer() - g_startTime;

    if (isSorted(&g_arrayData[0], &g_arrayData[g_maxSize]))
    {
        printf("Array is sorted\n");
    }
    else
    {
        printf("Array is not sorted\n");
    }

    printf("The execution time is %g sec\n", g_finalTime);
    return 0;
}

void initWorkerData()
{
    for (int i = 0; i < g_maxWorkers; i++)
    {
        g_workerData[i].id = 0;
        g_workerData[i].start = 0;
        g_workerData[i].n = 0;
        g_workerData[i].size = 0;
    }
}

void generate(int *start, int *end)
{
    while (start != end)
    {
        *start = rand();
        start++;
    }
}

bool isSorted(int *start, int *end)
{
    start++;
    while (start != end)
    {
        if (*(start - 1) > *start)
            return false;
        start++;
    }
    return true;
}

int compare(const void *a, const void *b)
{
    return (*(int *)a - *(int *)b);
}

void *startThread(void *data)
{
    struct WorkerData *p = (struct WorkerData *)data;
    parallelQuicksort(p->start, p->n, p->size);
    pthread_exit(0);
}

void parallelQuicksort(int *start, int n, int size)
{
    if (n < g_sortCutoff)
    {
        qsort(start, n, sizeof(int), compare);
        return;
    }

    pthread_mutex_lock(&g_lock);
    if (g_activeWorkers < g_maxWorkers)
    {
        int worker_index = g_activeWorkers++;
        pthread_mutex_unlock(&g_lock);

        int pivotIndex = getPivot(start, n);
        int right = n - 1;
        swap(&start[pivotIndex], &start[right]);

        int storeIndex = 0;
        for (int i = 0; i < right; i++)
        {
            if (start[i] < start[right])
            {
                swap(&start[i], &start[storeIndex]);
                storeIndex++;
            }
        }
        swap(&start[storeIndex], &start[right]);
        pivotIndex = storeIndex;

        g_workerData[worker_index].id = worker_index;
        g_workerData[worker_index].start = start + pivotIndex;
        g_workerData[worker_index].n = n - pivotIndex;
        g_workerData[worker_index].size = size;

        pthread_create(&workers[worker_index], &g_attr, startThread, (void *)&g_workerData[worker_index]);

        parallelQuicksort(start, pivotIndex, size);

        void *status;
        int rc = pthread_join(workers[worker_index], &status);
        if (rc)
        {
            printf("ERROR; return code from pthread_join() is %d\n", rc);
            exit(-1);
        }
    }
    else
    {
        pthread_mutex_unlock(&g_lock);
        qsort(start, n, sizeof(int), compare);
    }
}

int getPivot(int *start, int n)
{
    if (n < 2)
        return 0;
    struct Pivot pivots[MAX_PIVOTS];
    int maxPivots = (MAX_PIVOTS > n) ? n : MAX_PIVOTS;

    for (int i = 0; i < maxPivots; i++)
    {
        int index = rand() % n;
        pivots[i].index = index;
        pivots[i].value = start[index];
    }

    qsort(&pivots[0], maxPivots, sizeof(struct Pivot), comparePivot);
    return pivots[maxPivots / 2].index;
}

int comparePivot(const void *a, const void *b)
{
    return (((struct Pivot *)a)->value - ((struct Pivot *)b)->value);
}

void swap(int *a, int *b)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void printArray(int *start, int *end)
{
    while (start != end)
    {
        printf("%d ", *start);
        start++;
    }
    printf("\n");
}

double readTimer()
{
    static bool initialized = false;
    static struct timeval start;
    struct timeval end;
    if (!initialized)
    {
        gettimeofday(&start, NULL);
        initialized = true;
    }
    gettimeofday(&end, NULL);
    return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}