__kernel void sum_sin(
        __global float *M1,
        __global float *M2,
        __global float *M2_ret
) {
    /*if (((long) (M2[j] / minNotZero)) % 2 == 0)
        X += sin(M2[j]);*/
    float min_non_zero = M1[0];
    float x = 0;
    int local_size = get_local_size(0);
    int global_size = get_global_size(0);
    int group_id = get_group_id(0);
    //printf("OPENCL: group_id %d, local_size %d, global_size %d, min_not_zero %f\n", group_id, local_size, global_size, min_non_zero);
    for (int i = 0; i < local_size; i++) {
        int element_idx = group_id * local_size + i;
        if ((global_size > element_idx) && (((int)(M2[element_idx] / min_non_zero) % 2) == 0)
        ) {
            x += sin(M2[element_idx]);
        }
    }
    M2_ret[group_id] = x;
    //printf("OPENCL: %d is %f\n", group_id, M2_ret[group_id]);
}