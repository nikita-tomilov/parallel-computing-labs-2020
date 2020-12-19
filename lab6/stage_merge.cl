__kernel void merge(
        __global float *M1,
        __global float *M2,
        __global float *M2_ret
) {
    M2_ret[get_global_id(0)] = fabs(M1[get_global_id(0)] - M2[get_global_id(0)]);
}