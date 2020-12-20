__kernel void merge(
        __global float *M1,
        __global float *M2,
        __global float *M2_ret
) {
    int global_id = get_global_id(0);
    M2_ret[global_id] = fabs(M1[global_id] - M2[global_id]);
}

__kernel void m1map(
        __global float *M1,
        __global float *B,
        __global float *Ret
) {
    int global_id = get_global_id(0);
    Ret[global_id] = pow((float) (M1[global_id] / exp(1.0)), (float)(1.0 / 3.0));
}

__kernel void m2map(
        __global float *M2_copy,
        __global float *M2,
        __global float *Ret
) {
    int global_id = get_global_id(0);

    float sum = M2[global_id];
    if (global_id > 0) {
        sum += M2_copy[global_id - 1];
    }

    Ret[global_id] = pow((float)log10(sum), (float)exp(1.0));
}