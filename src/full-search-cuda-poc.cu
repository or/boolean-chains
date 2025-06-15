#include <assert.h>
#include <cuda_runtime.h>
#include <helper_cuda.h>
#include <helper_functions.h>
#include <stdio.h>

__global__ void cuda_hello(int val) { printf("Hello! %d\n", val); }

int main(int argc, char **argv) {
    int dev_id;
    cudaDeviceProp props;

    dev_id = findCudaDevice(argc, (const char **)argv);

    checkCudaErrors(cudaGetDevice(&dev_id));
    checkCudaErrors(cudaGetDeviceProperties(&props, dev_id));

    dim3 dimGrid(2, 2);
    dim3 dimBlock(2, 2, 2);

    cuda_hello<<<dimGrid, dimBlock>>>(16);

    cudaDeviceSynchronize();

    return 0;
}
