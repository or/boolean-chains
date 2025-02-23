#include <bitset>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <set>
using namespace std;

#define SMART 1

const uint32_t N = 16;
const uint32_t SIZE = ((1 << (N - 1)) + 31) / 32;
const uint32_t MAX_LENGTH = 25;
const uint32_t TAUTOLOGY = (1 << N) - 1;
const uint32_t TARGET_1 =
    ((~(uint32_t)0b1011011111100011) >> (16 - N)) & TAUTOLOGY;
const uint32_t TARGET_2 =
    ((~(uint32_t)0b1111100111100100) >> (16 - N)) & TAUTOLOGY;
const uint32_t TARGET_3 =
    ((~(uint32_t)0b1101111111110100) >> (16 - N)) & TAUTOLOGY;
const uint32_t TARGET_4 =
    ((~(uint32_t)0b1011011011011110) >> (16 - N)) & TAUTOLOGY;
const uint32_t TARGET_5 =
    ((~(uint32_t)0b1010001010111111) >> (16 - N)) & TAUTOLOGY;
const uint32_t TARGET_6 =
    ((~(uint32_t)0b1000111111110011) >> (16 - N)) & TAUTOLOGY;
const uint32_t TARGET_7 =
    (((uint32_t)0b0011111011111111) >> (16 - N)) & TAUTOLOGY;
const uint32_t TARGETS[] = {
    TARGET_1, TARGET_2, TARGET_3, TARGET_4, TARGET_5, TARGET_6, TARGET_7,
};
const uint32_t NUM_TARGETS = sizeof(TARGETS) / sizeof(uint32_t);

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

bool choices_vector_equals_start_indices(vector<uint32_t> &choices,
                                         vector<uint32_t> &start_indices) {
  if (choices.size() > start_indices.size()) {
    return false;
  }
  for (int i = 0; i < choices.size(); i++) {
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

inline void add_new_expression(set<uint32_t> &new_expressions, uint32_t value,
                               const uint32_t *seen) {
  if (bit_set_get(seen, value)) {
    return;
  }

  new_expressions.insert(value);
}

void find_optimal_chain(uint32_t *chain, size_t &chain_size,
                        size_t &current_best_length, vector<uint32_t> &choices,
                        vector<uint32_t> &start_indices, uint32_t *seen,
                        size_t num_fulfilled_target_functions,
                        uint64_t &total_chains, time_t &last_print,
                        size_t max_length, bool &progress_check_done) {
  total_chains++;
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

  if (chain_size + NUM_TARGETS - num_fulfilled_target_functions > max_length) {
    return;
  }

  set<uint32_t> new_expressions;

  for (size_t j = 0; j < chain_size; j++) {
    for (size_t k = j + 1; k < chain_size; k++) {
      const uint32_t &g = chain[j];
      const uint32_t &h = chain[k];

      add_new_expression(new_expressions, g & h, seen);
      add_new_expression(new_expressions, (~g) & h, seen);
      add_new_expression(new_expressions, g & (~h), seen);
      add_new_expression(new_expressions, g | h, seen);
      add_new_expression(new_expressions, g ^ h, seen);
    }
  }

  size_t current_length = chain_size;
  size_t start_index_offset = choices.size();
#if SMART
  uint32_t clean_up[1000];
  size_t clean_up_size = 0;
#endif
  size_t i = 0;
  for (const uint32_t &ft : new_expressions) {
#if SMART
    bit_set_insert(seen, ft);
    clean_up[clean_up_size] = ft;
    clean_up_size++;
#endif

    if (!progress_check_done) {
      if (choices_vector_equals_start_indices(choices, start_indices) &&
          start_index_offset < start_indices.size() &&
          i < start_indices[start_index_offset]) {
        i++;
        continue;
      }
      if (choices.size() > start_indices.size()) {
        progress_check_done = true;
      }
    }

    choices.push_back(i);
    chain[chain_size] = ft;
    chain_size++;
    if (time(NULL) >= last_print + 10) {
      for (size_t j = 0; j < choices.size(); ++j) {
        cout << choices[j];
        if (j != choices.size() - 1) {
          cout << ", ";
        }
      }
      cout << " [best: " << current_best_length << "] " << total_chains << endl;
      last_print = time(NULL);
      // exit(1);
    }

    size_t new_num_fulfilled = num_fulfilled_target_functions;
    if (ft == TARGET_1 || ft == TARGET_2 || ft == TARGET_3 || ft == TARGET_4 ||
        ft == TARGET_5 || ft == TARGET_6 || ft == TARGET_7) {
      new_num_fulfilled++;
    }
#if SMART != 1
    bit_set_insert(seen, ft);
#endif
    find_optimal_chain(chain, chain_size, current_best_length, choices,
                       start_indices, seen, new_num_fulfilled, total_chains,
                       last_print, max_length, progress_check_done);
#if SMART != 1
    bit_set_remove(seen, ft);
#endif
    choices.pop_back();
    chain_size--;
    i++;
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
  vector<uint32_t> choices;
  choices.reserve(25);
  uint64_t total_chains = 0;
  uint32_t seen[SIZE] = {0};
  bit_set_insert(seen, 0);
  for (int i = 0; i < chain_size; i++) {
    bit_set_insert(seen, chain[i]);
  }
  time_t last_print = time(NULL);
  bool progress_check_done = false;
  find_optimal_chain(chain, chain_size, current_best_length, choices,
                     start_indices, seen, 0, total_chains, last_print,
                     MAX_LENGTH, progress_check_done);

  cout << "total chains: " << total_chains << endl;
  return 0;
}
