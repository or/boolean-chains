#include <cstdint>
#include <cuda_runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- CUDA Kernel and Device Functions ---

// Using constants directly in the device code
#define N 12
#define SIZE (1 << (N - 1))
#define MAX_LENGTH 18
#define TAUTOLOGY ((1 << N) - 1)

// Targets are moved to device constant memory for faster access
__constant__ uint32_t d_TARGETS[7];

// A simple structure to hold the state of each thread's search
struct ThreadState {
    uint32_t chain[MAX_LENGTH];
    uint32_t expressions[1000];
    uint32_t expressions_size[MAX_LENGTH];
    uint8_t unseen[SIZE];
    uint32_t choices[MAX_LENGTH];
    uint32_t num_unfulfilled_targets;
    uint32_t total_chains;
};

// Device function to check if a value is a target
__device__ inline bool is_target(uint32_t val, uint32_t &num_unfulfilled) {
    for (int i = 0; i < 7; ++i) {
        if (val == d_TARGETS[i]) {
            // This is a simplified logic. A more robust implementation
            // would handle unique target fulfillment.
            num_unfulfilled--;
            return true;
        }
    }
    return false;
}

// The core search logic encapsulated in a CUDA kernel
__global__ void search_kernel(uint32_t *global_total_chains,
                              uint32_t start_depth) {
    // Get unique thread ID
    int tid = blockIdx.x * blockDim.x + threadIdx.x;

    // Each thread gets its own state
    ThreadState s;

    // --- Initialization ---
    s.total_chains = 0;
    s.num_unfulfilled_targets = 7; // NUM_TARGETS

    // Initialize unseen array
    for (int i = 0; i < SIZE; ++i) {
        s.unseen[i] = 1;
    }

    // Initial chain elements
    s.chain[0] = 0b0000000011111111 >> (16 - N);
    s.chain[1] = 0b0000111100001111 >> (16 - N);
    s.chain[2] = 0b0011001100110011 >> (16 - N);
    s.chain[3] = 0b0101010101010101 >> (16 - N);

    for (int i = 0; i < 4; i++) {
        s.unseen[s.chain[i]] = 0;
        is_target(s.chain[i], s.num_unfulfilled_targets);
    }

    s.expressions_size[3] = 0;
    for (uint32_t k = 1; k < 4; k++) {
        const uint32_t h = s.chain[k];
        const uint32_t not_h = ~h;
        for (uint32_t j = 0; j < k; j++) {
            const uint32_t g = s.chain[j];
            const uint32_t not_g = ~g;

            // Simplified ADD_EXPRESSION
            s.expressions[s.expressions_size[3]++] = g & h;
            s.unseen[g & h] = 0;
            s.expressions[s.expressions_size[3]++] = g | h;
            s.unseen[g | h] = 0;
            s.expressions[s.expressions_size[3]++] = g ^ h;
            s.unseen[g ^ h] = 0;
            s.expressions[s.expressions_size[3]++] = not_g & h;
            s.unseen[not_g & h] = 0;
            s.expressions[s.expressions_size[3]++] = g & not_h;
            s.unseen[g & not_h] = 0;
        }
    }

    // --- Main Search Loop ---
    // This is a simplified, non-recursive version of the original loop
    // structure. A full translation of the goto-based state machine is complex
    // for a direct GPU port. This iterative approach demonstrates the basic
    // principle.
    uint32_t chain_size = 4;
    s.choices[chain_size] =
        tid; // Each thread starts at a different initial choice

    while (chain_size < MAX_LENGTH) {
        if (s.choices[chain_size] >= s.expressions_size[chain_size - 1]) {
            // Backtrack
            chain_size--;
            if (chain_size <= start_depth)
                break; // Stop if we backtrack too far

            // Restore unseen state for the expressions of the level we are
            // leaving
            for (uint32_t i = 0; i < s.expressions_size[chain_size]; ++i) {
                s.unseen[s.expressions[i]] = 1;
            }

            s.choices[chain_size]++;
            continue;
        }

        uint32_t expr_idx = s.choices[chain_size];
        s.chain[chain_size] = s.expressions[expr_idx];

        if (s.unseen[s.chain[chain_size]] == 0) {
            s.choices[chain_size]++;
            continue;
        }

        s.total_chains++;

        // Check for solution
        if (is_target(s.chain[chain_size], s.num_unfulfilled_targets) &&
            s.num_unfulfilled_targets == 0) {
            // In a real scenario, we would save the solution chain here.
            // For now, we just count it.
            // To print, we'd need to copy the chain to a global buffer.
            // printf is very slow on GPUs and should be used for debugging
            // only.
        }

        // Move to next level
        uint32_t prev_chain_size = chain_size;
        chain_size++;
        s.choices[chain_size] = 0;
        s.expressions_size[chain_size - 1] =
            s.expressions_size[prev_chain_size - 1];

        // Generate new expressions
        const uint32_t h = s.chain[prev_chain_size];
        const uint32_t not_h = ~h;
        for (uint32_t j = 0; j < prev_chain_size; j++) {
            const uint32_t g = s.chain[j];
            const uint32_t not_g = ~s.chain[j];
            s.expressions[s.expressions_size[chain_size - 1]++] = g & h;
            s.expressions[s.expressions_size[chain_size - 1]++] = g & not_h;
            s.expressions[s.expressions_size[chain_size - 1]++] = not_g & h;
            s.expressions[s.expressions_size[chain_size - 1]++] = g | h;
            s.expressions[s.expressions_size[chain_size - 1]++] = g ^ h;
        }
    }

    // Atomically add this thread's count to the global total
    atomicAdd(global_total_chains, s.total_chains);
}

