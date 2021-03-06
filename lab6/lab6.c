#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#define GENERATE_PARALLEL 1

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

#define DEBUG 0
#define debug_print(fmt, ...) \
            do { if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

void debug_print_ret_code(char *func, int ret) {
    if (ret < 0) {
        printf("WARNING: %s returned %d\n", func, ret);
    } else {
        debug_print("%s returned %d\n", func, ret);
    }
}

#define _GNU_SOURCE
#define MIN(a, b) (((a)<(b))?(a):(b))
#define MAX_I 50
#define CL_TARGET_OPENCL_VERSION 120
#define __CL_ENABLE_EXCEPTIONS

int NUM_THREADS = 4;
int CHUNK_SIZE = 20;
int WORK_PARALLEL = 1;
int WORK_OPENCL = 1;

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

long profiling_time_counter = 0;

long get_profiling_time_us() {
    long tmp = profiling_time_counter;
    profiling_time_counter = 0;
    return tmp;
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

size_t source_size;
char *source_str;

cl_uint platformCount;
cl_device_id device;
cl_uint uNumCPU;
cl_context context;

cl_program program;

void createProgram(char *fileName) {
    FILE *program_handle;
    program_handle = fopen(fileName, "r");
    if (program_handle == NULL) {
        perror("Couldn't find the program file");
        exit(1);
    }
    fseek(program_handle, 0, SEEK_END);
    source_size = ftell(program_handle);
    rewind(program_handle);
    source_str = (char *) malloc(source_size + 1);
    source_str[source_size] = '\0';
    unsigned long n = fread(source_str, sizeof(char), source_size, program_handle);
    if (n < 0) {
        printf("error in reading %s: %lu\n", fileName, n);
    }
    fclose(program_handle);
}

void setupOpenCLContext() {
    cl_platform_id platform;
    cl_int ret = clGetPlatformIDs(1, &platform, &platformCount);
    debug_print_ret_code("clGetPlatformIDs", ret);
    debug_print("number platforms %d\n", platformCount);

    ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &uNumCPU);
    debug_print_ret_code("clGetDeviceIDs", ret);

    context = clCreateContext(NULL, 1, &device, NULL, NULL, &ret);
    debug_print_ret_code("clCreateContext", ret);
}

void buildKernelProgram() {
    cl_int ret;
    createProgram("kernel.cl");
    program = clCreateProgramWithSource(context, 1, (const char **) &source_str,
                                        (const size_t *) &source_size, &ret);
    debug_print_ret_code("clCreateProgramWithSource ret", ret);

    ret = clBuildProgram(program, 1, &device, "-cl-std=CL1.2", NULL, NULL);
    debug_print_ret_code("clBuildProgram", ret);
    if (ret == CL_BUILD_PROGRAM_FAILURE) {
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char *) malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("%s\n", log);
    }
    free(source_str);
}

void teardownOpenCL() {
    clReleaseContext(context);
    clReleaseProgram(program);
}

void RunWithOpenCL(char *func_name, float *M1, int M1_size, float *M2, int M2_size) {
    cl_int ret;

    cl_command_queue queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &ret);
    debug_print_ret_code("clCreateCommandQueue", ret);

    cl_kernel kernel = clCreateKernel(program, func_name, &ret);
    debug_print_ret_code("clCreateKernel", ret);

    cl_mem M1_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, M1_size * sizeof(float), NULL, &ret);
    debug_print_ret_code("clCreateBuffer", ret);
    cl_mem M2_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, M2_size * sizeof(float), NULL, &ret);
    debug_print_ret_code("clCreateBuffer M2", ret);
    cl_mem ret_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, M2_size * sizeof(float), NULL, &ret);
    debug_print_ret_code("clCreateBuffer M2_ret", ret);

    ret = clEnqueueWriteBuffer(queue, M1_clmem, CL_TRUE, 0, M1_size * sizeof(float), M1, 0, NULL, NULL);
    debug_print_ret_code("clEnqueueWriteBuffer M1_clmem", ret);
    ret = clEnqueueWriteBuffer(queue, M2_clmem, CL_TRUE, 0, M2_size * sizeof(float), M2, 0, NULL, NULL);
    debug_print_ret_code("clEnqueueWriteBuffer M2_clmem", ret);

    clSetKernelArg(kernel, 0, sizeof(M1_clmem), (void *) &M1_clmem);
    clSetKernelArg(kernel, 1, sizeof(M2_clmem), (void *) &M2_clmem);
    clSetKernelArg(kernel, 2, sizeof(ret_clmem), (void *) &ret_clmem);

    size_t uGlobalWorkSize = M2_size;
    size_t retMemSize = M2_size;
    cl_event event;
    clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &uGlobalWorkSize, NULL, 0, NULL, &event);
    clWaitForEvents(1, &event);
    clFinish(queue);

    cl_ulong start = 0, end = 0;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    cl_double execTimeUs = (cl_double)(end - start)*(cl_double)(1e-03);
    profiling_time_counter += execTimeUs;

    cl_float *puData = (cl_float *) clEnqueueMapBuffer(queue, ret_clmem, CL_TRUE, CL_MAP_READ, 0,
                                                       retMemSize * sizeof(cl_float), 0, NULL, NULL, NULL);

    memcpy(M2, puData, retMemSize * sizeof(float));

    clEnqueueUnmapMemObject(queue, ret_clmem, puData, 0, NULL, NULL);
    clReleaseMemObject(M1_clmem);
    clReleaseMemObject(M2_clmem);
    clReleaseMemObject(ret_clmem);
    clReleaseCommandQueue(queue);
    clReleaseKernel(kernel);
}

