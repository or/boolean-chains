#include <algorithm>
#include <bitset>
#include <cstdint>
#include <cuda_runtime.h>
#include <random>
#include <stdio.h>
#include <vector>

// --- Problem Constants ---
// These constants define the parameters of the search problem.

// N defines the number of bits in the boolean functions.
#define N 12
// The maximum length of the logic chain to search for.
#define MAX_LENGTH 18
// The total number of possible functions is 2^(N-1), as f(x) = f(~x).
#define SIZE (1 << (N - 1))
// A bitmask representing a tautology (all ones).
#define TAUTOLOGY ((1 << N) - 1)
// The number of target functions we are trying to synthesize.
#define NUM_TARGETS 7

#define MAX_SOLUTIONS 1000

// The maximum number of parallel search tasks to generate on the CPU.
// Each task will be assigned to a CUDA thread.
#define MAX_GPU_TASKS 65536

// A structure to store a chain on the GPU.
struct Chain {
  uint32_t length;
  uint32_t expressions[1000];
  uint32_t expressions_size[MAX_LENGTH];
  uint32_t chain[MAX_LENGTH];
  uint32_t choices[MAX_LENGTH];
  uint8_t unseen[SIZE];
};

// __constant__ memory is fast, read-only memory accessible by all threads.
// It's perfect for storing the target functions.
__constant__ uint32_t d_TARGETS[NUM_TARGETS];

// A device-side lookup table to quickly check if a function is a target.
// Initialized once from the host.
__device__ uint8_t d_TARGET_LOOKUP[SIZE];

#define ADD_EXPRESSION(value, chain, chain_size)                               \
  chain.expressions[chain.expressions_size[chain_size]] = value;               \
  chain.expressions_size[chain_size] += chain.unseen[value];                   \
  chain.unseen[value] = 0;

#define ADD_EXPRESSION_TARGET(value, chain, chain_size)                        \
  {                                                                            \
    const uint32_t v = value;                                                  \
    const uint32_t a = chain.unseen[v] & d_TARGET_LOOKUP[v];                   \
    chain.expressions[chain.expressions_size[chain_size]] = v;                 \
    chain.expressions_size[chain_size] += a;                                   \
    chain.unseen[v] &= ~a;                                                     \
  }

#define GENERATE_NEW_EXPRESSIONS(chain_size, chain_struct, add_expression)     \
  {                                                                            \
    chain_struct.expressions_size[chain_size] =                                \
        chain_struct.expressions_size[chain_size - 1];                         \
    const uint32_t h = chain_struct.chain[chain_size - 1];                     \
    const uint32_t not_h = ~h;                                                 \
                                                                               \
    for (int j = 0; j < chain_size - 1; j++) {                                 \
      const uint32_t g = chain_struct.chain[j];                                \
      const uint32_t not_g = ~chain_struct.chain[j];                           \
      add_expression(g & h, chain_struct, chain_size);                         \
      add_expression(not_g & h, chain_struct, chain_size);                     \
      add_expression(g & not_h, chain_struct, chain_size);                     \
      add_expression(g ^ h, chain_struct, chain_size);                         \
      add_expression(g | h, chain_struct, chain_size);                         \
    }                                                                          \
  }

