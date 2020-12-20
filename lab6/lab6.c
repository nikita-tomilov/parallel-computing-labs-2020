#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <CL/cl.h>
#include <string.h>
#include <stddef.h>
#include <math.h>
#include <float.h>
#include <pthread.h>

#include <sys/time.h>

#define _GNU_SOURCE
#define MIN(a, b) (((a)<(b))?(a):(b))
#define MAX_I 50
#define CL_TARGET_OPENCL_VERSION 120
#define __CL_ENABLE_EXCEPTIONS


cl_uint platformCount;
cl_uint ret;
cl_device_id device;
cl_uint uNumCPU;

void createProgram(char *fileName, char *source_str, size_t source_size);

void createProgram_sin(char *fileName);

cl_mem M2_clmem_sin;
cl_mem M2_ret_clmem_sin;
cl_kernel kernel_sin;
cl_command_queue queue_sin;
cl_context context_sin;
cl_float *puData_sin;
cl_program program_sin;
char *source_str_sin;
size_t source_size_sin;
char *source_str_sin;
size_t source_size_sin;

cl_mem M1_clmem_mrg;
cl_mem M1_ret_clmem_mrg;
cl_mem M2_clmem_mrg;
cl_mem M2_ret_clmem_mrg;
cl_kernel kernel_mrg;
cl_command_queue queue_mrg;
cl_context context_mrg;
cl_float *puData_mrg;
cl_program program_mrg;
char *source_str_mrg;
size_t source_size_mrg;
char *source_str_mrg;
size_t source_size_mrg;

//const int num_cores = CL_DEVICE_MAX_COMPUTE_UNITS;
const int num_cores = 12;

void cl_sin_init(int M2_size);

void cl_sin_destroy();

float sum_sin(float *M2, int M2_size, float min_non_zero);

void mergecl(float *M1, float *M2, int M2_size);

void cl_mrg_init(int M2_size);

void cl_mrg_destroy();

int NUM_THREADS = 4;
int CHUNK_SIZE = 20;
int WORK_PARALLEL = 1;

void merge(float arr[], int l, int m, int r) {
    int n1 = m - l + 1;
    int n2 = r - m;
    float L[n1], R[n2];
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

void mergeSort(float arr[], int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);
        merge(arr, l, m, r);
    }
}

float custom_rand(float from, float to, unsigned int *seed) {
    int random_int;
    random_int = rand_r(seed); //random [0; RAND_MAX]
    float random_float = (float) (random_int) / RAND_MAX; //random [0.0; 1.0]
    random_float = (to - from) * random_float; //random [0.0; (to - from)]
    return from + random_float; //random [from; to]
}

void *printPercent(void *i) {
    while (*(int *) i < MAX_I) {
        printf("%d%% completed\n", 2 * *(int *) i);
        usleep(1000000);
    }
    return 0;
}

typedef struct {
    float *array1;
    float *array2;
    long offset;
    long count;
} Chunk_t;

typedef struct {
    int threadId;
    void *func;
} ThreadInfo_t;

pthread_mutex_t mutex;
float *array1;
float *array2;
long arrayLength;
long currentOffset;
long chunkSize;

float reductionResult;

float (*reductionFunction)(float, float);

Chunk_t getNextChunk(int threadId, float intermediateReductionResult) {
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
    float (*f)(Chunk_t);
    f = info.func;
    while (chunk.count > 0) {
        float intermediateRes = f(chunk);
        chunk = getNextChunk(info.threadId, intermediateRes);
    }
}

