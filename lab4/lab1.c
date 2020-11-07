#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <unistd.h>

#ifdef _OPENMP

	#include "omp.h"

#else
	#include <pthread.h>

	int omp_set_num_threads(int M) { return 1; }
	int omp_get_num_threads() { return 1; }
	int omp_get_thread_num() { return 0; }
	int omp_get_num_procs(){return 0;}
#endif

#define CONST_NUM_THREADS 7

double custom_rand(double from, double to, unsigned int *seed) {
    int random_int;
    random_int = rand_r(seed); //random [0; RAND_MAX]
    double random_double = (double) (random_int) / RAND_MAX; //random [0.0; 1.0]
    random_double = (to - from) * random_double; //random [0.0; (to - from)]
    return from + random_double; //random [from; to]
}

void* printPercent(void *i) {
	while ( *(int*)i < 50){
		printf("%d%% completed\n", 2 * *(int*)i);
		usleep(1000000);
	}
	return 0;
}

int main(int argc, char *argv[]) {
    int i, j;
    struct timeval T1, T2;
    long delta_ms;
    int N = atoi(argv[1]); /* N равен первому параметру командной строки */
    int M2_size = N / 2;
    double *M1 = (double *) malloc(N * sizeof(double));
    double *M2 = (double *) malloc(M2_size * sizeof(double));
    double *M2_copy = (double *) malloc((M2_size) * sizeof(double));
    unsigned int A = (unsigned int) 240;
    
	#ifdef _OPENMP
	double start = omp_get_wtime();
	#pragma omp task default(none) shared(i)
		printPercent(&i);
	#else
	gettimeofday(&T1, NULL); /* запомнить текущее время T1 */
	pthread_t thread;
	pthread_create(&thread, NULL, printPercent, &i);
	#endif
	
    for (i = 0; i < 50; i++) /* 50 экспериментов */
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
#pragma omp parallel for default(none) shared(M1, N)
        for (j = 0; j < N; j++) {//Кубический корень после деления на число e
            M1[j] = pow((double) (M1[j] / exp(1.0)), 1.0 / 3.0);
        }

        //этап Map для M2
#pragma omp parallel for default(none) shared(M2, M2_copy, M2_size)
        for (j = 0; j < M2_size; j++) {//Десятичный логарифм, возведенный в степень e
            double sum = M2_copy[j]; //для нач элемента массива предыдущий элемент равен0
            if (j > 0) {
                sum += M2_copy[j - 1]; //каждый элемент поочерёдно сложить с предыдущим
            }
            M2[j] = pow(log10(sum), exp(1.0));
        }

        //этап Merge
#pragma omp parallel for default(none) shared(M1, M2, M2_size)
        for (j = 0; j < M2_size; j++) {//Модуль разности
            M2[j] = fabs(M1[j] - M2[j]);
        }

        /* Отсортировать массив с результатами указанным методом */
        //aka этап Sort
        
		int k = omp_get_num_procs();
#pragma omp parallel default(none) shared(M2, M2_size) num_threads(k)
		{
			const int num_threads = omp_get_num_threads(); //кол-во тредов выполняют секцию
			const int chunk = M2_size / num_threads + 1; // размер куска для треда
			const int cur_thread = omp_get_thread_num();//текущий номер треда
			
			long location;
			double elem;
			if(omp_get_num_threads() - 1 != cur_thread ) // цикл для всех тредов кроме последнего
				for (int p = chunk * cur_thread ; p < chunk * (cur_thread +1); p++) {
					elem = M2[p];
					location = p - 1;
					while (location >= 0 && M2[location] > elem) {
						M2[location + 1] = M2[location];
						location = location - 1;
					}
					M2[location + 1] = elem;
				}
			else{ // цикл для последнего треда как как нет гарантии что массив поровну разделится
			
				for (int p = chunk * cur_thread ; p < M2_size ; p++){
					elem = M2[p];
					location = p - 1;
					while (location >= 0 && M2[location] > elem) {
						M2[location + 1] = M2[location];
						location = location - 1;
					}
					M2[location + 1] = elem;
				}
			}
		}
		
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

        double minNotZero = 0;
#pragma omp parallel for default(none) shared(M2, M2_size, i) reduction(min:minNotZero)
        for (j = 0; j < M2_size; j++) {//ищим минимальный ненулевой элемент массива М2
            if (M2[j] > 0 && M2[j] < minNotZero)
                minNotZero = M2[j];
        }

        double X = 0;
#pragma omp parallel for default(none) shared(M2, M2_size, minNotZero, i) reduction(+:X)
        for (j = 0; j < M2_size; j++) {//Рассчитать сумму синусов тех элементов массиваМ2,
            //которые при делении на минимальный ненулевой
            //элемент массива М2 дают чётное число
            if (((long) (M2[j] / minNotZero)) % 2 == 0)
                X += sin(M2[i]);
        }
        printf("N=%d\t X=%.20f\n", i, X);
    }
    free(M1);
    free(M2);
    free(M2_copy);
	
    #ifdef _OPENMP
	delta_ms = (omp_get_wtime() - start)*1000;
	#else
	gettimeofday(&T2, NULL);   /* запомнить текущее время T2 */
	delta_ms = 1000 * (T2.tv_sec - T1.tv_sec) + (T2.tv_usec - T1.tv_usec) / 1000;
	pthread_join(thread, NULL);
	#endif
	
    printf("100%% completed\n");
    printf("\nN=%d. Milliseconds passed: %ld\n", N, delta_ms); /* T2 - T1 */
    return 0;
}