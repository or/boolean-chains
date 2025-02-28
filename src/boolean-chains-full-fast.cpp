#include <bitset>
#include <cinttypes>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>
using namespace std;

#ifndef CAPTURE_STATS
#define CAPTURE_STATS 1
#endif

#ifndef PLAN_MODE
#define PLAN_MODE 0
#endif

constexpr uint32_t N = 13;
constexpr uint32_t SIZE = ((1 << (N - 1)) + 31) / 32;
constexpr uint32_t MAX_LENGTH = 19;
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

vector<uint32_t> start_indices;
size_t start_indices_size;
uint32_t chain[25];
size_t current_best_length = 1000;
uint32_t choices[30];
uint64_t total_chains = 0;
uint32_t seen[SIZE];
bool progress_check_done = false;
uint32_t expressions[1000];
bool chunk_mode = false;
size_t start_chain_length = 0;

#if CAPTURE_STATS
uint64_t stats_total_num_expressions[25] = {0};
uint32_t stats_min_num_expressions[25] = {0};
uint32_t stats_max_num_expressions[25] = {0};
uint64_t stats_num_data_points[25] = {0};
#endif

#if PLAN_MODE
size_t plan_depth = 1;
#endif

inline uint32_t target_lookup_get(uint32_t bit) {
  const uint32_t index = bit >> 5;
  const uint32_t bit_index = bit & 0b11111;
  return (TARGET_LOOKUP[index] >> bit_index) & 1;
}

inline void target_lookup_insert(uint32_t bit) {
  const uint32_t index = bit >> 5;
  const uint32_t bit_index = bit & 0b11111;
  TARGET_LOOKUP[index] |= (1 << bit_index);
}

inline uint32_t seen_get(uint32_t bit) {
  const uint32_t index = bit >> 5;
  const uint32_t bit_index = bit & 0b11111;
  return (seen[index] >> bit_index) & 1;
}

inline void seen_insert(uint32_t bit) {
  const uint32_t index = bit >> 5;
  const uint32_t bit_index = bit & 0b11111;
  seen[index] |= (1 << bit_index);
}

inline bool seen_insert_if_not_present(uint32_t bit) {
  const uint32_t index = bit >> 5;
  const uint32_t bit_index = bit & 0b11111;
  if ((seen[index] >> bit_index) & 1) {
    return false;
  }
  seen[index] |= (1 << bit_index);
  return true;
}

inline void seen_remove(uint32_t bit) {
  const uint32_t index = bit >> 5;
  const uint32_t bit_index = bit & 0b11111;
  seen[index] &= ~(1 << bit_index);
}

#define ADD_EXPRESSION(expressions_size, value)                                \
  {                                                                            \
    const uint32_t index = value >> 5;                                         \
    const uint32_t bit_index = value & 0b11111;                                \
    const uint32_t shifted = 1 << bit_index;                                   \
    if (!(seen[index] & shifted)) {                                            \
      seen[index] |= shifted;                                                  \
      expressions[expressions_size] = value;                                   \
      expressions_size++;                                                      \
    }                                                                          \
  }

int compare_choices_with_start_indices(const size_t chain_size) {
  int max_i =
      chain_size <= start_indices_size ? chain_size : start_indices_size;
  for (int i = start_chain_length; i < max_i; i++) {
    if (choices[i] < start_indices[i]) {
      return -1;
    } else if (choices[i] > start_indices[i]) {
      return 1;
    }
  }
  if (chain_size > start_indices_size) {
    return 1;
  }
  return 0;
}

