#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>

int main(int argc, char *argv[]) {
    int i, j, N, M2_size;
    struct timeval T1, T2;
    long delta_ms;
    N = atoi(argv[1]); /* N равен первому параметру командной строки */
    M2_size = N / 2;
    double *M1 = (double *) malloc(N * sizeof(double));
    double *M2 = (double *) malloc(M2_size * sizeof(double));
    double *M2_copy = (double *) malloc((M2_size) * sizeof(double));
    unsigned int A = (unsigned int) 240;
    gettimeofday(&T1, NULL); /* запомнить текущее время T1 */
    for (i = 0; i < 50; i++) /* 50 экспериментов */
    {
        srand(i);  /* инициализировать начальное значение ГСЧ   */
        /* Заполнить массив исходных данных размером N */
        unsigned int A_for_rand = A - 1;
        for (j = 0; j < N; j++) {// генерим М1
            M1[j] = (double) (1 + rand_r(&A_for_rand));
        }

        A_for_rand = A * 9;
        for (j = 0; j < M2_size; j++) {// генерим М2 и его копию
            double rand = (A + (double) rand_r(&A_for_rand));
            M2[j] = rand;
            M2_copy[j] = rand;
        }

        /* Решить поставленную задачу, заполнить массив с результатами */
        for (j = 0; j < N; j++) {//Кубический корень после деления на число e
            M1[j] = pow((double) (M1[j] / exp(1.0)), 1 / 3);
        }

        for (j = 0; j < M2_size - 1; j++) {//Десятичный логарифм, возведенный в степень e
            M2[j + 1] = pow(log10(M2_copy[j] + M2_copy[j + 1]), exp(1.0));
        }

        for (j = 0; j < M2_size; j++) {//Модуль разности
            M2[j] = fabs(M2_copy[j] - M2[j]);
        }

        /* Отсортировать массив с результатами указанным методом */
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
        for (j = 0; j < M2_size; j++) {//ищим минимальный ненулевой элемент массива М2
            if (M2[j] > 0 && M2[j] < minNotZero)
                minNotZero = M2[j];
        }

        double X = 0;
        for (j = 0; j < M2_size; j++) {//Рассчитать сумму синусов тех элементов массиваМ2,
            //которые при делении на минимальный ненулевой 
            //элемент массива М2 дают чётное число
            if (((long) (M2[j] / minNotZero)) % 2 == 0)
                X += sin(M2[i]);
        }
        printf("N=%d\t X=%.20f\n", i, X);
    }
    gettimeofday(&T2, NULL);   /* запомнить текущее время T2 */
    delta_ms = 1000 * (T2.tv_sec - T1.tv_sec) + (T2.tv_usec - T1.tv_usec) / 1000;
    printf("\nN=%d. Milliseconds passed: %ld\n", N, delta_ms); /* T2 - T1 */
    return 0;
}