#define LOOP(CS, PREV_CS, NEXT_CS, chain_struct)                               \
  loop_##CS : if (CS < MAX_LENGTH) {                                           \
    if (chain_struct.choices[CS] < chain_struct.expressions_size[CS]) {        \
      chain_struct.chain[CS] =                                                 \
          chain_struct.expressions[chain_struct.choices[CS]];                  \
                                                                               \
      if (CS < MAX_LENGTH - 1 && NEXT_CS >= MAX_LENGTH - NUM_TARGETS) {        \
        if (__builtin_expect(                                                  \
                NEXT_CS + num_unfulfilled_targets -                            \
                        d_TARGET_LOOKUP[chain_struct.chain[CS]] ==             \
                    MAX_LENGTH,                                                \
                1)) {                                                          \
          tmp_chain_size = NEXT_CS;                                            \
          generated_chain_size = CS;                                           \
          tmp_num_unfulfilled_targets =                                        \
              num_unfulfilled_targets -                                        \
              d_TARGET_LOOKUP[chain_struct.chain[CS]];                         \
          uint32_t j = chain_struct.choices[CS] + 1;                           \
                                                                               \
          next_##CS : if (__builtin_expect(tmp_chain_size < MAX_LENGTH, 1)) {  \
            GENERATE_NEW_EXPRESSIONS(tmp_chain_size, chain_struct,             \
                                     ADD_EXPRESSION_TARGET)                    \
            generated_chain_size = tmp_chain_size;                             \
                                                                               \
            for (; j < chain_struct.expressions_size[tmp_chain_size]; ++j) {   \
              if (__builtin_expect(                                            \
                      d_TARGET_LOOKUP[chain_struct.expressions[j]], 0)) {      \
                chain_struct.chain[tmp_chain_size] =                           \
                    chain_struct.expressions[j];                               \
                tmp_num_unfulfilled_targets--;                                 \
                tmp_chain_size++;                                              \
                if (__builtin_expect(!tmp_num_unfulfilled_targets, 0)) {       \
                  chain.length = tmp_chain_size;                               \
                  uint32_t sol_idx = atomicAdd(solution_count, 1);             \
                  if (sol_idx < MAX_SOLUTIONS) {                               \
                    solutions[sol_idx] = chain_struct;                         \
                  }                                                            \
                  break;                                                       \
                }                                                              \
                j++;                                                           \
                goto next_##CS;                                                \
              }                                                                \
            }                                                                  \
          }                                                                    \
                                                                               \
          for (uint32_t i = chain_struct.expressions_size[CS];                 \
               i < chain_struct.expressions_size[generated_chain_size]; i++) { \
            chain_struct.unseen[chain_struct.expressions[i]] = 1;              \
          }                                                                    \
                                                                               \
          chain_struct.choices[CS] +=                                          \
              1 + (d_TARGET_LOOKUP[chain_struct.chain[CS]] << 16);             \
          goto loop_##CS;                                                      \
        }                                                                      \
      }                                                                        \
                                                                               \
      num_unfulfilled_targets -= d_TARGET_LOOKUP[chain_struct.chain[CS]];      \
                                                                               \
      chain_struct.choices[NEXT_CS] = chain_struct.choices[CS] + 1;            \
      GENERATE_NEW_EXPRESSIONS(NEXT_CS, chain_struct, ADD_EXPRESSION)          \
                                                                               \
      goto loop_##NEXT_CS;                                                     \
    }                                                                          \
                                                                               \
    if (CS > 6) {                                                              \
      for (uint32_t i = chain_struct.expressions_size[PREV_CS];                \
           i < chain_struct.expressions_size[CS]; i++) {                       \
        chain_struct.unseen[chain_struct.expressions[i]] = 1;                  \
      }                                                                        \
      num_unfulfilled_targets += d_TARGET_LOOKUP[chain_struct.chain[PREV_CS]]; \
      chain_struct.choices[PREV_CS] +=                                         \
          1 + (d_TARGET_LOOKUP[chain_struct.chain[PREV_CS]] << 16);            \
      goto loop_##PREV_CS;                                                     \
    } else {                                                                   \
      continue;                                                                  \
    }                                                                          \
  }

/**
 * @brief The main CUDA kernel for finding optimal logic chains.
 * Each thread executes this kernel to explore a part of the search space.
 * The recursive search of the original C++ code is transformed into an
 * iterative search using a manually managed stack (`search_stack`).
 *
 * @param initial_chains An array of starting chains generated by the host.
 * @param num_tasks The total number of starting chains to process.
 * @param solutions A device buffer to store found solutions.
 * @param solution_count An atomic counter for the number of solutions found.
 */
