#define CL_TARGET_OPENCL_VERSION 120
#define __CL_ENABLE_EXCEPTIONS

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <CL/cl.h>
#include <string.h>
#include <stddef.h>

cl_uint platformCount;
cl_uint ret;
cl_device_id device;
cl_uint uNumCPU;

size_t source_size;
char *source_str;
const int g_cuNumItems = 1200;
//const int num_cores = CL_DEVICE_MAX_COMPUTE_UNITS;
const int num_cores = 12;

void merge(float *M1, float *M2, int min_non_zero);

void sum_sin(float *M2, float min_non_zero);

void createProgram(char *fileName) {

    FILE *program_handle;
    program_handle = fopen(fileName, "r");
    if (program_handle == NULL) {
        perror("Couldn't find the program file");
        exit(1);
    }
    fseek(program_handle, 0, SEEK_END);
    source_size = ftell(program_handle);
    rewind(program_handle);
    source_str = (char *) malloc(source_size + 1);
    source_str[source_size] = '\0';
    fread(source_str, sizeof(char), source_size, program_handle);
    fclose(program_handle);
}


int main() {

    float *M1 = (float *) malloc(sizeof(float) * g_cuNumItems);
    float *M2 = (float *) malloc(sizeof(float) * g_cuNumItems);

    for (int i = 0; i < g_cuNumItems; i++) {
        M1[i] = 2;
        M2[i] = 5;
    }

    merge(M1, M2, g_cuNumItems);

    float num = 1.0;
    sum_sin(M1, num);

    return 0;
}

void merge(float *M1, float *M2, int M2_size) {
    cl_platform_id platform;
    cl_int ret = clGetPlatformIDs(1, &platform, &platformCount);
    printf("clGetPlatformIDs ret %d\n", ret);
    printf("number platforms %d\n", platformCount);

    ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &uNumCPU);
    printf("clGetDeviceIDs ret %d\n", ret);

    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &ret);
    printf("clCreateContext ret %d\n", ret);

    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &ret);
    printf("clCreateCommandQueue ret %d\n", ret);

    createProgram("stage_merge.cl");
    cl_program program = clCreateProgramWithSource(context, 1, (const char **) &source_str,
                                                   (const size_t *) &source_size, &ret);
    printf("clCreateProgramWithSource ret %d\n", ret);

    ret = clBuildProgram(program, 1, &device, "-cl-std=CL1.2", NULL, NULL);
    printf("clBuildProgram ret %d\n", ret);
    if (ret == CL_BUILD_PROGRAM_FAILURE) {
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char *) malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("%s\n", log);
    }

    cl_kernel kernel = clCreateKernel(program, "merge", &ret);
    printf("clCreateKernel ret %d\n", ret);

    cl_mem M1_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, M2_size * sizeof(float), NULL, &ret);
    printf("clCreateBuffer M1 ret %d\n", ret);
    cl_mem M2_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, M2_size * sizeof(float), NULL, &ret);
    printf("clCreateBuffer M2 ret %d\n", ret);
    cl_mem M2_ret_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, M2_size * sizeof(float), NULL, &ret);
    printf("clCreateBuffer M2_ret ret %d\n", ret);

    ret = clEnqueueWriteBuffer(queue, M1_clmem, CL_TRUE, 0, M2_size * sizeof(float), M1, 0, NULL, NULL);
    printf("clEnqueueWriteBuffer M1_clmem ret %d\n", ret);
    ret = clEnqueueWriteBuffer(queue, M2_clmem, CL_TRUE, 0, M2_size * sizeof(float), M2, 0, NULL, NULL);
    printf("clEnqueueWriteBuffer M2_clmem ret %d\n", ret);

    clSetKernelArg(kernel, 0, sizeof(M1_clmem), (void *) &M1_clmem);
    clSetKernelArg(kernel, 1, sizeof(M2_clmem), (void *) &M2_clmem);
    clSetKernelArg(kernel, 2, sizeof(M2_ret_clmem), (void *) &M2_ret_clmem);

    size_t uGlobalWorkSize = M2_size;
    clEnqueueNDRangeKernel(
            queue,
            kernel,
            1,
            NULL,
            &uGlobalWorkSize,
            NULL,
            0,
            NULL,
            NULL);
    clFinish(queue);

    cl_float *puData = (cl_float *) clEnqueueMapBuffer(
            queue,
            M2_ret_clmem,
            CL_TRUE,
            CL_MAP_READ,
            0,
            M2_size * sizeof(cl_float),
            0,
            NULL,
            NULL,
            NULL);

    for (int i = 0; i < M2_size; ++i)
        printf("%f \n", puData[i]);

    clEnqueueUnmapMemObject(queue, M2_ret_clmem, puData, 0, NULL, NULL);
    clReleaseContext(context);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    free(source_str);
    clReleaseKernel(kernel);
}

