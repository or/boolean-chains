#include "bit_set_fast.h"
#include <algorithm>
#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <execinfo.h>
#include <tuple>
#include <vector>
using namespace std;

uint32_t N = 16;
constexpr uint32_t MAX_LENGTH = 25;
constexpr uint32_t SIZE = 1 << 15;
constexpr uint32_t TAUTOLOGY = (1 << 16) - 1;
constexpr uint32_t TARGET_1 = ((~(uint32_t)0b1011011111100011)) & TAUTOLOGY;
constexpr uint32_t TARGET_2 = ((~(uint32_t)0b1111100111100100)) & TAUTOLOGY;
constexpr uint32_t TARGET_3 = ((~(uint32_t)0b1101111111110100)) & TAUTOLOGY;
constexpr uint32_t TARGET_4 = ((~(uint32_t)0b1011011011011110)) & TAUTOLOGY;
constexpr uint32_t TARGET_5 = ((~(uint32_t)0b1010001010111111)) & TAUTOLOGY;
constexpr uint32_t TARGET_6 = ((~(uint32_t)0b1000111111110011)) & TAUTOLOGY;
constexpr uint32_t TARGET_7 = (((uint32_t)0b0011111011111111)) & TAUTOLOGY;
uint32_t TARGETS[] = {
    TARGET_1, TARGET_2, TARGET_3, TARGET_4, TARGET_5, TARGET_6, TARGET_7,
};
constexpr uint32_t NUM_TARGETS = sizeof(TARGETS) / sizeof(uint32_t);

BitSet footprints[SIZE];
uint32_t costs[SIZE] __attribute__((aligned(64))) = {0};
uint32_t levels[50][50000];
size_t levels_size[10] = {0};
uint32_t frequencies[50000];
uint32_t target_lookup[SIZE] __attribute__((aligned(64))) = {0};

#define ADD_FIRST_EXPRESSION(value)                                            \
  {                                                                            \
    uint32_t f = value;                                                        \
    if (costs[f] == 0xffffffff) {                                              \
      costs[f] = 1;                                                            \
      levels[1][levels_size[1]] = f;                                           \
      footprints[f].insert(levels_size[1]);                                    \
      levels_size[1]++;                                                        \
      c--;                                                                     \
    }                                                                          \
  }

#define GENERATE_NEW_EXPRESSIONS                                               \
  algorithm_l_with_footprints(chain, chain_size);                              \
                                                                               \
  expressions_size[chain_size] = levels_size[1];                               \
  memcpy(expressions[chain_size], levels[1],                                   \
         sizeof(uint32_t) * expressions_size[chain_size]);                     \
                                                                               \
  priorities[chain_size].clear();                                              \
  for (size_t i = 0; i < expressions_size[chain_size]; i++) {                  \
    priorities[chain_size].push_back(i);                                       \
  }                                                                            \
                                                                               \
  count_first_expressions_in_footprints(expressions_size[chain_size]);         \
                                                                               \
  sort(priorities[chain_size].begin(), priorities[chain_size].end(),           \
       [&](size_t x, size_t y) { return get_priority(x) < get_priority(y); });

void erase(uint32_t *array, size_t &array_size, uint32_t f) {
  for (size_t i = 0; i < array_size; i++) {
    if (array[i] == f) {
      if (i < array_size - 1) {
        array[i] = array[array_size - 1];
      }
      array_size--;
      return;
    }
  }
}