__global__ void find_optimal_chain_kernel(const Chain *initial_chains,
                                          uint32_t num_tasks,
                                          uint32_t tasks_per_thread,
                                          Chain *solutions,
                                          uint32_t *solution_count) {
  // Determine which task this thread is responsible for.
  uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;
  for (uint32_t task_id = idx * tasks_per_thread; task_id < idx * tasks_per_thread + tasks_per_thread && task_id < num_tasks;
       task_id++) {
    Chain chain = initial_chains[task_id];
    uint8_t num_unfulfilled_targets = NUM_TARGETS;
    uint8_t tmp_num_unfulfilled_targets;
    uint8_t tmp_chain_size;
    uint8_t generated_chain_size;
    chain.choices[chain.length] = chain.choices[chain.length - 1] + 1;
    GENERATE_NEW_EXPRESSIONS(chain.length, chain, ADD_EXPRESSION)

    // Calculate how many targets are unfulfilled by this initial chain.
    for (uint32_t i = 0; i < chain.length; i++) {
      num_unfulfilled_targets -= d_TARGET_LOOKUP[chain.chain[i]];
    }

    // If the initial chain is already a solution, store it.
    if (num_unfulfilled_targets == 0) {
      uint32_t sol_idx = atomicAdd(solution_count, 1);
      if (sol_idx < MAX_SOLUTIONS) {
        solutions[sol_idx] = chain;
      }
      return;
    }

    LOOP(8, 7, 9, chain)
    LOOP(9, 8, 10, chain)
    LOOP(10, 9, 11, chain)
    LOOP(11, 10, 12, chain)
    LOOP(12, 11, 13, chain)
    LOOP(13, 12, 14, chain)
    LOOP(14, 13, 15, chain)
    LOOP(15, 14, 16, chain)
    LOOP(16, 15, 17, chain)
    LOOP(17, 16, 18, chain)
    LOOP(18, 17, 19, chain)
    LOOP(19, 18, 20, chain)

  loop_20: // shouldn't be used

  loop_7:
    continue;
  }
}

// --- Host-side C++ Code ---

// Helper to print a chain's binary representation.
void print_chain(const Chain &s, const uint8_t *target_lookup) {
  printf("chain (%d):\n", s.length);
  for (uint32_t i = 0; i < s.length; i++) {
    printf("x%d", i + 1);
    for (uint32_t j = 0; j < i; j++) {
      for (uint32_t k = j + 1; k < i; k++) {
        char op = 0;
        if (s.chain[i] == (s.chain[j] & s.chain[k])) {
          op = '&';
        } else if (s.chain[i] == (s.chain[j] | s.chain[k])) {
          op = '|';
        } else if (s.chain[i] == (s.chain[j] ^ s.chain[k])) {
          op = '^';
        } else if (s.chain[i] == ((~s.chain[j]) & s.chain[k])) {
          op = '<';
        } else if (s.chain[i] == (s.chain[j] & (~s.chain[k]))) {
          op = '>';
        } else {
          continue;
        }

        printf(" = x%d %c x%d", j + 1, op, k + 1);
      }
    }
    printf(" = %s", std::bitset<N>(s.chain[i]).to_string().c_str());
    if (target_lookup[s.chain[i]]) {
      printf(" [target]");
    }
    printf("\n");
  }
}

