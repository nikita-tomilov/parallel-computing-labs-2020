#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <float.h>
#include <pthread.h>
#include <sys/time.h>

#define MIN(a, b) (((a)<(b))?(a):(b))
#define MAX_I 50

int NUM_THREADS = 4;
int CHUNK_SIZE = 20;
int WORK_PARALLEL = 1;

void merge(double arr[], int l, int m, int r) {
    int n1 = m - l + 1;
    int n2 = r - m;
    double L[n1], R[n2];
    for (int i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (int j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];
    int i = 0;
    int j = 0;
    int k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}

void mergeSort(double arr[], int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);
        merge(arr, l, m, r);
    }
}

double custom_rand(double from, double to, unsigned int *seed) {
    int random_int;
    random_int = rand_r(seed); //random [0; RAND_MAX]
    double random_double = (double) (random_int) / RAND_MAX; //random [0.0; 1.0]
    random_double = (to - from) * random_double; //random [0.0; (to - from)]
    return from + random_double; //random [from; to]
}

void *printPercent(void *i) {
    while (*(int *) i < MAX_I) {
        printf("%d%% completed\n", 2 * *(int *) i);
        usleep(1000000);
    }
    return 0;
}

typedef struct {
    double *array1;
    double *array2;
    long offset;
    long count;
} Chunk_t;

typedef struct {
    int threadId;
    void *func;
} ThreadInfo_t;

pthread_mutex_t mutex;
double *array1;
double *array2;
long arrayLength;
long currentOffset;
long chunkSize;

double reductionResult;

double (*reductionFunction)(double, double);

Chunk_t getNextChunk(int threadId, double intermediateReductionResult) {
    pthread_mutex_lock(&mutex);
    if (reductionFunction != NULL) {
        reductionResult = (*reductionFunction)(reductionResult, intermediateReductionResult);
    }
    Chunk_t ans;
    ans.array1 = array1;
    ans.array2 = array2;
    if (currentOffset >= arrayLength) {
        ans.count = 0;
        ans.offset = 0;
    } else {
        ans.offset = currentOffset;
        ans.count = MIN(chunkSize, arrayLength - currentOffset);
    }
    currentOffset = currentOffset + chunkSize;
    //printf("thread %d requested next chunk, i said from %ld with size %ld\n", threadId, ans.offset, ans.count);
    pthread_mutex_unlock(&mutex);
    return ans;
}

void threadFunc(void *args) {
    ThreadInfo_t info = *(ThreadInfo_t *) args;
    Chunk_t chunk = getNextChunk(info.threadId, NAN);
    double (*f)(Chunk_t);
    f = info.func;
    while (chunk.count > 0) {
        double intermediateRes = f(chunk);
        chunk = getNextChunk(info.threadId, intermediateRes);
    }
}

void multiThreadComputingReduction(double _array1[], double _array2[], long _arrayLength, int threadsNum, void *func,
                                   void *_reductionFunction, double reductionInitValue, int _chunkSize) {
    pthread_t *thread = (pthread_t *) malloc(threadsNum * sizeof(pthread_t)); //создаем потоки
    ThreadInfo_t *info = (ThreadInfo_t *) malloc(threadsNum * sizeof(ThreadInfo_t)); //создаем инфо

    array1 = _array1;
    array2 = _array2;
    arrayLength = _arrayLength;
    chunkSize = _chunkSize;
    currentOffset = 0;
    reductionResult = reductionInitValue;
    reductionFunction = _reductionFunction;

    for (int j = 0; j < threadsNum; j++) {
        info[j].threadId = j + 1;
        info[j].func = func;
        pthread_create(&thread[j], NULL, (void *) threadFunc, (void *) (info + j));
    }

    for (int i = 0; i < threadsNum; i++) {
        pthread_join(thread[i], NULL);
    }
    free(thread);
    free(info);
}

void multiThreadComputing(double _array1[], double _array2[], long _arrayLength, int threadsNum, void *func,
                          int _chunkSize) {
    multiThreadComputingReduction(_array1, _array2, _arrayLength, threadsNum, func,
                                  NULL, NAN, _chunkSize);
}

void M1Map(Chunk_t chunk) {
    double *array = chunk.array1;
    long N = chunk.offset + chunk.count;
    //printf("m1map over %p from %ld len %ld\n", array1, chunk.offset, chunk.count);
    for (long j = chunk.offset; j < N; j++) {//Кубический корень после деления на число e
        array[j] = pow((double) (array[j] / exp(1.0)), 1.0 / 3.0);
    }
}

