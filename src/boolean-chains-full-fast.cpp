#include <bitset>
#include <cstdint>
#include <iostream>
using namespace std;

#define SMART 1

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

bool choices_vector_equals_start_indices(uint32_t *choices,
                                         size_t &choices_size,
                                         vector<uint32_t> &start_indices) {
  if (choices_size > start_indices.size()) {
    return false;
  }
  for (int i = 0; i < choices_size; i++) {
    if (choices[i] != start_indices[i]) {
      return false;
    }
  }
  return true;
}

void print_chain(uint32_t *chain, size_t &chain_size) {
  cout << "chain (" << chain_size << "):" << endl;
  for (int i = 0; i < chain_size; i++) {
    cout << bitset<N>(chain[i]).to_string() << endl;
  }
  cout << endl;
}

inline void add_new_expression(uint32_t *new_expressions,
                               size_t &new_expressions_size, uint32_t value,
                               const uint32_t *seen, uint32_t *tmp_seen) {
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

void find_optimal_chain(uint32_t *chain, size_t &chain_size,
                        size_t &current_best_length, uint32_t *choices,
                        size_t &choices_size, vector<uint32_t> &start_indices,
                        uint32_t *seen, size_t num_fulfilled_target_functions,
                        uint64_t &total_chains, size_t max_length,
                        bool &progress_check_done) {
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

  if (chain_size + NUM_TARGETS - num_fulfilled_target_functions > max_length) {
    return;
  }

  if (num_fulfilled_target_functions == NUM_TARGETS) {
    if (chain_size < current_best_length) {
      cout << "New best chain found (" << chain_size << "):" << endl;
      print_chain(chain, chain_size);
      current_best_length = chain_size;
    } else if (chain_size == current_best_length) {
      print_chain(chain, chain_size);
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
        add_new_expression(new_expressions, new_expressions_size, expr, seen,
                           tmp_seen);
      }
    }
  }

  size_t start_index_offset = choices_size;
#if SMART
  uint32_t clean_up[1000];
  size_t clean_up_size = 0;
#endif
  size_t next_chain_size = chain_size + 1;
  size_t next_choices_size = choices_size + 1;
  for (int i = 0; i < new_expressions_size; i++) {
    const auto &ft = new_expressions[i];
#if SMART
    bit_set_insert(seen, ft);
    clean_up[clean_up_size] = ft;
    clean_up_size++;
#endif

    if (!progress_check_done) {
      if (choices_vector_equals_start_indices(choices, choices_size,
                                              start_indices) &&
          choices_size < start_indices.size() &&
          i < start_indices[choices_size]) {
        continue;
      }
      if (choices_size >= start_indices.size()) {
        progress_check_done = true;
      }
    }

    choices[choices_size] = i;
    chain[chain_size] = ft;

    size_t new_num_fulfilled = num_fulfilled_target_functions;
    if (bit_set_get(TARGET_LOOKUP, ft)) {
      new_num_fulfilled++;
    }
#if SMART != 1
    bit_set_insert(seen, ft);
#endif
    find_optimal_chain(chain, next_chain_size, current_best_length, choices,
                       next_choices_size, start_indices, seen,
                       new_num_fulfilled, total_chains, max_length,
                       progress_check_done);
#if SMART != 1
    bit_set_remove(seen, ft);
#endif
  }

#if SMART
  for (int i = 0; i < clean_up_size; ++i) {
    bit_set_remove(seen, clean_up[i]);
  }
#endif
}

int main(int argc, char *argv[]) {
  // parse a vector of integers passed as arguments
  vector<uint32_t> start_indices;
  for (int i = 1; i < argc; i++) {
    start_indices.push_back(atoi(argv[i]));
  }

  cout << NUM_TARGETS << " targets:" << endl;
  for (int i = 0; i < NUM_TARGETS; i++) {
    cout << "  " << bitset<N>(TARGETS[i]).to_string() << endl;
    bit_set_insert(TARGET_LOOKUP, TARGETS[i]);
  }

  uint32_t chain[25];
  size_t chain_size = 0;
  chain[chain_size] = 0b0000000011111111 >> (16 - N);
  chain_size++;
  chain[chain_size] = 0b0000111100001111 >> (16 - N);
  chain_size++;
  chain[chain_size] = 0b0011001100110011 >> (16 - N);
  chain_size++;
  chain[chain_size] = 0b0101010101010101 >> (16 - N);
  chain_size++;

  size_t current_best_length = 1000;
  uint32_t choices[30];
  size_t choices_size = 0;
  uint64_t total_chains = 0;
  uint32_t seen[SIZE] = {0};
  bit_set_insert(seen, 0);
  for (int i = 0; i < chain_size; i++) {
    bit_set_insert(seen, chain[i]);
  }
  bool progress_check_done = false;
  find_optimal_chain(chain, chain_size, current_best_length, choices,
                     choices_size, start_indices, seen, 0, total_chains,
                     MAX_LENGTH, progress_check_done);

  cout << "total chains: " << total_chains << endl;
  return 0;
}
