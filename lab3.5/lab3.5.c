#include <stdio.h>

#ifdef _OPENMP

#include "omp.h"

#else
int omp_set_num_threads(int M) { return 1; }
int omp_get_num_threads() { return 1; }
int omp_get_thread_num() { return 0; }
#endif

#define MAX_THREADS 6

#define MIN(a, b) (((a)<(b))?(a):(b))

int main(int argc, char *argv[]) {
    const int N = 10;
    omp_set_num_threads(MAX_THREADS);
#pragma omp parallel default(none)
    {
        const int num_threads = omp_get_num_threads();
        const int N_per_thread = N / num_threads + 1;
        const int cur_thread = omp_get_thread_num();
        const int offset = cur_thread * N_per_thread;
        const int count = MIN(N, (cur_thread + 1) * N_per_thread);
        for (int i = offset; i < count; i++) {
#pragma omp critical
            printf("hello %d from thread %d\n", i, cur_thread);
        }
    };
}