void sum_sin(float *M2, float min_non_zero) {
//    float min_non_zero = 3;
    cl_platform_id platform;
    cl_int ret = clGetPlatformIDs(1, &platform, &platformCount);
    printf("clGetPlatformIDs ret %d\n", ret);
    printf("number platforms %d\n", platformCount);

    ret = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &uNumCPU);
    printf("clGetDeviceIDs ret %d\n", ret);

    cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &ret);
    printf("clCreateContext ret %d\n", ret);

    cl_command_queue queue = clCreateCommandQueue(context, device, 0, &ret);
    printf("clCreateCommandQueue ret %d\n", ret);

    createProgram("sum_sin.cl");
    cl_program program = clCreateProgramWithSource(context, 1, (const char **) &source_str,
                                                   (const size_t *) &source_size, &ret);
    printf("clCreateProgramWithSource ret %d\n", ret);

    ret = clBuildProgram(program, 1, &device, "-cl-std=CL1.2", NULL, NULL);
    printf("clBuildProgram ret %d\n", ret);
    if (ret == CL_BUILD_PROGRAM_FAILURE) {
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char *log = (char *) malloc(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("%s\n", log);
    }

    cl_kernel kernel = clCreateKernel(program, "sum_sin", &ret);
    printf("clCreateKernel ret %d\n", ret);

    cl_mem M2_clmem = clCreateBuffer(context, CL_MEM_READ_ONLY, g_cuNumItems * sizeof(float), NULL, &ret);
    printf("clCreateBuffer M2 ret %d\n", ret);
    cl_mem M2_ret_clmem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, g_cuNumItems * sizeof(float), NULL, &ret);
    printf("clCreateBuffer M2_ret ret %d\n", ret);

    ret = clEnqueueWriteBuffer(queue, M2_clmem, CL_TRUE, 0, g_cuNumItems * sizeof(float), M2, 0, NULL, NULL);
    printf("clEnqueueWriteBuffer M2_clmem ret %d\n", ret);

    clSetKernelArg(kernel, 0, sizeof(float), (void *) &min_non_zero);
    clSetKernelArg(kernel, 1, sizeof(M2_clmem), (void *) &M2_clmem);
    clSetKernelArg(kernel, 2, sizeof(M2_ret_clmem), (void *) &M2_ret_clmem);

    size_t uGlobalWorkSize = g_cuNumItems;
    size_t uLocalWorkSize = num_cores;
    clEnqueueNDRangeKernel(
            queue,
            kernel,
            1,
            NULL,
            &uGlobalWorkSize,
            &uLocalWorkSize,
            0,
            NULL,
            NULL);
    clFinish(queue);

    cl_float *puData = (cl_float *) clEnqueueMapBuffer(
            queue,
            M2_ret_clmem,
            CL_TRUE,
            CL_MAP_READ,
            0,
            g_cuNumItems / num_cores * sizeof(cl_float),
            0,
            NULL,
            NULL,
            NULL);

    for (int i = 0; i < g_cuNumItems / num_cores; ++i)
        printf("%f \n", puData[i]);

    clEnqueueUnmapMemObject(queue, M2_ret_clmem, puData, 0, NULL, NULL);
    clReleaseContext(context);
    clReleaseCommandQueue(queue);
    clReleaseProgram(program);
    free(source_str);
    clReleaseKernel(kernel);
}