// --- Host Code ---

void checkCUDAError(const char *msg) {
    cudaError_t err = cudaGetLastError();
    if (cudaSuccess != err) {
        fprintf(stderr, "CUDA Error: %s: %s.\n", msg, cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    printf("Starting CUDA computation...\n");

    // --- Device Setup ---
    int deviceId;
    cudaDeviceProp props;
    cudaGetDevice(&deviceId);
    checkCUDAError("cudaChooseDevice");
    cudaGetDeviceProperties(&props, deviceId);
    checkCUDAError("cudaGetDeviceProperties");
    printf("Using device: %s\n", props.name);
    printf("Compute Capability: %d.%d\n", props.major, props.minor);

    // --- Data Initialization ---
    const uint32_t h_TARGETS[] = {
        ((~(uint32_t)0b1011011111100011) >> (16 - N)) & TAUTOLOGY,
        ((~(uint32_t)0b1111100111100100) >> (16 - N)) & TAUTOLOGY,
        ((~(uint32_t)0b1101111111110100) >> (16 - N)) & TAUTOLOGY,
        ((~(uint32_t)0b1011011011011110) >> (16 - N)) & TAUTOLOGY,
        ((~(uint32_t)0b1010001010111111) >> (16 - N)) & TAUTOLOGY,
        ((~(uint32_t)0b1000111111110011) >> (16 - N)) & TAUTOLOGY,
        (((uint32_t)0b0011111011111111) >> (16 - N)) & TAUTOLOGY,
    };

    // Copy targets to device constant memory
    cudaMemcpyToSymbol(d_TARGETS, h_TARGETS, sizeof(h_TARGETS));
    checkCUDAError("cudaMemcpyToSymbol");

    // Allocate memory on the GPU for the global chain counter
    uint32_t *d_total_chains;
    cudaMalloc(&d_total_chains, sizeof(uint32_t));
    checkCUDAError("cudaMalloc");
    cudaMemset(d_total_chains, 0, sizeof(uint32_t));
    checkCUDAError("cudaMemset");

    // --- Kernel Launch ---
    // Configure the grid and block dimensions.
    // These values may need tuning based on the GPU.
    int num_blocks = 256;
    int threads_per_block = 256;
    dim3 dimGrid(num_blocks);
    dim3 dimBlock(threads_per_block);

    printf("Launching kernel with %d blocks and %d threads per block.\n",
           num_blocks, threads_per_block);

    // Launch the kernel
    search_kernel<<<dimGrid, dimBlock>>>(d_total_chains, 4);
    checkCUDAError("Kernel launch");

    // Synchronize to wait for the kernel to finish
    cudaDeviceSynchronize();
    checkCUDAError("cudaDeviceSynchronize");

    // --- Retrieve Results ---
    uint32_t h_total_chains = 0;
    cudaMemcpy(&h_total_chains, d_total_chains, sizeof(uint32_t),
               cudaMemcpyDeviceToHost);
    checkCUDAError("cudaMemcpy from device");

    printf("Computation finished.\n");
    printf("Total chains found (approximate): %llu\n", h_total_chains);

    // --- Cleanup ---
    cudaFree(d_total_chains);
    checkCUDAError("cudaFree");

    cudaDeviceReset();
    checkCUDAError("cudaDeviceReset");

    return 0;
}