void generate_first_expressions(const uint32_t *chain, const size_t chain_size,
                                uint32_t &c) {
  // U1. Initialize
  levels_size[0] = 0;
  for (size_t i = 0; i < chain_size; i++) {
    costs[chain[i]] = 0;
    levels[0][levels_size[0]] = chain[i];
    levels_size[0]++;
    c--;
  }

  // U2. Iterate through all pairs of chain expressions
  levels_size[1] = 0;
  for (size_t j = 0; j < chain_size; j++) {
    uint32_t g = chain[j];
    uint32_t not_g = ~g;
    for (size_t k = j + 1; k < chain_size; k++) {
      uint32_t h = chain[k];
      uint32_t not_h = ~h;

      ADD_FIRST_EXPRESSION(g & h);
      ADD_FIRST_EXPRESSION(not_g & h);
      ADD_FIRST_EXPRESSION(g & not_h);
      ADD_FIRST_EXPRESSION(g | h);
      ADD_FIRST_EXPRESSION(g ^ h);
    }
  }
}
void algorithm_l_with_footprints(const uint32_t *chain,
                                 const size_t chain_size) {
  memset(costs, 0xff, sizeof(costs));
  memset(footprints, 0, sizeof(footprints));
  uint32_t c = 1 << (N - 1);

  // for 0x00000000
  costs[0] = 0;
  c--;

  generate_first_expressions(chain, chain_size, c);

  // U3. Loop over r = 2, 3, ... while c > 0
  uint32_t r;
  for (r = 2; c > 0; ++r) {
    levels_size[r] = 0;

    bool all_targets_found = true;
    for (size_t i = 0; i < NUM_TARGETS; i++) {
      if (costs[TARGETS[i]] == 0xffffffff) {
        all_targets_found = false;
        break;
      }
    }

    if (all_targets_found) {
      break;
    }

    // U4. Loop over j = [(r-1)/2], ..., 0, k = r - 1 - j
    for (int j = (r - 1) / 2; j >= 0; --j) {
      uint32_t k = r - 1 - j;

      for (size_t gi = 0; gi < levels_size[j]; ++gi) {
        uint32_t g = levels[j][gi];
        uint32_t not_g = ~g;
        size_t start_hi = (j == k) ? gi + 1 : 0;

        for (size_t hi = start_hi; hi < levels_size[k]; ++hi) {
          uint32_t h = levels[k][hi];
          uint32_t not_h = ~h;

          BitSet v(footprints[g]);
          uint32_t u;

          if (footprints[g].is_disjoint(footprints[h])) {
            u = r;
            v.add(footprints[h]);
          } else {
            u = r - 1;
            v.intersect(footprints[h]);
          }

          for (const uint32_t &f :
               {g & h, not_g & h, g & not_h, g | h, g ^ h}) {
            if (costs[f] == 0xffffffff) {
              costs[f] = u;
              levels[u][levels_size[u]] = f;
              levels_size[u]++;
              footprints[f] = v;
              c--;
            } else if (costs[f] > u) {
              const uint32_t previous_u = costs[f];
              erase(levels[previous_u], levels_size[previous_u], f);
              costs[f] = u;
              levels[u][levels_size[u]] = f;
              levels_size[u]++;
              footprints[f] = v;
            } else if (costs[f] == u) {
              footprints[f].add(v);
            }
          }
        }
      }
    }
  }
}

void count_first_expressions_in_footprints(const uint32_t expressions_size) {
  memset(frequencies, 0, sizeof(uint32_t) * expressions_size);

  for (const uint32_t &f : TARGETS) {
    for (size_t i = 0; i < expressions_size; i++) {
      if (footprints[f].get(i)) {
        frequencies[i]++;
      }
    }
  }
}

auto get_priority(const size_t index) {
  int min_value = 1000;
  for (const auto &y : TARGETS) {
    if (footprints[y].get(index)) {
      min_value = min(min_value, static_cast<int>(costs[y]));
    }
  }
  return make_tuple(min_value, -static_cast<int>(frequencies[index]),
                    -static_cast<int>(index));
};

int main(int argc, char *argv[]) {
  uint32_t dummy_c = 0;
  uint32_t chain[MAX_LENGTH] __attribute__((aligned(64)));
  uint32_t num_unfulfilled_targets = NUM_TARGETS;
  uint32_t expressions[MAX_LENGTH][50000] __attribute__((aligned(64)));
  uint32_t expressions_size[MAX_LENGTH] __attribute__((aligned(64)));
  vector<uint32_t> priorities[MAX_LENGTH] __attribute__((aligned(64)));
  uint32_t choices[MAX_LENGTH] __attribute__((aligned(64)));
  size_t current_best_length = 1000;
  size_t solution_size = 0;
  uint32_t solution[MAX_LENGTH] = {0};

  size_t start_i = 1;
  N = atoi(argv[start_i++]);

  for (size_t i = start_i; i < argc; i++) {
    solution[solution_size++] = strtol(argv[i], NULL, 2);
  }

  for (size_t i = 0; i < NUM_TARGETS; i++) {
    TARGETS[i] = TARGETS[i] >> (16 - N);
    target_lookup[TARGETS[i]] = 1;
  }

  chain[0] = 0b0000000011111111 >> (16 - N);
  chain[1] = 0b0000111100001111 >> (16 - N);
  chain[2] = 0b0011001100110011 >> (16 - N);
  chain[3] = 0b0101010101010101 >> (16 - N);
  size_t chain_size = 4;

  for (size_t i = 0; i < solution_size; i++) {

    GENERATE_NEW_EXPRESSIONS

    bool found = false;
    for (size_t j = 0; j < priorities[chain_size].size(); j++) {
      if (expressions[chain_size][priorities[chain_size][j]] == solution[i]) {
        choices[i] = j;
        found = true;
        break;
      }
    }
    if (!found) {
      printf("error: couldn't find the index for function number %zu: %s\n", i,
             bitset<16>(solution[i]).to_string().c_str());
      exit(-1);
    }

    chain[chain_size] =
        expressions[chain_size][priorities[chain_size][choices[i]]];

    chain_size++;
  }

  for (size_t i = 0; i < solution_size; i++) {
    printf("%d ", choices[i]);
  }
  printf("\n");

  return 0;
}