void multiThreadComputingReduction(float _array1[], float _array2[], long _arrayLength, int threadsNum, void *func,
                                   void *_reductionFunction, float reductionInitValue, int _chunkSize) {
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

void multiThreadComputing(float _array1[], float _array2[], long _arrayLength, int threadsNum, void *func,
                          int _chunkSize) {
    multiThreadComputingReduction(_array1, _array2, _arrayLength, threadsNum, func,
                                  NULL, NAN, _chunkSize);
}

void M1Map(Chunk_t chunk) {
    float *array = chunk.array1;
    long N = chunk.offset + chunk.count;
    //printf("m1map over %p from %ld len %ld\n", array1, chunk.offset, chunk.count);
    for (long j = chunk.offset; j < N; j++) {//Кубический корень после деления на число e
        array[j] = pow((float) (array[j] / exp(1.0)), 1.0 / 3.0);
    }
}

void M2Map(Chunk_t chunk) {
    float *array = chunk.array1;
    float *array_copy = chunk.array2;
    long N = chunk.offset + chunk.count;
    //printf("m2map over %p from %ld len %ld\n", array1, chunk.offset, chunk.count);
    for (long j = chunk.offset; j < N; j++) {
        float sum = array_copy[j]; //для нач элемента массива предыдущий элемент равен0
        if (j > 0) {
            sum += array_copy[j - 1]; //каждый элемент поочерёдно сложить с предыдущим
        }
        array[j] = pow(log10(sum), exp(1.0));
    }
}

void Merge(Chunk_t chunk) {
    float *M2 = chunk.array1;
    float *M1 = chunk.array2;
    long N = chunk.offset + chunk.count;
    //printf("m2map over %p from %ld len %ld\n", array1, chunk.offset, chunk.count);
    for (long j = chunk.offset; j < N; j++) {
        M2[j] = fabs(M1[j] - M2[j]);
    }
}

void InsertionSortChunk(Chunk_t chunk) {
    long from = chunk.offset;
    long to = chunk.offset + chunk.count;
    float *M2 = chunk.array1;
    long location;
    float elem;
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

float reductionMin(float a, float b) {
    if (isnan(b)) return a;
    return MIN(a, b);
}

float reductionSum(float a, float b) {
    if (isnan(b)) return a;
    return a + b;
}

float MinNotZero(Chunk_t chunk) {
    float minNotZero = DBL_MAX;
    float *M2 = chunk.array1;
    long N = chunk.offset + chunk.count;
    for (long j = chunk.offset; j < N; j++) { //ищим минимальный ненулевой элемент массива М2
        if (M2[j] > 0 && M2[j] < minNotZero)
            minNotZero = M2[j];
    }
    return minNotZero;
}

float SumOfSine(Chunk_t chunk) {
    float X = 0;
    float *M2 = chunk.array1;
    float minNotZero = chunk.array2[0];
    //printf("min not zero: %lf\n", minNotZero);
    long N = chunk.offset + chunk.count;
    for (long j = chunk.offset; j < N; j++) { //считаем сумму синусов
        if (((long) (M2[j] / minNotZero)) % 2 == 0)
            X += sin(M2[j]);
    }
    return X;
}

Chunk_t fullChunk(float *_array1, float *_array2, long size) {
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

unsigned int gSeed;

float gA;

void GenerateM1(Chunk_t chunk) {
    long N = chunk.offset + chunk.count;
    float *M1 = chunk.array1;
    for (long j = chunk.offset; j < N; j++) { //ищим минимальный ненулевой элемент массива М2
        M1[j] = custom_rand(1.0, gA, &gSeed);
    }
}

void GenerateM2(Chunk_t chunk) {
    long N = chunk.offset + chunk.count;
    float *M2 = chunk.array1;
    float *M2_copy = chunk.array2;
    for (long j = chunk.offset; j < N; j++) { //ищим минимальный ненулевой элемент массива М2
        float rand = custom_rand(gA, 10.0 * gA, &gSeed);
        M2[j] = rand;
        M2_copy[j] = rand;
    }
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

    pthread_mutex_init(&mutex, NULL);

    struct timeval T1, T2;
    gettimeofday(&T1, NULL); /* запомнить текущее время T1 */
    pthread_t thread;
    pthread_create(&thread, NULL, printPercent, &i);

    char *end;
    N = (int) strtol(argv[1], &end, 10); /* N равен первому параметру командной строки */
    int M2_size = N / 2;
    float *M1 = (float *) malloc(N * sizeof(float));
    float *M2 = (float *) malloc(M2_size * sizeof(float));
    float *M2_copy = (float *) malloc((M2_size) * sizeof(float));
    unsigned int A = (unsigned int) 240;

    cl_sin_init(M2_size);//openCL init
    cl_mrg_init(M2_size);

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
        time_meas_start();
        gA = A;
        gSeed = i;
        //Параллельная версия Generate будет не совпадать с ЛР1, т к порядок вызова rand_r неизвестен заранее
        if (WORK_PARALLEL) {
            multiThreadComputing(M1, NULL, N, NUM_THREADS, GenerateM1, CHUNK_SIZE);
            multiThreadComputing(M2, M2_copy, M2_size, NUM_THREADS, GenerateM2, CHUNK_SIZE);
        } else {
            GenerateM1(fullChunk(M1, NULL, N));
            GenerateM2(fullChunk(M2, M2_copy, M2_size));
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
        /*if (WORK_PARALLEL) {
            multiThreadComputing(M2, M1, M2_size, NUM_THREADS, Merge, CHUNK_SIZE);
        } else {
            Merge(fullChunk(M2, M1, M2_size));
        }*/
        //OpenCL
        mergecl(M1, M2, M2_size);
        delta_merge_stage_us += get_time_us();

        /* Отсортировать массив с результатами указанным методом */
        //aka этап Sort
        if (WORK_PARALLEL) {
            multiThreadComputing(M2, NULL, M2_size, NUM_THREADS, InsertionSortChunk, (int) CHUNK_SIZE);
            //здесь у нас 6 (или k) кусков массивов, каждый из которых внутри себя отсортирован,
            //но сам массив еще нет. когда у нас массив частично сортирован, mergeSort - то что нужно
            mergeSort(M2, 0, M2_size - 1);
        } else {
            InsertionSortChunk(fullChunk(M2, M1, M2_size));
        }

        delta_sort_stage_us += get_time_us();

        /* Найти минимальный ненулевой элемент массива */
        //aka Reduce-1
        float minNotZero = DBL_MAX;
        if (WORK_PARALLEL) {
            multiThreadComputingReduction(M2, NULL, M2_size, NUM_THREADS, MinNotZero, reductionMin, DBL_MAX,
                                          CHUNK_SIZE);
            minNotZero = reductionResult;
        } else {
            minNotZero = MinNotZero(fullChunk(M2, NULL, M2_size));
        }
        //printf("%f\n", minNotZero);

        /*Рассчитать сумму синусов тех элементов массива М2,
          которые при делении на минимальный ненулевой
          элемент массива М2 дают чётное число*/
        //aka Reduce-2
        float X;
        /*if (WORK_PARALLEL) {
            multiThreadComputingReduction(M2, &minNotZero, M2_size, NUM_THREADS, SumOfSine, reductionSum, 0,
                                          CHUNK_SIZE);
            X = reductionResult;
        } else {
            X = SumOfSine(fullChunk(M2, &minNotZero, M2_size));
        }*/
        //OpenCL
        X = sum_sin(M2, M2_size, minNotZero);
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

    pthread_mutex_destroy(&mutex);
    cl_sin_destroy();
    cl_mrg_destroy();
    return 0;
}

void createProgram_sin(char *fileName) {
    FILE *program_handle;
    program_handle = fopen(fileName, "r");
    if (program_handle == NULL) {
        perror("Couldn't find the program file");
        exit(1);
    }
    fseek(program_handle, 0, SEEK_END);
    source_size_sin = ftell(program_handle);
    rewind(program_handle);
    source_str_sin = (char *) malloc(source_size_sin + 1);
    source_str_sin[source_size_sin] = '\0';
    unsigned long n = fread(source_str_sin, sizeof(char), source_size_sin, program_handle);
    if (n < 0) {
        printf("error in reading file %s: %lu\n", fileName, n);
    }
    fclose(program_handle);
}

void createProgram_mrg(char *fileName) {
    FILE *program_handle;
    program_handle = fopen(fileName, "r");
    if (program_handle == NULL) {
        perror("Couldn't find the program file");
        exit(1);
    }
    fseek(program_handle, 0, SEEK_END);
    source_size_mrg = ftell(program_handle);
    rewind(program_handle);
    source_str_mrg = (char *) malloc(source_size_mrg + 1);
    source_str_mrg[source_size_mrg] = '\0';
    unsigned long n = fread(source_str_mrg, sizeof(char), source_size_mrg, program_handle);
    if (n < 0) {
        printf("error in reading file %s: %lu\n", fileName, n);
    }
    fclose(program_handle);
}

void mergecl(float *M1, float *M2, int M2_size) {

    ret = clEnqueueWriteBuffer(queue_mrg, M1_clmem_mrg, CL_TRUE, 0, M2_size * sizeof(float), M1, 0, NULL, NULL);
    ret = clEnqueueWriteBuffer(queue_mrg, M2_clmem_mrg, CL_TRUE, 0, M2_size * sizeof(float), M2, 0, NULL, NULL);

    clSetKernelArg(kernel_mrg, 0, sizeof(M1_clmem_mrg), (void *) &M1_clmem_mrg);
    clSetKernelArg(kernel_mrg, 1, sizeof(M2_clmem_mrg), (void *) &M2_clmem_mrg);
    clSetKernelArg(kernel_mrg, 2, sizeof(M2_ret_clmem_mrg), (void *) &M2_ret_clmem_mrg);

    size_t uGlobalWorkSize = M2_size;
    clEnqueueNDRangeKernel(
            queue_mrg,
            kernel_mrg,
            1,
            NULL,
            &uGlobalWorkSize,
            NULL,
            0,
            NULL,
            NULL);
    clFinish(queue_mrg);

    for (int i = 0; i < M2_size; ++i)
        M2[i] = puData_mrg[i];

}

float sum_sin(float *M2, int M2_size, float min_non_zero) {
    ret = clEnqueueWriteBuffer(queue_sin, M2_clmem_sin, CL_TRUE, 0, M2_size * sizeof(float), M2, 0, NULL, NULL);

    clSetKernelArg(kernel_sin, 0, sizeof(float), (void *) &min_non_zero);
    clSetKernelArg(kernel_sin, 1, sizeof(M2_clmem_sin), (void *) &M2_clmem_sin);
    clSetKernelArg(kernel_sin, 2, sizeof(M2_ret_clmem_sin), (void *) &M2_ret_clmem_sin);

    size_t uGlobalWorkSize = M2_size;
    size_t uLocalWorkSize = num_cores;
    clEnqueueNDRangeKernel(
            queue_sin,
            kernel_sin,
            1,
            NULL,
            &uGlobalWorkSize,
            &uLocalWorkSize,
            0,
            NULL,
            NULL);
    clFinish(queue_sin);

    float X = 0;
    for (int i = 0; i < M2_size / num_cores; ++i)
        X += puData_sin[i];
    return X;
}

void cl_sin_init(int M2_size) {
    cl_platform_id platform;
    cl_int ret = clGetPlatformIDs(1, &platform, &platformCount);

    ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, &uNumCPU);

    context_sin = clCreateContext(NULL, 1, &device, NULL, NULL, &ret);

    queue_sin = clCreateCommandQueue(context_sin, device, 0, &ret);

    createProgram_sin("/home/hotaro/ifmo/parallel-computing/lab6/sum_sin.cl");
    program_sin = clCreateProgramWithSource(context_sin, 1, (const char **) &source_str_sin,
                                            (const size_t *) &source_size_sin, &ret);

    ret = clBuildProgram(program_sin, 1, &device, "-cl-std=CL1.2", NULL, NULL);
    printf("clBuildProgram ret %d\n", ret);
    if (ret == CL_BUILD_PROGRAM_FAILURE) {
        size_t log_size;
        clGetProgramBuildInfo(program_sin, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char *) malloc(log_size);
        clGetProgramBuildInfo(program_sin, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("%s\n", log);
    }

    kernel_sin = clCreateKernel(program_sin, "sum_sin", &ret);

    M2_clmem_sin = clCreateBuffer(context_sin, CL_MEM_READ_ONLY, M2_size * sizeof(float), NULL, &ret);
    M2_ret_clmem_sin = clCreateBuffer(context_sin, CL_MEM_WRITE_ONLY, M2_size * sizeof(float), NULL, &ret);

    puData_sin = (cl_float *) clEnqueueMapBuffer(
            queue_sin,
            M2_ret_clmem_sin,
            CL_TRUE,
            CL_MAP_READ,
            0,
            M2_size / num_cores * sizeof(cl_float),
            0,
            NULL,
            NULL,
            NULL);
}

void cl_mrg_init(int M2_size) {
    cl_platform_id platform;
    cl_int ret = clGetPlatformIDs(1, &platform, &platformCount);

    ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, &uNumCPU);

    context_mrg = clCreateContext(NULL, 1, &device, NULL, NULL, &ret);

    queue_mrg = clCreateCommandQueue(context_mrg, device, 0, &ret);

    createProgram_mrg("/home/hotaro/ifmo/parallel-computing/lab6/stage_merge.cl");
    program_mrg = clCreateProgramWithSource(context_mrg, 1, (const char **) &source_str_mrg,
                                            (const size_t *) &source_size_mrg, &ret);

    ret = clBuildProgram(program_mrg, 1, &device, "-cl-std=CL1.2", NULL, NULL);
    if (ret == CL_BUILD_PROGRAM_FAILURE) {
        size_t log_size;
        clGetProgramBuildInfo(program_mrg, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char *) malloc(log_size);
        clGetProgramBuildInfo(program_mrg, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("%s\n", log);
    }

    kernel_mrg = clCreateKernel(program_mrg, "merge", &ret);

    M1_clmem_mrg = clCreateBuffer(context_mrg, CL_MEM_READ_ONLY, M2_size * sizeof(float), NULL, &ret);
    M2_clmem_mrg = clCreateBuffer(context_mrg, CL_MEM_READ_ONLY, M2_size * sizeof(float), NULL, &ret);
    M2_ret_clmem_mrg = clCreateBuffer(context_mrg, CL_MEM_WRITE_ONLY, M2_size * sizeof(float), NULL, &ret);

    puData_mrg = (cl_float *) clEnqueueMapBuffer(
            queue_mrg,
            M2_ret_clmem_mrg,
            CL_TRUE,
            CL_MAP_READ,
            0,
            M2_size * sizeof(cl_float),
            0,
            NULL,
            NULL,
            NULL);
}

void cl_sin_destroy() {
    clEnqueueUnmapMemObject(queue_sin, M2_ret_clmem_sin, puData_sin, 0, NULL, NULL);
    clReleaseContext(context_sin);
    clReleaseCommandQueue(queue_sin);
    clReleaseProgram(program_sin);
    free(source_str_sin);
    clReleaseKernel(kernel_sin);
}

void cl_mrg_destroy() {
    clEnqueueUnmapMemObject(queue_mrg, M2_ret_clmem_mrg, puData_mrg, 0, NULL, NULL);
    clReleaseContext(context_mrg);
    clReleaseCommandQueue(queue_mrg);
    clReleaseProgram(program_mrg);
    free(source_str_mrg);
    clReleaseKernel(kernel_mrg);
}