void print_chain(const size_t chain_size) {
  cout << "chain (" << chain_size << "):" << endl;
  for (int i = 0; i < chain_size; i++) {
    cout << "x" << i + 1;
    for (int j = 0; j < i; j++) {
      for (int k = j + 1; k < i; k++) {
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
    if (target_lookup_get(chain[i])) {
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

  size_t new_expressions_size = expressions_size;
  const uint32_t h = chain[chain_size - 1];
  const uint32_t not_h = ~h;
  for (size_t j = 0; j < chain_size - 1; j++) {
    const uint32_t g = chain[j];
    const uint32_t not_g = ~g;

    const uint32_t ft1 = g & h;
    ADD_EXPRESSION(new_expressions_size, ft1)

    const uint32_t ft2 = g & not_h;
    ADD_EXPRESSION(new_expressions_size, ft2)

    const uint32_t ft3 = g ^ h;
    ADD_EXPRESSION(new_expressions_size, ft3)

    const uint32_t ft4 = g | h;
    ADD_EXPRESSION(new_expressions_size, ft4)

    const uint32_t ft5 = not_g & h;
    ADD_EXPRESSION(new_expressions_size, ft5)
  }

  size_t next_chain_size = chain_size + 1;
  int start_i = expressions_index;
  if (!progress_check_done) {
    choices[chain_size] = 0;
    int result = compare_choices_with_start_indices(next_chain_size);
    if (result < 0 && chain_size < start_indices_size) {
      start_i = start_indices[chain_size];
      cout << "skipping to ";
      for (size_t j = start_chain_length; j < chain_size; ++j) {
        cout << choices[j] << ", ";
      }
      cout << start_i << endl;
    }
    if (result > 0) {
      progress_check_done = true;
    }
  }

#if CAPTURE_STATS
  if (progress_check_done) {
    const auto new_expressions = new_expressions_size - expressions_size;
    stats_total_num_expressions[chain_size] += new_expressions;
    if (stats_num_data_points[chain_size] == 0 ||
        new_expressions < stats_min_num_expressions[chain_size]) {
      stats_min_num_expressions[chain_size] = new_expressions;
    }
    if (stats_num_data_points[chain_size] == 0 ||
        new_expressions > stats_max_num_expressions[chain_size]) {
      stats_max_num_expressions[chain_size] = new_expressions;
    }
    stats_num_data_points[chain_size]++;
  }
#endif

  choices[chain_size] = start_i;
  for (int i = start_i; i < new_expressions_size; i++, choices[chain_size]++) {
    chain[chain_size] = expressions[i];
    const uint32_t next_num_unfulfilled_targets =
        num_unfulfilled_target_functions - target_lookup_get(chain[chain_size]);

    total_chains++;
    if ((total_chains & 0xfffffff) == 0) {
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
      continue;
    }

    if (!next_num_unfulfilled_targets) {
      if (next_chain_size < current_best_length) {
        cout << "New best chain found (" << next_chain_size << "):" << endl;
        print_chain(next_chain_size);
        current_best_length = next_chain_size;
      } else if (next_chain_size == current_best_length) {
        print_chain(next_chain_size);
      }
      continue;
    }

    find_optimal_chain(next_chain_size, next_num_unfulfilled_targets,
                       new_expressions_size, i + 1);
  }

  if (chunk_mode && chain_size == start_indices_size) {
    cout << "completed chunk" << endl;
    exit(0);
  }

  for (int i = expressions_size; i < new_expressions_size; i++) {
    seen_remove(expressions[i]);
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
  for (int i = start_chain_length; i < MAX_LENGTH; i++) {
    printf("%2d: %16" PRIu64 " %25" PRIu64 " %16" PRIu64 " %16" PRId32
           " %16" PRIu32 "\n",
           i, stats_num_data_points[i], stats_total_num_expressions[i],
           stats_num_data_points[i] == 0
               ? 0
               : stats_total_num_expressions[i] / stats_num_data_points[i],
           stats_min_num_expressions[i], stats_max_num_expressions[i]);
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

  int start_i = 1;
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
  for (int i = 0; i < NUM_TARGETS; i++) {
#if PLAN_MODE != 1
    cout << "  " << bitset<N>(TARGETS[i]).to_string() << endl;
#endif
    target_lookup_insert(TARGETS[i]);
  }

  chain[0] = 0b0000000011111111 >> (16 - N);
  chain[1] = 0b0000111100001111 >> (16 - N);
  chain[2] = 0b0011001100110011 >> (16 - N);
  chain[3] = 0b0101010101010101 >> (16 - N);
  size_t chain_size = 4;

  for (int i = 0; i < chain_size; i++) {
    start_indices.push_back(0);
  }

#if PLAN_MODE != 1
  // read the progress vector, e.g 5 2 9, commas will be ignored: 5, 2, 9
  for (int i = start_i; i < argc; i++) {
    start_indices.push_back(atoi(argv[i]));
  }
  start_indices_size = start_indices.size();
  seen_insert(0);
  for (int i = 0; i < chain_size; i++) {
    seen_insert(chain[i]);
  }
#endif

  size_t expressions_size = 0;
  for (size_t k = 1; k < chain_size - 1; k++) {
    const uint32_t h = chain[k];
    const uint32_t not_h = ~h;
    for (size_t j = 0; j < k; j++) {
      const uint32_t g = chain[j];
      const uint32_t not_g = ~g;

      const uint32_t ft1 = g & h;
      ADD_EXPRESSION(expressions_size, ft1)

      const uint32_t ft2 = g & not_h;
      ADD_EXPRESSION(expressions_size, ft2)

      const uint32_t ft3 = g ^ h;
      ADD_EXPRESSION(expressions_size, ft3)

      const uint32_t ft4 = g | h;
      ADD_EXPRESSION(expressions_size, ft4)

      const uint32_t ft5 = not_g & h;
      ADD_EXPRESSION(expressions_size, ft5)
    }
  }

  start_chain_length = chain_size;
  find_optimal_chain(chain_size, NUM_TARGETS, expressions_size, 0);

  return 0;
}