int main(int argc, char *argv[]) {
    if ((argc < 2) || (argc > 5)) {
        printf("usage: %s N [ thread_count [ chunk_size [ --noopencl ] ] ]\n", argv[0]);
        return -1;
    }
    int i = 0;
    long delta_ms;
    long delta_generate_stage_us = 0;
    long delta_map_stage_us = 0;
    long delta_map_stage_us_prof = 0;
    long delta_merge_stage_us = 0;
    long delta_merge_stage_us_prof = 0;
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

    if (argc > 2) {
        NUM_THREADS = (int) strtol(argv[2], &end, 10);
        if (NUM_THREADS == 0) {
            WORK_PARALLEL = 0;
        } else {
            if (argc > 3) {
                CHUNK_SIZE = (int) strtol(argv[3], &end, 10);
            }
            if (argc > 4) {
                WORK_OPENCL = 0;
            }
        }
    }

    printf("mode Parallel=%d, NumThreads=%d, ChunkSize=%d, OpenCL=%d\n", WORK_PARALLEL, NUM_THREADS, CHUNK_SIZE, WORK_OPENCL);

    if (WORK_PARALLEL) {
        setupOpenCLContext();
        buildKernelProgram();
    }

    for (i = 0; i < MAX_I; i++) /* 50 экспериментов */
    {
        /* Заполнить массив исходных данных размером N */
        //aka Этап Generate
        time_meas_start();
        gA = A;
        gSeed = i;
        //Параллельная версия Generate будет не совпадать с ЛР1, т к порядок вызова rand_r неизвестен заранее
        if (GENERATE_PARALLEL) {
            multiThreadComputing(M1, NULL, N, NUM_THREADS, GenerateM1, CHUNK_SIZE);
            multiThreadComputing(M2, M2_copy, M2_size, NUM_THREADS, GenerateM2, CHUNK_SIZE);
        } else {
            GenerateM1(fullChunk(M1, NULL, N));
            GenerateM2(fullChunk(M2, M2_copy, M2_size));
        }

        delta_generate_stage_us += get_time_us();

        /* Решить поставленную задачу, заполнить массив с результатами */
        if (WORK_PARALLEL) {
            if (WORK_OPENCL) {
                //aka этап Map для M1
                RunWithOpenCL("m1map", M1, N, M1, N);
                //этап Map для M2
                RunWithOpenCL("m2map", M2_copy, M2_size, M2, M2_size);
            } else {
                multiThreadComputing(M1, NULL, N, NUM_THREADS, M1Map, CHUNK_SIZE);
                multiThreadComputing(M2, M2_copy, M2_size, NUM_THREADS, M2Map, CHUNK_SIZE);
            }
        } else {
            M1Map(fullChunk(M1, NULL, N));
            M2Map(fullChunk(M2, M2_copy, M2_size));
        }

        delta_map_stage_us += get_time_us();
        delta_map_stage_us_prof += get_profiling_time_us();

        //этап Merge
        if (WORK_PARALLEL) {
            if (WORK_OPENCL) {
                //OpenCL
                RunWithOpenCL("merge", M1, M2_size, M2, M2_size);
            } else {
                multiThreadComputing(M2, M1, M2_size, NUM_THREADS, Merge, CHUNK_SIZE);
            }
        } else {
            Merge(fullChunk(M2, M1, M2_size));
        }

        delta_merge_stage_us += get_time_us();
        delta_merge_stage_us_prof += get_profiling_time_us();

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
        float minNotZero = FLT_MAX;
        if (WORK_PARALLEL) {
            multiThreadComputingReduction(M2, NULL, M2_size, NUM_THREADS, MinNotZero, reductionMin, DBL_MAX,
                                          CHUNK_SIZE);
            minNotZero = reductionResult;
        } else {
            minNotZero = MinNotZero(fullChunk(M2, NULL, M2_size));
        }
        //printf("for i = %d minNotZero = %f\n", i, minNotZero);

        /*Рассчитать сумму синусов тех элементов массива М2,
          которые при делении на минимальный ненулевой
          элемент массива М2 дают чётное число*/
        //aka Reduce-2
        float X = 0;
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
    printf("Prof map:%ld\n", delta_map_stage_us_prof);
    printf("Merge:%ld\n", delta_merge_stage_us);
    printf("Prof merge:%ld\n", delta_merge_stage_us_prof);
    printf("Sort:%ld\n", delta_sort_stage_us);
    printf("Reduce:%ld\n", delta_reduce_stage_us);

    pthread_mutex_destroy(&mutex);

    if (WORK_PARALLEL) {
        teardownOpenCL();
    }
    return 0;
}