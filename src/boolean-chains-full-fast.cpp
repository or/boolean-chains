#include <bitset>
#include <cinttypes>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
using namespace std;

#ifndef CAPTURE_STATS
#define CAPTURE_STATS 1
#endif

#ifndef PLAN_MODE
#define PLAN_MODE 0
#endif

#if CAPTURE_STATS
#define CAPTURE_STATS_CALL                                                     \
  {                                                                            \
    const auto new_expressions = next_expressions_size - expressions_size;     \
    if (new_expressions < stats_min_num_expressions[chain_size]) {             \
      stats_min_num_expressions[chain_size] = new_expressions;                 \
    }                                                                          \
    if (new_expressions > stats_max_num_expressions[chain_size]) {             \
      stats_max_num_expressions[chain_size] = new_expressions;                 \
    }                                                                          \
    stats_total_num_expressions[chain_size] += new_expressions;                \
    stats_num_data_points[chain_size]++;                                       \
  }
#else
#define CAPTURE_STATS_CALL
#endif

constexpr uint32_t N = 15;
constexpr uint32_t SIZE = 1 << (N - 1);
constexpr uint32_t MAX_LENGTH = 21;
constexpr uint32_t TAUTOLOGY = (1 << N) - 1;
constexpr uint32_t TARGET_1 =
    ((~(uint32_t)0b1011011111100011) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_2 =
    ((~(uint32_t)0b1111100111100100) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_3 =
    ((~(uint32_t)0b1101111111110100) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_4 =
    ((~(uint32_t)0b1011011011011110) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_5 =
    ((~(uint32_t)0b1010001010111111) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_6 =
    ((~(uint32_t)0b1000111111110011) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGET_7 =
    (((uint32_t)0b0011111011111111) >> (16 - N)) & TAUTOLOGY;
constexpr uint32_t TARGETS[] = {
    TARGET_1, TARGET_2, TARGET_3, TARGET_4, TARGET_5, TARGET_6, TARGET_7,
};
constexpr uint32_t NUM_TARGETS = sizeof(TARGETS) / sizeof(uint32_t);

uint32_t TARGET_LOOKUP[SIZE] = {0};

bool chunk_mode = false;
uint16_t start_indices[100] = {0};
size_t start_indices_size = 0;
size_t current_best_length = 1000;
uint16_t choices[30];
uint64_t total_chains = 0;
constexpr uint32_t start_chain_length = 4;
uint32_t seen[SIZE] __attribute__((aligned(64)));
uint32_t chain[25] __attribute__((aligned(64)));
uint32_t expressions[1000] __attribute__((aligned(64)));

#if CAPTURE_STATS
#define UNDEFINED 0xffffffff
uint64_t stats_total_num_expressions[25] = {0};
uint32_t stats_min_num_expressions[25] = {UNDEFINED};
uint32_t stats_max_num_expressions[25] = {0};
uint64_t stats_num_data_points[25] = {0};
#endif

#if PLAN_MODE
size_t plan_depth = 1;
#endif

#define ADD_EXPRESSION(expressions_size, value)                                \
  expressions[expressions_size] = value;                                       \
  expressions_size += seen[value];                                             \
  seen[value] = 0;

#define GENERATE_NEW_EXPRESSIONS                                               \
  const uint32_t h = chain[chain_size - 1];                                    \
  const uint32_t not_h = ~h;                                                   \
  for (size_t j = 0; j < chain_size - 1; j++) {                                \
    const uint32_t g = chain[j];                                               \
    const uint32_t not_g = ~g;                                                 \
                                                                               \
    const uint32_t ft1 = g & h;                                                \
    ADD_EXPRESSION(next_expressions_size, ft1)                                 \
                                                                               \
    const uint32_t ft2 = g & not_h;                                            \
    ADD_EXPRESSION(next_expressions_size, ft2)                                 \
                                                                               \
    const uint32_t ft3 = g ^ h;                                                \
    ADD_EXPRESSION(next_expressions_size, ft3)                                 \
                                                                               \
    const uint32_t ft4 = g | h;                                                \
    ADD_EXPRESSION(next_expressions_size, ft4)                                 \
                                                                               \
    const uint32_t ft5 = not_g & h;                                            \
    ADD_EXPRESSION(next_expressions_size, ft5)                                 \
  }

void print_chain(const size_t chain_size) {
  cout << "chain (" << chain_size << "):" << endl;
  for (size_t i = 0; i < chain_size; i++) {
    cout << "x" << i + 1;
    for (size_t j = 0; j < i; j++) {
      for (size_t k = j + 1; k < i; k++) {
        char op = 0;
        if (chain[i] == (chain[j] & chain[k])) {
          op = '&';
        } else if (chain[i] == (chain[j] | chain[k])) {
          op = '|';
        } else if (chain[i] == (chain[j] ^ chain[k])) {
          op = '^';
        } else if (chain[i] == ((~chain[j]) & chain[k])) {
          op = '<';
        } else if (chain[i] == (chain[j] & (~chain[k]))) {
          op = '>';
        } else {
          continue;
        }

        cout << " = " << "x" << j + 1 << " " << op << " x" << k + 1;
      }
    }
    cout << " = " << bitset<N>(chain[i]).to_string();
    if (TARGET_LOOKUP[chain[i]]) {
      cout << " [target]";
    }
    cout << endl;
  }
  cout << endl;
}

void find_optimal_chain(const size_t chain_size,
                        const size_t num_unfulfilled_target_functions,
                        const size_t expressions_size,
                        const size_t expressions_index) {
#if PLAN_MODE
  if (chain_size - start_chain_length >= plan_depth) {
    cout << "-c";
    for (size_t j = start_chain_length; j < chain_size; ++j) {
      cout << " " << choices[j];
    }
    cout << endl;
    return;
  }
#endif

  const size_t next_chain_size = chain_size + 1;
  size_t next_expressions_size = expressions_size;

  GENERATE_NEW_EXPRESSIONS

  CAPTURE_STATS_CALL

  for (size_t i = expressions_index; i < next_expressions_size;) {
    choices[chain_size] = i;
    chain[chain_size] = expressions[i];
    const uint32_t next_num_unfulfilled_targets =
        num_unfulfilled_target_functions - TARGET_LOOKUP[chain[chain_size]];

    total_chains++;
    if (__builtin_expect((total_chains & 0xffffffff) == 0, 0)) {
      for (size_t j = start_chain_length; j < next_chain_size; ++j) {
        cout << choices[j];
        if (j != next_chain_size - 1) {
          cout << ", ";
        }
      }
      cout << " [best: " << current_best_length << "] " << total_chains << endl;
      // exit(0);
    }

    if (next_chain_size + next_num_unfulfilled_targets > MAX_LENGTH) {
      i++;
      continue;
    }

    if (__builtin_expect(!next_num_unfulfilled_targets, 0)) {
      if (next_chain_size < current_best_length) {
        cout << "New best chain found (" << next_chain_size << "):" << endl;
        print_chain(next_chain_size);
        current_best_length = next_chain_size;
      } else if (next_chain_size == current_best_length) {
        print_chain(next_chain_size);
      }
      i++;
      continue;
    }

    // do this manually here, this avoids an extra addition for i + 1 in the
    // call
    i++;
    find_optimal_chain(next_chain_size, next_num_unfulfilled_targets,
                       next_expressions_size, i);
  }

  for (size_t i = expressions_size; i < next_expressions_size; i++) {
    seen[expressions[i]] = 1;
  }
}

void find_optimal_chain_restore_progress(
    const size_t chain_size, const size_t num_unfulfilled_target_functions,
    const size_t expressions_size, const size_t expressions_index) {
  const size_t next_chain_size = chain_size + 1;
  size_t next_expressions_size = expressions_size;

  GENERATE_NEW_EXPRESSIONS

  choices[chain_size] = start_indices[chain_size];
  chain[chain_size] = expressions[start_indices[chain_size]];
  const uint32_t next_num_unfulfilled_targets =
      num_unfulfilled_target_functions - TARGET_LOOKUP[chain[chain_size]];

  cout << "skipping to ";
  for (size_t j = start_chain_length; j < chain_size; ++j) {
    cout << choices[j] << ", ";
  }
  cout << choices[chain_size] << endl;

  if (next_chain_size < start_indices_size) {
    find_optimal_chain_restore_progress(
        next_chain_size, next_num_unfulfilled_targets, next_expressions_size,
        choices[chain_size] + 1);
  } else {
    find_optimal_chain(next_chain_size, next_num_unfulfilled_targets,
                       next_expressions_size, choices[chain_size] + 1);
  }
}

void on_exit() {
  cout << "total chains: " << total_chains << endl;

#if CAPTURE_STATS
  cout << "new expressions at chain length:" << endl;
  cout << "                   n                       sum              avg     "
          "         "
          "min              max"
       << endl;

  stats_total_num_expressions[start_chain_length] +=
      stats_total_num_expressions[start_chain_length - 1];
  stats_min_num_expressions[start_chain_length] +=
      stats_min_num_expressions[start_chain_length - 1];
  stats_max_num_expressions[start_chain_length] +=
      stats_min_num_expressions[start_chain_length - 1];
  for (int i = start_chain_length; i < MAX_LENGTH; i++) {
    printf("%2d: %16" PRIu64 " %25" PRIu64 " %16" PRIu64 " %16" PRId32
           " %16" PRIu32 "\n",
           i, stats_num_data_points[i], stats_total_num_expressions[i],
           stats_num_data_points[i] == 0
               ? 0
               : stats_total_num_expressions[i] / stats_num_data_points[i],
           stats_min_num_expressions[i] == UNDEFINED
               ? 0
               : stats_min_num_expressions[i],
           stats_max_num_expressions[i]);
  }
  cout << flush;
#endif
}

void signal_handler(int signal) { exit(signal); }

int main(int argc, char *argv[]) {
#if PLAN_MODE
  if (argc > 1) {
    plan_depth = atoi(argv[1]);
  }
#else
  atexit(on_exit);
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  cout << "N = " << N << ", MAX_LENGTH: " << MAX_LENGTH
       << ", CAPTURE_STATS: " << CAPTURE_STATS << endl;

  size_t start_i = 1;
  // -c for chunk mode, only complete one slice of the depth given by the
  // progress vector
  if (argc > 1 && strcmp(argv[1], "-c") == 0) {
    start_i++;
    chunk_mode = true;
  }

#endif

#if PLAN_MODE != 1
  cout << NUM_TARGETS << " targets:" << endl;
#endif
  for (size_t i = 0; i < NUM_TARGETS; i++) {
#if PLAN_MODE != 1
    cout << "  " << bitset<N>(TARGETS[i]).to_string() << endl;
#endif
    TARGET_LOOKUP[TARGETS[i]] = 1;
  }

#if CAPTURE_STATS
  memset(stats_min_num_expressions, UNDEFINED,
         sizeof(stats_min_num_expressions));
#endif

  chain[0] = 0b0000000011111111 >> (16 - N);
  chain[1] = 0b0000111100001111 >> (16 - N);
  chain[2] = 0b0011001100110011 >> (16 - N);
  chain[3] = 0b0101010101010101 >> (16 - N);
  size_t chain_size = 4;

  for (size_t i = 0; i < chain_size; i++) {
    start_indices[start_indices_size++] = 0;
  }

#if PLAN_MODE != 1
  // read the progress vector, e.g 5 2 9, commas will be ignored: 5, 2, 9
  for (size_t i = start_i; i < argc; i++) {
    start_indices[start_indices_size++] = atoi(argv[i]);
  }
#endif

  for (size_t i = 0; i < SIZE; i++) {
    // flip the logic: 1 means unseen, 0 seen, that'll avoid one operation when
    // setting this flag
    seen[i] = 1;
  }

  seen[0] = 0;
  for (size_t i = 0; i < chain_size; i++) {
    seen[chain[i]] = 0;
  }

  const uint32_t expressions_size = 0;
  size_t next_expressions_size = 0;
  for (size_t k = 1; k < chain_size - 1; k++) {
    const uint32_t h = chain[k];
    const uint32_t not_h = ~h;
    for (size_t j = 0; j < k; j++) {
      const uint32_t g = chain[j];
      const uint32_t not_g = ~g;

      const uint32_t ft1 = g & h;
      ADD_EXPRESSION(next_expressions_size, ft1)

      const uint32_t ft2 = g & not_h;
      ADD_EXPRESSION(next_expressions_size, ft2)

      const uint32_t ft3 = g ^ h;
      ADD_EXPRESSION(next_expressions_size, ft3)

      const uint32_t ft4 = g | h;
      ADD_EXPRESSION(next_expressions_size, ft4)

      const uint32_t ft5 = not_g & h;
      ADD_EXPRESSION(next_expressions_size, ft5)
    }
  }

  // just to get the initial branch before the algorithm even starts
  chain_size--;
  CAPTURE_STATS_CALL
  chain_size++;

  if (chunk_mode) {
    find_optimal_chain_restore_progress(chain_size, NUM_TARGETS,
                                        next_expressions_size, 0);
  } else {
    find_optimal_chain(chain_size, NUM_TARGETS, next_expressions_size, 0);
  }

  return 0;
}