int main(int argc, char *argv[]) {
  printf("Starting logic chain synthesis for N=%d\n", N);
  printf("Max length: %d, Num targets: %d\n", MAX_LENGTH, NUM_TARGETS);

  // --- CUDA Device Setup ---
  uint32_t dev_id = 0;
  cudaDeviceProp props;
  cudaSetDevice(dev_id);
  cudaGetDeviceProperties(&props, dev_id);
  printf("Using GPU: %s\n", props.name);
  if (props.major < 3) {
    printf("Error: This code requires a GPU with compute capability 3.0 or "
           "higher.\n");
    return 1;
  }

  // --- Host-side Data Initialization ---
  uint32_t host_targets[NUM_TARGETS] = {
      ((~(uint32_t)0b1011011111100011) >> (16 - N)) & TAUTOLOGY,
      ((~(uint32_t)0b1111100111100100) >> (16 - N)) & TAUTOLOGY,
      ((~(uint32_t)0b1101111111110100) >> (16 - N)) & TAUTOLOGY,
      ((~(uint32_t)0b1011011011011110) >> (16 - N)) & TAUTOLOGY,
      ((~(uint32_t)0b1010001010111111) >> (16 - N)) & TAUTOLOGY,
      ((~(uint32_t)0b1000111111110011) >> (16 - N)) & TAUTOLOGY,
      (((uint32_t)0b0011111011111111) >> (16 - N)) & TAUTOLOGY,
  };

  uint8_t target_lookup[SIZE] = {0};
  for (uint32_t i = 0; i < NUM_TARGETS; i++) {
    target_lookup[host_targets[i]] = 1;
  }

  uint32_t start_i = 1;
  // -c for chunk mode, only complete one slice of the depth given by the
  // progress vector
  if (argc > 1 && strcmp(argv[1], "-c") == 0) {
    start_i++;
  }

  uint32_t start_indices_size __attribute__((aligned(64))) = 0;
  uint16_t start_indices[100] __attribute__((aligned(64))) = {0};

  // --- Generate Initial Search Tasks on the CPU ---
  printf("Generating initial search tasks on the CPU...\n");
  std::vector<Chain> initial_tasks;

  Chain base_chain;
  base_chain.length = 4;
  base_chain.chain[0] = 0b0000000011111111 >> (16 - N);
  base_chain.chain[1] = 0b0000111100001111 >> (16 - N);
  base_chain.chain[2] = 0b0011001100110011 >> (16 - N);
  base_chain.chain[3] = 0b0101010101010101 >> (16 - N);

  for (uint32_t i = 0; i < base_chain.length; i++) {
    start_indices[start_indices_size++] = 0;
  }

  // read the progress vector, e.g 5 2 9, commas will be ignored: 5, 2, 9
  for (uint32_t i = start_i; i < argc; i++) {
    start_indices[start_indices_size++] = atoi(argv[i]);
  }

  memset(base_chain.unseen, 1, sizeof(base_chain.unseen));
  base_chain.unseen[0] = 0;
  for (int32_t k = 0; k < base_chain.length; k++) {
    base_chain.unseen[base_chain.chain[k]] = 0;
    base_chain.choices[k] = 0xffffffff;
  }

  memset(base_chain.expressions_size, 0, sizeof(base_chain.expressions_size));
  // only do it up to length - 1, the length step is done below
  for (int32_t k = 1; k < base_chain.length - 1; k++) {
    GENERATE_NEW_EXPRESSIONS(k + 1, base_chain, ADD_EXPRESSION);
    // printf("k: %d, expr size: %d\n", k + 1, base_chain.expressions_size[k +
    // 1]);
  }

  while (base_chain.length < start_indices_size) {
    GENERATE_NEW_EXPRESSIONS(base_chain.length, base_chain, ADD_EXPRESSION);
    base_chain.choices[base_chain.length] = start_indices[base_chain.length];
    base_chain.chain[base_chain.length] =
        base_chain.expressions[base_chain.choices[base_chain.length]];
    base_chain.length++;
  }

  // Unroll the first few levels of the search to create independent tasks
  uint32_t cpu_search_depth = 4;
  std::vector<Chain> queue;
  queue.push_back(base_chain);

  for (uint32_t depth = 0; depth < cpu_search_depth; ++depth) {
    // printf("generating at depth %d: %d in queue\n", depth, queue.size());
    std::vector<Chain> next_queue;
    for (auto &current : queue) {
      GENERATE_NEW_EXPRESSIONS(current.length, current, ADD_EXPRESSION);
      // printf("chain: %d length, %d expression size\n", current.length,
      //        current.expressions_size[current.length]);
      for (uint32_t i = current.choices[current.length - 1] + 1;
           i < current.expressions_size[current.length]; ++i) {
        current.choices[current.length] = i;
        current.chain[current.length] =
            current.expressions[current.choices[current.length]];
        current.length++;
        next_queue.push_back(current);
        current.length--;
      }
    }
    queue = next_queue;
  }
  initial_tasks = queue;
  printf("Generated %zu tasks to be processed by the GPU.\n",
         initial_tasks.size());

  std::random_device rd;
  std::default_random_engine rng(rd());

  // Shuffle the vector
  std::shuffle(initial_tasks.begin(), initial_tasks.end(), rng);

  // --- Allocate and Transfer Data to GPU ---
  cudaError_t err;

  Chain *d_initial_chains;
  err = cudaMalloc(&d_initial_chains, initial_tasks.size() * sizeof(Chain));
  if (err != cudaSuccess) {
    printf("cudaMalloc failed: %s\n", cudaGetErrorString(err));
    return 1;
  }

  err =
      cudaMemcpy(d_initial_chains, initial_tasks.data(),
                 initial_tasks.size() * sizeof(Chain), cudaMemcpyHostToDevice);
  if (err != cudaSuccess) {
    printf("cudaMemcpy failed: %s\n", cudaGetErrorString(err));
    return 1;
  }

  Chain *d_solutions;
  cudaMalloc(&d_solutions, MAX_SOLUTIONS * sizeof(Chain));

  uint32_t *d_solution_count;
  cudaMalloc(&d_solution_count, sizeof(uint32_t));
  cudaMemset(d_solution_count, 0, sizeof(uint32_t));

  // Copy data to __constant__ and __device__ global memory
  cudaMemcpyToSymbol(d_TARGETS, host_targets, NUM_TARGETS * sizeof(uint32_t));
  cudaMemcpyToSymbol(d_TARGET_LOOKUP, target_lookup, SIZE * sizeof(uint8_t));

  // --- Launch Kernel ---
  printf("Launching CUDA kernel...\n");
  uint32_t num_tasks = initial_tasks.size();
  uint32_t tasks_per_thread = num_tasks / MAX_GPU_TASKS + 1;
  uint32_t num_threads = num_tasks / tasks_per_thread + 1;
  uint32_t threads_per_block = 256;
  uint32_t blocks_per_grid =
      (num_threads + threads_per_block - 1) / threads_per_block;

  find_optimal_chain_kernel<<<blocks_per_grid, threads_per_block>>>(
      d_initial_chains, num_tasks, tasks_per_thread, d_solutions,
      d_solution_count);

  // Synchronize to wait for the kernel to finish and check for errors.
  cudaDeviceSynchronize();
  err = cudaGetLastError();
  if (err != cudaSuccess) {
    printf("Kernel launch failed: %s\n", cudaGetErrorString(err));
  }

  // --- Retrieve and Print Results ---
  printf("Kernel finished. Retrieving results...\n");
  int solution_count_host = 0;
  cudaMemcpy(&solution_count_host, d_solution_count, sizeof(uint32_t),
             cudaMemcpyDeviceToHost);
  printf("found %d solutions, only kept up to %d\n", solution_count_host,
         MAX_SOLUTIONS);
  solution_count_host = std::min(solution_count_host, MAX_SOLUTIONS);

  if (solution_count_host > 0) {
    std::vector<Chain> solutions_host(solution_count_host);
    cudaMemcpy(solutions_host.data(), d_solutions,
               solution_count_host * sizeof(Chain), cudaMemcpyDeviceToHost);

    for (const auto &s : solutions_host) {
      print_chain(s, target_lookup);
    }
  } else {
    printf("No solutions found.\n");
  }

  // --- Cleanup ---
  cudaFree(d_initial_chains);
  cudaFree(d_solutions);
  cudaFree(d_solution_count);

  return 0;
}
