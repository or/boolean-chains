#include <bitset>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace std;

#define CAPTURE_STATS 1

constexpr uint32_t N = 10;
constexpr uint32_t SIZE = ((1 << (N - 1)) + 31) / 32;
constexpr uint32_t MAX_LENGTH = 15;
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
uint32_t chain[25];
size_t current_best_length = 1000;
uint32_t choices[30];
uint64_t total_chains = 0;
uint32_t seen[SIZE] = {0};
bool progress_check_done = false;

#if CAPTURE_STATS
uint64_t stats_total_num_new_expressions[25] = {0};
uint32_t stats_min_num_new_expressions[25] = {0};
uint32_t stats_max_num_new_expressions[25] = {0};
uint64_t stats_num_data_points[25] = {0};
#endif

inline uint32_t bit_set_get(const uint32_t *bit_set, uint32_t bit) {
  const uint32_t index = bit >> 5;
  const uint32_t bit_index = bit & 0b11111;
  return bit_set[index] & (1 << bit_index);
}

inline void bit_set_insert(uint32_t *bit_set, uint32_t bit) {
  const uint32_t index = bit >> 5;
  const uint32_t bit_index = bit & 0b11111;
  bit_set[index] |= (1 << bit_index);
}

inline void bit_set_remove(uint32_t *bit_set, uint32_t bit) {
  const uint32_t index = bit >> 5;
  const uint32_t bit_index = bit & 0b11111;
  bit_set[index] &= ~(1 << bit_index);
}

int compare_choices_with_start_indices(const size_t choices_size) {
  int max_i = choices_size <= start_indices.size() ? choices_size
                                                   : start_indices.size();
  for (int i = 0; i < max_i; i++) {
    if (choices[i] < start_indices[i]) {
      return -1;
    } else if (choices[i] > start_indices[i]) {
      return 1;
    }
  }
  if (choices_size > start_indices.size()) {
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
    if (bit_set_get(TARGET_LOOKUP, chain[i])) {
      cout << " [target]";
    }
    cout << endl;
  }
  cout << endl;
}

inline void add_new_expression(uint32_t *new_expressions,
                               size_t &new_expressions_size, uint32_t value,
                               uint32_t *tmp_seen) {
  if (bit_set_get(seen, value)) {
    return;
  }

  if (bit_set_get(tmp_seen, value)) {
    return;
  }
  bit_set_insert(tmp_seen, value);

  new_expressions[new_expressions_size] = value;
  new_expressions_size++;
}