void M2Map(Chunk_t chunk) {
    double *array = chunk.array1;
    double *array_copy = chunk.array2;
    long N = chunk.offset + chunk.count;
    //printf("m2map over %p from %ld len %ld\n", array1, chunk.offset, chunk.count);
    for (long j = chunk.offset; j < N; j++) {
        double sum = array_copy[j]; //для нач элемента массива предыдущий элемент равен0
        if (j > 0) {
            sum += array_copy[j - 1]; //каждый элемент поочерёдно сложить с предыдущим
        }
        array[j] = pow(log10(sum), exp(1.0));
    }
}

void Merge(Chunk_t chunk) {
    double *M2 = chunk.array1;
    double *M1 = chunk.array2;
    long N = chunk.offset + chunk.count;
    //printf("m2map over %p from %ld len %ld\n", array1, chunk.offset, chunk.count);
    for (long j = chunk.offset; j < N; j++) {
        M2[j] = fabs(M1[j] - M2[j]);
    }
}

void InsertionSortChunk(Chunk_t chunk) {
    long from = chunk.offset;
    long to = chunk.offset + chunk.count;
    double *M2 = chunk.array1;
    long location;
    double elem;
    for (long p = from + 1; p < to; p++) {
        elem = M2[p];
        location = p - 1;
        while (location >= from && M2[location] > elem) {
            M2[location + 1] = M2[location];
            location = location - 1;
        }
        M2[location + 1] = elem;
    }
}

double reductionMin(double a, double b) {
    if (isnan(b)) return a;
    return MIN(a, b);
}

double reductionSum(double a, double b) {
    if (isnan(b)) return a;
    return a + b;
}

double MinNotZero(Chunk_t chunk) {
    double minNotZero = DBL_MAX;
    double *M2 = chunk.array1;
    long N = chunk.offset + chunk.count;
    for (long j = chunk.offset; j < N; j++) { //ищим минимальный ненулевой элемент массива М2
        if (M2[j] > 0 && M2[j] < minNotZero)
            minNotZero = M2[j];
    }
    return minNotZero;
}

double SumOfSine(Chunk_t chunk) {
    double X = 0;
    double *M2 = chunk.array1;
    double minNotZero = chunk.array2[0];
    //printf("min not zero: %lf\n", minNotZero);
    long N = chunk.offset + chunk.count;
    for (long j = chunk.offset; j < N; j++) { //считаем сумму синусов
        if (((long) (M2[j] / minNotZero)) % 2 == 0)
            X += sin(M2[j]);
    }
    return X;
}

Chunk_t fullChunk(double *_array1, double *_array2, long size) {
    Chunk_t chunk;
    chunk.array1 = _array1;
    chunk.array2 = _array2;
    chunk.count = size;
    chunk.offset = 0;
    return chunk;
}

struct timeval Tstart, Tcur;

void time_meas_start() {
    gettimeofday(&Tstart, NULL);
}

long get_time_us() {
    gettimeofday(&Tcur, NULL);
    long ans = 1000000 * (Tcur.tv_sec - Tstart.tv_sec) + (Tcur.tv_usec - Tstart.tv_usec);
    time_meas_start();
    return ans;
}

