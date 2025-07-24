#include <bitset>
#include <cstdint>
#include <cstdlib>

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

uint32_t start_chain_length;
uint64_t total_chains = 0;

#define ADD_EXPRESSION(value, chain_size)                                      \
  expressions[expressions_size[chain_size]] = value;                           \
  expressions_size[chain_size] += unseen[value];                               \
  unseen[value] = 0;

#define GENERATE_NEW_EXPRESSIONS(chain_size, add_expression)                   \
  {                                                                            \
    expressions_size[chain_size] = expressions_size[chain_size - 1];           \
    const uint32_t h = chain[chain_size - 1];                                  \
    const uint32_t not_h = not_chain[chain_size - 1];                          \
                                                                               \
    uint32_t j = 0;                                                            \
    _Pragma("loop unroll(full)") for (; j < chain_size - 4; j += 4) {          \
      const uint32_t g0 = chain[j], g1 = chain[j + 1], g2 = chain[j + 2],      \
                     g3 = chain[j + 3];                                        \
      const uint32_t not_g0 = not_chain[j], not_g1 = not_chain[j + 1],         \
                     not_g2 = not_chain[j + 2], not_g3 = not_chain[j + 3];     \
                                                                               \
      add_expression(g0 & h, chain_size);                                      \
      add_expression(g1 & h, chain_size);                                      \
      add_expression(g2 & h, chain_size);                                      \
      add_expression(g3 & h, chain_size);                                      \
                                                                               \
      add_expression(not_g0 & h, chain_size);                                  \
      add_expression(not_g1 & h, chain_size);                                  \
      add_expression(not_g2 & h, chain_size);                                  \
      add_expression(not_g3 & h, chain_size);                                  \
                                                                               \
      add_expression(g0 & not_h, chain_size);                                  \
      add_expression(g1 & not_h, chain_size);                                  \
      add_expression(g2 & not_h, chain_size);                                  \
      add_expression(g3 & not_h, chain_size);                                  \
                                                                               \
      add_expression(g0 ^ h, chain_size);                                      \
      add_expression(g1 ^ h, chain_size);                                      \
      add_expression(g2 ^ h, chain_size);                                      \
      add_expression(g3 ^ h, chain_size);                                      \
                                                                               \
      add_expression(g0 | h, chain_size);                                      \
      add_expression(g1 | h, chain_size);                                      \
      add_expression(g2 | h, chain_size);                                      \
      add_expression(g3 | h, chain_size);                                      \
    }                                                                          \
                                                                               \
    _Pragma("loop unroll(full)") for (; j < chain_size - 1; j++) {             \
      const uint32_t g = chain[j];                                             \
      const uint32_t not_g = not_chain[j];                                     \
      add_expression(g & h, chain_size);                                       \
      add_expression(not_g & h, chain_size);                                   \
      add_expression(g & not_h, chain_size);                                   \
      add_expression(g ^ h, chain_size);                                       \
      add_expression(g | h, chain_size);                                       \
    }                                                                          \
  }

int main(int argc, char *argv[]) {
  uint32_t num_unfulfilled_targets = NUM_TARGETS;
  uint32_t choices[30] __attribute__((aligned(64)));
  uint8_t target_lookup[SIZE] __attribute__((aligned(64))) = {0};
  uint8_t unseen[SIZE] __attribute__((aligned(64)));
  uint32_t chain[25] __attribute__((aligned(64)));
  uint32_t not_chain[25] __attribute__((aligned(64)));
  uint32_t expressions[600] __attribute__((aligned(64)));
  uint32_t expressions_size[25] __attribute__((aligned(64)));
  uint32_t tmp_chain_size;
  uint32_t generated_chain_size;
  uint32_t tmp_num_unfulfilled_targets;
  size_t solution_size = 0;
  uint32_t solution[MAX_LENGTH] = {0};

  size_t start_i = 1;
  N = atoi(argv[start_i++]);

  for (size_t i = start_i; i < argc; i++) {
    solution[solution_size++] = strtol(argv[i], NULL, 2);
  }

  chain[0] = 0b0000000011111111 >> (16 - N);
  chain[1] = 0b0000111100001111 >> (16 - N);
  chain[2] = 0b0011001100110011 >> (16 - N);
  chain[3] = 0b0101010101010101 >> (16 - N);
  not_chain[0] = ~chain[0];
  not_chain[1] = ~chain[1];
  not_chain[2] = ~chain[2];
  not_chain[3] = ~chain[3];
  uint32_t chain_size = 4;

  for (uint32_t i = 0; i < SIZE; i++) {
    // flip the logic: 1 means unseen, 0 unseen, that'll avoid one operation
    // when setting this flag
    unseen[i] = 1;
  }

  for (uint32_t i = 0; i < NUM_TARGETS; i++) {
    target_lookup[TARGETS[i]] = 1;
  }

  unseen[0] = 0;
  for (uint32_t i = 0; i < chain_size; i++) {
    unseen[chain[i]] = 0;
  }

  chain_size--;
  expressions_size[chain_size] = 0;
  for (uint32_t k = 1; k < chain_size; k++) {
    const uint32_t h = chain[k];
    const uint32_t not_h = not_chain[k];
    for (uint32_t j = 0; j < k; j++) {
      const uint32_t g = chain[j];
      const uint32_t not_g = not_chain[j];

      ADD_EXPRESSION(g & h, chain_size)
      ADD_EXPRESSION(g & not_h, chain_size)
      ADD_EXPRESSION(g ^ h, chain_size)
      ADD_EXPRESSION(g | h, chain_size)
      ADD_EXPRESSION(not_g & h, chain_size)
    }
  }

  chain_size++;

  for (size_t i = 0; i < solution_size; i++) {
    GENERATE_NEW_EXPRESSIONS(chain_size, ADD_EXPRESSION)

    bool found = false;
    for (size_t j = 0; j < expressions_size[chain_size]; j++) {
      if (expressions[j] == solution[i]) {
        choices[i] = j;
        found = true;
        break;
      }
    }
    if (!found) {
      printf("error: couldn't find the index for function number %zu: %s\n", i,
             std::bitset<16>(solution[i]).to_string().c_str());
      exit(-1);
    }

    chain[chain_size] = expressions[choices[i]];
    not_chain[chain_size] = ~chain[chain_size];

    chain_size++;
  }

  for (size_t i = 0; i < solution_size; i++) {
    printf("%d ", choices[i]);
  }
  printf("\n");

  return 0;
}