void find_optimal_chain(const size_t chain_size, const size_t choices_size,
                        const size_t num_fulfilled_target_functions) {
  total_chains++;
  if ((total_chains & 0xfffffff) == 0) {
    for (size_t j = 0; j < choices_size; ++j) {
      cout << choices[j];
      if (j != choices_size - 1) {
        cout << ", ";
      }
    }
    cout << " [best: " << current_best_length << "] " << total_chains << endl;
    // exit(1);
  }

  if (chain_size + NUM_TARGETS - num_fulfilled_target_functions > MAX_LENGTH) {
    return;
  }

  if (num_fulfilled_target_functions == NUM_TARGETS) {
    if (chain_size < current_best_length) {
      cout << "New best chain found (" << chain_size << "):" << endl;
      print_chain(chain_size);
      current_best_length = chain_size;
    } else if (chain_size == current_best_length) {
      print_chain(chain_size);
    }
    return;
  }

  uint32_t new_expressions[1000];
  size_t new_expressions_size = 0;
  uint32_t tmp_seen[SIZE] = {0};

  for (size_t j = 0; j < chain_size; j++) {
    const uint32_t g = chain[j];
    const uint32_t not_g = ~g;

    for (size_t k = j + 1; k < chain_size; k++) {
      const uint32_t h = chain[k];
      const uint32_t not_h = ~h;

      const uint32_t expressions[] = {g & h, not_g & h, g & not_h, g | h,
                                      g ^ h};

      for (uint32_t expr : expressions) {
        add_new_expression(new_expressions, new_expressions_size, expr,
                           tmp_seen);
      }
    }
  }

  uint32_t clean_up[1000];
  size_t clean_up_size = 0;
  size_t next_chain_size = chain_size + 1;
  size_t next_choices_size = choices_size + 1;
  int start_i = 0;
  if (!progress_check_done) {
    choices[choices_size] = 0;
    int result = compare_choices_with_start_indices(next_choices_size);
    if (result < 0 && choices_size < start_indices.size()) {
      start_i = start_indices[choices_size];
      cout << "skipping to ";
      for (size_t j = 0; j < choices_size; ++j) {
        cout << choices[j] << ", ";
      }
      cout << start_i << endl;

      for (int i = 0; i < start_i; i++) {
        const auto &ft = new_expressions[i];
        bit_set_insert(seen, ft);
        clean_up[clean_up_size] = ft;
        clean_up_size++;
      }
    }
    if (result > 0) {
      progress_check_done = true;
    }
  }
#if CAPTURE_STATS
  if (progress_check_done) {
    stats_total_num_new_expressions[chain_size] += new_expressions_size;
    if (stats_num_data_points[chain_size] == 0 ||
        new_expressions_size < stats_min_num_new_expressions[chain_size]) {
      stats_min_num_new_expressions[chain_size] = new_expressions_size;
    }
    if (stats_num_data_points[chain_size] == 0 ||
        new_expressions_size > stats_max_num_new_expressions[chain_size]) {
      stats_max_num_new_expressions[chain_size] = new_expressions_size;
    }
    stats_num_data_points[chain_size]++;
  }
#endif

  for (int i = start_i; i < new_expressions_size; i++) {
    const auto &ft = new_expressions[i];
    bit_set_insert(seen, ft);
    clean_up[clean_up_size] = ft;
    clean_up_size++;

    choices[choices_size] = i;
    chain[chain_size] = ft;

    find_optimal_chain(next_chain_size, next_choices_size,
                       bit_set_get(TARGET_LOOKUP, ft)
                           ? num_fulfilled_target_functions + 1
                           : num_fulfilled_target_functions);
  }

  for (int i = 0; i < clean_up_size; ++i) {
    bit_set_remove(seen, clean_up[i]);
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
  for (int i = 4; i < MAX_LENGTH; i++) {
    printf("%2d: %16llu %25llu %16llu %16u %16u\n", i, stats_num_data_points[i],
           stats_total_num_new_expressions[i],
           stats_total_num_new_expressions[i] / stats_num_data_points[i],
           stats_min_num_new_expressions[i], stats_max_num_new_expressions[i]);
  }
  cout << flush;
#endif
}

void signal_handler(int signal) { exit(signal); }

int main(int argc, char *argv[]) {
  atexit(on_exit);
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  cout << "N = " << N << ", MAX_LENGTH: " << MAX_LENGTH
       << ", CAPTURE_STATS: " << CAPTURE_STATS << endl;
  for (int i = 1; i < argc; i++) {
    start_indices.push_back(atoi(argv[i]));
  }

  cout << NUM_TARGETS << " targets:" << endl;
  for (int i = 0; i < NUM_TARGETS; i++) {
    cout << "  " << bitset<N>(TARGETS[i]).to_string() << endl;
    bit_set_insert(TARGET_LOOKUP, TARGETS[i]);
  }

  chain[0] = 0b0000000011111111 >> (16 - N);
  chain[1] = 0b0000111100001111 >> (16 - N);
  chain[2] = 0b0011001100110011 >> (16 - N);
  chain[3] = 0b0101010101010101 >> (16 - N);

  bit_set_insert(seen, 0);
  for (int i = 0; i < 4; i++) {
    bit_set_insert(seen, chain[i]);
  }
  find_optimal_chain(4, 0, 0);

  return 0;
}
