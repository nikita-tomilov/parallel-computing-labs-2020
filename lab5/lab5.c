#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>
#include <float.h>
#include <pthread.h>
#include <sys/time.h>

#define MIN(a, b) (((a)<(b))?(a):(b))

#define SCHEDULE_STRING
#define CONST_NUM_THREADS 4
#define MAX_I 1

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

struct arg_struct {//структура которую распаралеливаемая вункция получает как параметры
    double* array;
	long arrayLenght;
} ;

void voidFunc(){};

pthread_t* firstFreedThread(pthread_t *threads, int threadsNum){
	while(1){
		for(int i = 0 ; i < threadsNum ; i++){
			//printf("hear 1\n");
			if( pthread_tryjoin_np(threads[i], NULL) != 0){
				//printf("hear 2\n");
				return (pthread_t*)threads[i];
			}
		}	
	}
};

void multiThreadComputing(int threadsNum, double array[], long arrayLenght, void* func, void* funcReduction, int chunkSize){
	pthread_t *thread = (pthread_t *) malloc(threadsNum * sizeof(pthread_t));//создаем потоки
	int chunks = arrayLenght / chunkSize;//вычисляем кол-во кусков
	chunks -= 1;//последний кусок может быть меньше указаного размера
	struct arg_struct* argsArray = malloc(sizeof(struct arg_struct) * chunks);
	for(int j = 0; j < threadsNum; j++){//запускаем пустые функции
		pthread_create(&thread[j], NULL, (void*)voidFunc, NULL);
	}

	for(int i = 0 ; i < chunks ; i++){ //работаем куски размером chunkSize
		argsArray[i].arrayLenght = chunkSize;
		argsArray[i].array = &array[i * chunkSize];
		printf("chunk %d address %p\n", i, (void*)argsArray[i].array);
		pthread_t *threadFreed = firstFreedThread(thread, threadsNum);
		pthread_create(threadFreed, NULL, func, &argsArray[i]);
	}
	//считаем огрызок если есть
	if(arrayLenght % chunkSize > 0){
		struct arg_struct* stubArgs = malloc(sizeof(struct arg_struct));
		stubArgs->arrayLenght = arrayLenght % chunkSize;
		stubArgs->array = &array[chunks * chunkSize];
		pthread_t *threadFreed = firstFreedThread(thread, threadsNum);
		pthread_create(threadFreed, NULL, (void*)func, stubArgs);
		pthread_join(*threadFreed, NULL);
		free(stubArgs);
	}

	for(int i = 0 ; i < threadsNum ; i++){
		pthread_join(thread[i], NULL);
	}
	free(thread);
	free(argsArray);
}

void m1Map(struct arg_struct *args){
	for (int j = 0; j < args->arrayLenght; j++) {//Кубический корень после деления на число e
		//printf("%d\n",(*((double*)args->array)));
		args->array[j] = pow((double) (args->array[j] / exp(1.0)), 1.0 / 3.0);
	}
}
void m2Map(struct arg_struct *args){
	for (int j = 0; j < args->arrayLenght; j++) {//Десятичный логарифм, возведенный в степень e
			double sum = args->array[j]; //для нач элемента массива предыдущий элемент равен0
			if (j > 0) {
				sum += args->array[j - 1]; //каждый элемент поочерёдно сложить с предыдущим
			}
			args->array[j] = pow(log10(sum), exp(1.0));
		}
}

int main(int argc, char *argv[]) {
    int i = 0;
    long delta_ms;
    int N;
	
	struct timeval T1, T2;
	gettimeofday(&T1, NULL); /* запомнить текущее время T1 */
	pthread_t thread;
	pthread_create(&thread, NULL, printPercent, &i);


	int j;
	N = atoi(argv[1]); /* N равен первому параметру командной строки */
	int M2_size = N / 2;
	double *M1 = (double *) malloc(N * sizeof(double));
	printf("address M1 %p\n",(void*)M1);
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
		
		//multiThreadComputing(2, M1, N, printHello, NULL, 20);

		/* Решить поставленную задачу, заполнить массив с результатами */
		//aka этап Map для M1
		//m1Map(M1);
		multiThreadComputing(2, M1, N, m1Map, NULL, 20);
		
		printf("print M1\n");
		for (j = 0; j < N; j++) {//Кубический корень после деления на число e
					printf("%d - %f\n",j, M1[j]);
        }

		//этап Map для M2
		//multiThreadComputing(1, M2_copy, M2_size, m2Map, NULL, 20);
		/*for (j = 0; j < M2_size; j++) {//Десятичный логарифм, возведенный в степень e
			double sum = M2_copy[j]; //для нач элемента массива предыдущий элемент равен0
			if (j > 0) {
				sum += M2_copy[j - 1]; //каждый элемент поочерёдно сложить с предыдущим
			}
			M2[j] = pow(log10(sum), exp(1.0));
		}*/

		//этап Merge
		for (j = 0; j < M2_size; j++) {//Модуль разности
			M2[j] = fabs(M1[j] - M2[j]);
		}

		/* Отсортировать массив с результатами указанным методом */
		//aka этап Sort


/*#pragma omp parallel default(none) shared(M2, M2_size) num_threads(k)
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
*/
		
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
//#pragma omp parallel for default(none) shared(M2, M2_size, i) reduction(min:minNotZero) num_threads(CONST_NUM_THREADS) SCHEDULE_STRING
		for (j = 0; j < M2_size; j++) {//ищим минимальный ненулевой элемент массива М2
			if (M2[j] > 0 && M2[j] < minNotZero)
				minNotZero = M2[j];
		}

		double X = 0;
//#pragma omp parallel for default(none) shared(M2, M2_size, minNotZero, i) reduction(+:X) num_threads(CONST_NUM_THREADS) SCHEDULE_STRING
		for (j = 0; j < M2_size; j++) {//Рассчитать сумму синусов тех элементов массиваМ2,
			//которые при делении на минимальный ненулевой
			//элемент массива М2 дают чётное число
			if (((long) (M2[j] / minNotZero)) % 2 == 0)
				X += sin(M2[i]);
		}
		//printf("N=%d\t X=%.20f\n", i, X);
	}
	free(M1);
	free(M2);
	free(M2_copy);

    gettimeofday(&T2, NULL);   /* запомнить текущее время T2 */
    delta_ms = 1000 * (T2.tv_sec - T1.tv_sec) + (T2.tv_usec - T1.tv_usec) / 1000;
    pthread_join(thread, NULL);


    printf("100%% completed\n");
    printf("\nN=%d. Milliseconds passed: %ld\n", N, delta_ms); /* T2 - T1 */
    return 0;
}