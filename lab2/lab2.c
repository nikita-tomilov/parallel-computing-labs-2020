#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>

#include "library/fwBase.h"
#include "library/fwSignal.h"

double custom_rand(double from, double to, unsigned int *seed) {
    int random_int = rand_r(seed); //random [0; RAND_MAX]
    double random_double = (double) (random_int) / RAND_MAX; //random [0.0; 1.0]
    random_double = (to - from) * random_double; //random [0.0; (to - from)]
    return from + random_double; //random [from; to]
}
//http://framewave.sourceforge.net/Manual/aa_000_frames.html
int main(int argc, char *argv[]) {
    int i, j, N, M2_size;
    struct timeval T1, T2;
    long delta_ms;
    N = atoi(argv[1]); /* N равен первому параметру командной строки */
    M2_size = N / 2;
    double *M1 = malloc(N * sizeof(double));
    double *M2 = (double *) malloc(M2_size * sizeof(double));
    double *M2_copy = (double *) malloc((M2_size + 1) * sizeof(double));
    unsigned int A = (unsigned int) 240;
    double e = exp(1.0);

    fwSetNumThreads(atoi(argv[2]));

    gettimeofday(&T1, NULL); /* запомнить текущее время T1 */
    for (i = 0; i < 50; i++) /* 50 экспериментов */
    {
        /* Заполнить массив исходных данных размером N */
        //aka Этап Generate
        unsigned int seed = i;
        for (j = 0; j < N; j++) {// генерим М1
            M1[j] = custom_rand(1.0, A, &seed);
        }

        M2_copy[0] = 0;
        for (j = 0; j < M2_size; j++) {// генерим М2 и его копию
            double rand = custom_rand(A, 10.0 * A, &seed);
            M2[j] = rand;
            M2_copy[j + 1] = rand;
        }
        /* Решить поставленную задачу, заполнить массив с результатами */
        //aka этап Map для M1
        //Кубический корень после деления на число e
        fwsDivC_64f_I(e, M1, N);
        fwsPowx_64f_A53(M1, 1.0 / 3.0, M1, N);

        //этап Map для M2
        //в массиве М2 каждый элемент поочерёдно сложить с преды-
        //дущим (для начального элемента массива предыдущий элемент равен нулю),
        //а к результату сложения применить след операцию:
        //"Десятичный логарифм, возведенный в степень e"
        //(было в лр1 M2[i] = M2[i] + M2[i - 1] для i > 0,
        //станет в лр2 M2'[i] = M2'[i] + M2_copy'[i], т к M2_copy'[i] == M2_copy[i - 1],
        //смещение около строки 44
        fwsAdd_64f(M2, M2_copy, M2, M2_size);
        fwsLog10_64f_A53(M2, M2, M2_size);
        fwsPowx_64f_A53(M2, e, M2, M2_size);

        //этап Merge
        //Модуль разности
        //опять учитываем смещение
        fwsSub_64f(M2, (M2_copy + 1), M2, M2_size);
        fwsAbs_64f_I(M2, M2_size);

        /* Отсортировать массив с результатами указанным методом */
        //aka этап Sort
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
    free(M1);
    free(M2);
    free(M2_copy);
    gettimeofday(&T2, NULL);   /* запомнить текущее время T2 */
    delta_ms = 1000 * (T2.tv_sec - T1.tv_sec) + (T2.tv_usec - T1.tv_usec) / 1000;
    printf("\nN=%d. Milliseconds passed: %ld\n", N, delta_ms); /* T2 - T1 */
    return 0;
}