int main(int argc, char *argv[]) {
    if ((argc < 2) || (argc > 5)) {
        printf("usage: %s N [ thread_count [ chunk_size ] ]\n", argv[0]);
        return -1;
    }
    int i = 0;
    long delta_ms;
    long delta_generate_stage_us = 0;
    long delta_map_stage_us = 0;
    long delta_merge_stage_us = 0;
    long delta_sort_stage_us = 0;
    long delta_reduce_stage_us = 0;
    int N;

    struct timeval T1, T2;
    gettimeofday(&T1, NULL); /* запомнить текущее время T1 */
    pthread_t thread;
    pthread_create(&thread, NULL, printPercent, &i);

    int j;
    char *end;
    N = (int) strtol(argv[1], &end, 10); /* N равен первому параметру командной строки */
    int M2_size = N / 2;
    double *M1 = (double *) malloc(N * sizeof(double));
    double *M2 = (double *) malloc(M2_size * sizeof(double));
    double *M2_copy = (double *) malloc((M2_size) * sizeof(double));
    unsigned int A = (unsigned int) 240;

    if (argc > 2) {
        NUM_THREADS = (int) strtol(argv[2], &end, 10);
        if (NUM_THREADS == 0) {
            WORK_PARALLEL = 0;
        } else {
            if (argc > 3) {
                CHUNK_SIZE = (int) strtol(argv[3], &end, 10);
            }
        }
    }

    printf("mode Parallel=%d, NumThreads=%d, ChunkSize=%d\n", WORK_PARALLEL, NUM_THREADS, CHUNK_SIZE);

    for (i = 0; i < MAX_I; i++) /* 50 экспериментов */
    {
        /* Заполнить массив исходных данных размером N */
        //aka Этап Generate
        //TODO: parallel
        unsigned int seed = i;
        time_meas_start();
        for (j = 0; j < N; j++) {// генерим М1
            M1[j] = custom_rand(1.0, A, &seed);
        }

        for (j = 0; j < M2_size; j++) {// генерим М2 и его копию
            double rand = custom_rand(A, 10.0 * A, &seed);
            M2[j] = rand;
            M2_copy[j] = rand;
        }

        delta_generate_stage_us += get_time_us();

        /* Решить поставленную задачу, заполнить массив с результатами */
        if (WORK_PARALLEL) {
            //aka этап Map для M1
            multiThreadComputing(M1, NULL, N, NUM_THREADS, M1Map, CHUNK_SIZE);
            //этап Map для M2
            multiThreadComputing(M2, M2_copy, M2_size, NUM_THREADS, M2Map, CHUNK_SIZE);
        } else {
            M1Map(fullChunk(M1, NULL, N));
            M2Map(fullChunk(M2, M2_copy, M2_size));
        }

        delta_map_stage_us += get_time_us();

        //этап Merge
        if (WORK_PARALLEL) {
            multiThreadComputing(M2, M1, M2_size, NUM_THREADS, Merge, CHUNK_SIZE);
        } else {
            Merge(fullChunk(M2, M1, M2_size));
        }

        delta_merge_stage_us += get_time_us();

        /* Отсортировать массив с результатами указанным методом */
        //aka этап Sort
        if (WORK_PARALLEL) {
            multiThreadComputing(M2, NULL, M2_size, NUM_THREADS, InsertionSortChunk, M2_size / 24);
            //здесь у нас 6 (или k) кусков массивов, каждый из которых внутри себя отсортирован,
            //но сам массив еще нет. когда у нас массив частично сортирован, mergeSort - то что нужно
            mergeSort(M2, 0, M2_size - 1);
        } else {
            InsertionSortChunk(fullChunk(M2, M1, M2_size));
        }

        delta_sort_stage_us += get_time_us();

        /* Найти минимальный ненулевой элемент массива */
        //aka Reduce-1
        double minNotZero = DBL_MAX;
        if (WORK_PARALLEL) {
            multiThreadComputingReduction(M2, NULL, M2_size, NUM_THREADS, MinNotZero, reductionMin, DBL_MAX,
                                          CHUNK_SIZE);
            minNotZero = reductionResult;
        } else {
            minNotZero = MinNotZero(fullChunk(M2, NULL, M2_size));
        }

        /*Рассчитать сумму синусов тех элементов массива М2,
          которые при делении на минимальный ненулевой
          элемент массива М2 дают чётное число*/
        //aka Reduce-2
        double X;
        if (WORK_PARALLEL) {
            multiThreadComputingReduction(M2, &minNotZero, M2_size, NUM_THREADS, SumOfSine, reductionSum, 0,
                                          CHUNK_SIZE);
            X = reductionResult;
        } else {
            X = SumOfSine(fullChunk(M2, &minNotZero, M2_size));
        }
        delta_reduce_stage_us += get_time_us();
        printf("N=%d\t X=%.20f\n", i, X);
    }
    free(M1);
    free(M2);
    free(M2_copy);

    gettimeofday(&T2, NULL);   /* запомнить текущее время T2 */
    delta_ms = 1000 * (T2.tv_sec - T1.tv_sec) + (T2.tv_usec - T1.tv_usec) / 1000;
    pthread_join(thread, NULL);


    printf("100%% completed\n");
    printf("\nN=%d. Milliseconds passed: %ld\n", N, delta_ms); /* T2 - T1 */
    printf("Generate:%ld\n", delta_generate_stage_us);
    printf("Map:%ld\n", delta_map_stage_us);
    printf("Merge:%ld\n", delta_merge_stage_us);
    printf("Sort:%ld\n", delta_sort_stage_us);
    printf("Reduce:%ld\n", delta_reduce_stage_us);
    return 0;
}