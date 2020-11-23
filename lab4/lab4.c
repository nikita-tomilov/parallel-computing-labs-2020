#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>
#include <float.h>

#define MIN(a, b) (((a)<(b))?(a):(b))

#define SCHEDULE_STRING
#define CONST_NUM_THREADS 4
#define MAX_I 50

#include <sys/time.h>

#ifdef _OPENMP

#include "omp.h"

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

#else

#include <pthread.h>

int omp_set_num_threads(int M) { return 1; }

int omp_get_num_threads() { return 1; }

int omp_get_thread_num() { return 0; }

int omp_get_num_procs() { return 0; }

void omp_set_nested() {}

#endif

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

int main(int argc, char *argv[]) {
    int i = 0;
    long delta_ms;
	long delta_map_stage_ms = 0;
	long delta_merge_stage_ms = 0;
	long delta_sort_stage_ms = 0;
	long delta_reduce_stage_ms = 0;
	struct timeval T3, T4;
    int N;
#ifdef _OPENMP
    double start = omp_get_wtime();
    omp_set_nested(1);

        {
#else
            struct timeval T1, T2;
            gettimeofday(&T1, NULL); /* запомнить текущее время T1 */
            pthread_t thread;
            pthread_create(&thread, NULL, printPercent, &i);
#endif

            int j;
            N = atoi(argv[1]); /* N равен первому параметру командной строки */
            int M2_size = N / 2;
            double *M1 = (double *) malloc(N * sizeof(double));
            double *M2 = (double *) malloc(M2_size * sizeof(double));
            double *M2_copy = (double *) malloc((M2_size) * sizeof(double));
            unsigned int A = (unsigned int) 240;


            for (i = 0; i < MAX_I; i++) /* 50 экспериментов */
            {
                /* Заполнить массив исходных данных размером N */
                //aka Этап Generate
                unsigned int seed = i;
                for (j = 0; j < N; j++) {// генерим М1
                    M1[j] = custom_rand(1.0, A, &seed);
                }

                for (j = 0; j < M2_size; j++) {// генерим М2 и его копию
                    double rand = custom_rand(A, 10.0 * A, &seed);
                    M2[j] = rand;
                    M2_copy[j] = rand;
                }

                /* Решить поставленную задачу, заполнить массив с результатами */
                //aka этап Map для M1
				gettimeofday(&T3, NULL);
#pragma omp parallel for default(none) shared(M1, N) num_threads(CONST_NUM_THREADS) SCHEDULE_STRING
                for (j = 0; j < N; j++) {//Кубический корень после деления на число e
                    M1[j] = pow((double) (M1[j] / exp(1.0)), 1.0 / 3.0);
                }

                //этап Map для M2
#pragma omp parallel for default(none) shared(M2, M2_copy, M2_size) num_threads(CONST_NUM_THREADS) SCHEDULE_STRING
                for (j = 0; j < M2_size; j++) {//Десятичный логарифм, возведенный в степень e
                    double sum = M2_copy[j]; //для нач элемента массива предыдущий элемент равен0
                    if (j > 0) {
                        sum += M2_copy[j - 1]; //каждый элемент поочерёдно сложить с предыдущим
                    }
                    M2[j] = pow(log10(sum), exp(1.0));
                }
				gettimeofday(&T4, NULL);
				delta_map_stage_ms += 1000 * (T4.tv_sec - T3.tv_sec) + (T4.tv_usec - T3.tv_usec) / 1000;
				
				gettimeofday(&T3, NULL);
                //этап Merge
#pragma omp parallel for default(none) shared(M1, M2, M2_size) num_threads(CONST_NUM_THREADS) SCHEDULE_STRING
                for (j = 0; j < M2_size; j++) {//Модуль разности
                    M2[j] = fabs(M1[j] - M2[j]);
                }
				
				gettimeofday(&T4, NULL);
				delta_merge_stage_ms += 1000 * (T4.tv_sec - T3.tv_sec) + (T4.tv_usec - T3.tv_usec) / 1000;

                /* Отсортировать массив с результатами указанным методом */
                //aka этап Sort

				gettimeofday(&T3, NULL);
#ifdef _OPENMP
                int k = omp_get_num_procs();
#pragma omp parallel default(none) shared(M2, M2_size) num_threads(k)
                {
                    const int num_threads = omp_get_num_threads(); //кол-во тредов выполняют секцию
                    const int chunk_size = M2_size / num_threads + 1; // размер куска для треда
                    const int cur_thread = omp_get_thread_num();//текущий номер треда
                    const int from = chunk_size * cur_thread;
                    const int to = MIN(chunk_size * (cur_thread + 1), M2_size);
                    long location;
                    double elem;
                    for (int p = from + 1; p < to; p++) {
                        elem = M2[p];
                        location = p - 1;
                        while (location >= from && M2[location] > elem) {
                            M2[location + 1] = M2[location];
                            location = location - 1;
                        }
                        M2[location + 1] = elem;
                    }
                }
                //здесь у нас 6 (или k) кусков массивов, каждый из которых внутри себя отсортирован,
                //но сам массив еще нет. когда у нас массив частично сортирован, mergeSort - то что нужно
                mergeSort(M2, 0, M2_size - 1);

#else
                long location;
                double elem;
                for (j = 1; j < M2_size; j++) {//Сортировка вставками (Insertion sort).
                    elem = M2[j];
                    location = j - 1;
                    while (location >= 0 && M2[location] > elem) {
                        M2[location + 1] = M2[location];
                        location = location - 1;
                    }
                    M2[location + 1] = elem;
                }
#endif

				gettimeofday(&T4, NULL);
				delta_sort_stage_ms += 1000 * (T4.tv_sec - T3.tv_sec) + (T4.tv_usec - T3.tv_usec) / 1000;
				
				gettimeofday(&T3, NULL);
                double minNotZero = DBL_MAX;
#pragma omp parallel for default(none) shared(M2, M2_size, i) reduction(min:minNotZero) num_threads(CONST_NUM_THREADS) SCHEDULE_STRING
                for (j = 0; j < M2_size; j++) {//ищим минимальный ненулевой элемент массива М2
                    if (M2[j] > 0 && M2[j] < minNotZero)
                        minNotZero = M2[j];
                }

                double X = 0;
#pragma omp parallel for default(none) shared(M2, M2_size, minNotZero, i) reduction(+:X) num_threads(CONST_NUM_THREADS) SCHEDULE_STRING
                for (j = 0; j < M2_size; j++) {//Рассчитать сумму синусов тех элементов массиваМ2,
                    //которые при делении на минимальный ненулевой
                    //элемент массива М2 дают чётное число
                    if (((long) (M2[j] / minNotZero)) % 2 == 0)
                        X += sin(M2[j]);
                }
				gettimeofday(&T4, NULL);
				delta_reduce_stage_ms += 1000 * (T4.tv_sec - T3.tv_sec) + (T4.tv_usec - T3.tv_usec) / 1000;
                printf("N=%d\t X=%.20f\n", i, X);
            }
            free(M1);
            free(M2);
            free(M2_copy);

#ifdef _OPENMP
            delta_ms = (omp_get_wtime() - start) * 1000;
        }
    

#else
    gettimeofday(&T2, NULL);   /* запомнить текущее время T2 */
    delta_ms = 1000 * (T2.tv_sec - T1.tv_sec) + (T2.tv_usec - T1.tv_usec) / 1000;
    pthread_join(thread, NULL);
#endif

    printf("100%% completed\n");
    printf("\nN=%d. Milliseconds passed: %ld\n", N, delta_ms); /* T2 - T1 */
	    printf("Map:%ld\n", delta_map_stage_ms);
    printf("Merge:%ld\n", delta_merge_stage_ms);
    printf("Sort:%ld\n", delta_sort_stage_ms);
    printf("reduce:%ld\n", delta_reduce_stage_ms);
    return 0;
}