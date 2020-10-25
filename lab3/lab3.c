#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>

#define SCHEDULE_STRING
#define CONST_NUM_THREADS 6

double custom_rand(double from, double to, unsigned int *seed) {
    int random_int;
#pragma omp critical
    random_int = rand_r(seed); //random [0; RAND_MAX]
    double random_double = (double) (random_int) / RAND_MAX; //random [0.0; 1.0]
    random_double = (to - from) * random_double; //random [0.0; (to - from)]
    return from + random_double; //random [from; to]
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
    gettimeofday(&T1, NULL); /* запомнить текущее время T1 */
    for (i = 0; i < 50; i++) /* 50 экспериментов */
    {
        /* Заполнить массив исходных данных размером N */
        //aka Этап Generate
        unsigned int seed = i;
#pragma omp parallel for default(none) shared(seed, M1, A, N) num_threads(CONST_NUM_THREADS) SCHEDULE_STRING
        for (j = 0; j < N; j++) {// генерим М1
            printf("%d\n", j);
            M1[j] = custom_rand(1.0, A, &seed);
        }

#pragma omp parallel for default(none) shared(seed, M2, M2_copy, A, N, M2_size) num_threads(CONST_NUM_THREADS) SCHEDULE_STRING
        for (j = 0; j < M2_size; j++) {// генерим М2 и его копию
            double rand = custom_rand(A, 10.0 * A, &seed);
            M2[j] = rand;
            M2_copy[j] = rand;
        }

        /* Решить поставленную задачу, заполнить массив с результатами */
        //aka этап Map для M1
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

        //этап Merge
#pragma omp parallel for default(none) shared(M1, M2, M2_size) num_threads(CONST_NUM_THREADS) SCHEDULE_STRING
        for (j = 0; j < M2_size; j++) {//Модуль разности
            M2[j] = fabs(M1[j] - M2[j]);
        }

        /* Отсортировать массив с результатами указанным методом */
        //aka этап Sort
        //сортировку нельзя распаралелить так же просто как и остальные циклы
        //но можно в несколько потоков отсортировать массив по кускам и одним потоком доотсортировать
        //но это выходит за рамки ЛР3
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
        //бессмысленно распараллеливать так как блок цикла потребуется объявить критической сецкией
        //из-за проверки и присвоения
        //однако можно было бы разделить проверку условия на два этапа
        //проверка элемента что он больше нуля и проверка что он меньше minNotZero
        //в таком случае критическая секция началась бы после первой проверки
        //но это выходит за рамки ЛР3
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
                X += sin(M2[i]);
        }
        printf("N=%d\t X=%.20f\n", i, X);
    }
    free(M1);
    free(M2);
    free(M2_copy);
    gettimeofday(&T2, NULL);   /* запомнить текущее время T2 */
    delta_ms = 1000 * (T2.tv_sec - T1.tv_sec) + (T2.tv_usec - T1.tv_usec) / 1000;
    printf("\nN=%d. Milliseconds passed: %ld\n", N, delta_ms); /* T2 - T1 */
    return 0;
}