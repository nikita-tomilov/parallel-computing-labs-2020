__kernel void sum_sin(
        float min_non_zero,
        __global float *M2,
        __global float *M2_ret
) {
    /*if (((long) (M2[j] / minNotZero)) % 2 == 0)
        X += sin(M2[j]);*/
    float x = 0;
    for (int i = 0; i < get_local_size(0); i++)
        if ((get_global_size(0) > get_group_id(0) * get_local_size(0) + i)
        && (((int)(M2[get_group_id(0) * get_local_size(0) + i] / min_non_zero) % 2) == 0)
        )
            x += sin(M2[get_group_id(0) * get_local_size(0) + i]);
    M2_ret[get_group_id(0)] = x;
}