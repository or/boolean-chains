#include <bitset>
#include <cinttypes>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#ifndef CAPTURE_STATS
#define CAPTURE_STATS 1
#endif

#ifndef PLAN_MODE
#define PLAN_MODE 0
#endif

#define CHUNK_START_LENGTH 9

#if CAPTURE_STATS
#define CAPTURE_STATS_CALL(chain_size)                                         \
  {                                                                            \
    const auto new_expressions =                                               \
        expressions_size[chain_size] - expressions_size[chain_size - 1];       \
    stats_min_num_expressions[chain_size] +=                                   \
        (new_expressions < stats_min_num_expressions[chain_size]) *            \
        (new_expressions - stats_min_num_expressions[chain_size]);             \
    stats_max_num_expressions[chain_size] +=                                   \
        (new_expressions > stats_max_num_expressions[chain_size]) *            \
        (new_expressions - stats_max_num_expressions[chain_size]);             \
    stats_total_num_expressions[chain_size] += new_expressions;                \
    stats_num_data_points[chain_size]++;                                       \
  }
#else
#define CAPTURE_STATS_CALL(chain_size)
#endif

constexpr uint32_t N = 16;
constexpr uint32_t SIZE = 1 << (N - 1);
constexpr uint32_t MAX_LENGTH = 22;
constexpr uint32_t PRINT_PROGRESS_LENGTH = 10;
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

uint32_t plan_depth = 1;

uint32_t start_chain_length;
uint64_t total_chains = 0;
#if CAPTURE_STATS
#define UNDEFINED 0xffffffff
uint64_t stats_total_num_expressions[25] = {0};
uint32_t stats_min_num_expressions[25] = {UNDEFINED};
uint32_t stats_max_num_expressions[25] = {0};
uint64_t stats_num_data_points[25] = {0};
#endif

#define PRINT_PROGRESS(chain_size, last)                                       \
  for (uint32_t j = start_chain_length; j < chain_size; ++j) {                 \
    printf("%d, ", choices[j]);                                                \
  }                                                                            \
  printf("%d %" PRIu64 "\n", last, total_chains);                              \
  fflush(stdout);

#define ADD_EXPRESSION(value, chain_size)                                      \
  expressions[expressions_size[chain_size]] = value;                           \
  expressions_size[chain_size] += unseen[value];                               \
  unseen[value] = 0;

#define ADD_EXPRESSION_TARGET(value, chain_size)                               \
  {                                                                            \
    const uint32_t v = value;                                                  \
    const uint32_t a = unseen[v] & target_lookup[v];                           \
    expressions[expressions_size[chain_size]] = v;                             \
    expressions_size[chain_size] += a;                                         \
    unseen[v] &= ~a;                                                           \
  }

#define GENERATE_NEW_EXPRESSIONS(chain_size, add_expression)                   \
  {                                                                            \
    expressions_size[chain_size] = expressions_size[chain_size - 1];           \
    const uint32_t h = chain[chain_size - 1];                                  \
    const uint32_t not_h = ~h;                                                 \
                                                                               \
    uint32_t j = 0;                                                            \
    for (; j < chain_size - 4; j += 4) {                                       \
      const uint32_t g0 = chain[j], g1 = chain[j + 1], g2 = chain[j + 2],      \
                     g3 = chain[j + 3];                                        \
      const uint32_t not_g0 = ~g0, not_g1 = ~g1, not_g2 = ~g2, not_g3 = ~g3;   \
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
    for (; j < chain_size - 1; j++) {                                          \
      const uint32_t g = chain[j];                                             \
      const uint32_t not_g = ~chain[j];                                        \
      add_expression(g & h, chain_size);                                       \
      add_expression(not_g & h, chain_size);                                   \
      add_expression(g & not_h, chain_size);                                   \
      add_expression(g ^ h, chain_size);                                       \
      add_expression(g | h, chain_size);                                       \
    }                                                                          \
  }

#define FORWARD_DFS(CS, PREV_CS, NEXT_CS)                                      \
  GENERATE_NEW_EXPRESSIONS(CS, ADD_EXPRESSION)                                 \
  CAPTURE_STATS_CALL(CS)                                                       \
                                                                               \
  for (uint32_t i##CS = i##PREV_CS + 1; i##CS < expressions_size[CS];          \
       ++i##CS) {                                                              \
    chain[CS] = expressions[i##CS];                                            \
    choices[CS] = i##CS;                                                       \
    const uint8_t is_target = target_lookup[chain[CS]];                        \
                                                                               \
    if (PLAN_MODE) {                                                           \
      if (CS >= CHUNK_START_LENGTH - 1) {                                      \
        for (uint32_t j = start_chain_length; j <= CS; ++j) {                  \
          printf("%d ", choices[j]);                                           \
        }                                                                      \
        printf("\n");                                                          \
        continue;                                                              \
      }                                                                        \
    }                                                                          \
                                                                               \
    total_chains++;                                                            \
                                                                               \
    if (!PLAN_MODE && CS + 1 == PRINT_PROGRESS_LENGTH) {                       \
      PRINT_PROGRESS(CS, i##CS);                                               \
    }                                                                          \
                                                                               \
    num_unfulfilled_targets -= is_target;                                      \
    if (CS < MAX_LENGTH - 1 && NEXT_CS >= MAX_LENGTH - NUM_TARGETS) {          \
      if (__builtin_expect(NEXT_CS + num_unfulfilled_targets == MAX_LENGTH,    \
                           1)) {                                               \
        tmp_chain_size = NEXT_CS;                                              \
        generated_chain_size = CS;                                             \
        tmp_num_unfulfilled_targets = num_unfulfilled_targets;                 \
        uint32_t j = i##CS + 1;                                                \
                                                                               \
        next_##CS : if (__builtin_expect(tmp_chain_size < MAX_LENGTH, 1)) {    \
          GENERATE_NEW_EXPRESSIONS(tmp_chain_size, ADD_EXPRESSION_TARGET)      \
          generated_chain_size = tmp_chain_size;                               \
                                                                               \
          for (; j < expressions_size[tmp_chain_size]; ++j) {                  \
            total_chains++;                                                    \
            if (__builtin_expect(target_lookup[expressions[j]], 0)) {          \
              chain[tmp_chain_size] = expressions[j];                          \
              tmp_num_unfulfilled_targets--;                                   \
              tmp_chain_size++;                                                \
              if (__builtin_expect(!tmp_num_unfulfilled_targets, 0)) {         \
                print_chain(chain, target_lookup, tmp_chain_size);             \
                break;                                                         \
              }                                                                \
              j++;                                                             \
              goto next_##CS;                                                  \
            }                                                                  \
          }                                                                    \
        }                                                                      \
                                                                               \
        for (uint32_t i = expressions_size[CS];                                \
             i < expressions_size[generated_chain_size]; i++) {                \
          unseen[expressions[i]] = 1;                                          \
        }                                                                      \
                                                                               \
        num_unfulfilled_targets += is_target;                                  \
        i##CS += (is_target << 16);                                            \
        continue;                                                              \
      }                                                                        \
    }

#define BACKTRACK_DFS(CS, PREV_CS, NEXT_CS)                                    \
  num_unfulfilled_targets += is_target;                                        \
  i##CS += (is_target << 16);                                                  \
  }                                                                            \
                                                                               \
  for (uint32_t i = expressions_size[PREV_CS]; i < expressions_size[CS];       \
       i++) {                                                                  \
    unseen[expressions[i]] = 1;                                                \
  }

void print_chain(const uint32_t *chain, const uint8_t *target_lookup,
                 const uint32_t chain_size) {
  printf("chain (%d):\n", chain_size);
  for (uint32_t i = 0; i < chain_size; i++) {
    printf("x%d", i + 1);
    for (uint32_t j = 0; j < i; j++) {
      for (uint32_t k = j + 1; k < i; k++) {
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

        printf(" = x%d %c x%d", j + 1, op, k + 1);
      }
    }
    printf(" = %s", std::bitset<N>(chain[i]).to_string().c_str());
    if (target_lookup[chain[i]]) {
      printf(" [target]");
    }
    printf("\n");
  }
}

void on_exit() {
  printf("total chains: %" PRIu64 "\n", total_chains);

#if CAPTURE_STATS
  printf("new expressions at chain length:\n");
  printf("                   n                       sum              avg     "
         "         "
         "min              max\n");

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
#endif
}

void signal_handler(int signal) { exit(signal); }

int main(int argc, char *argv[]) {
  uint32_t num_unfulfilled_targets = NUM_TARGETS;
  uint32_t start_indices_size __attribute__((aligned(64))) = 0;
  uint16_t start_indices[100] __attribute__((aligned(64))) = {0};
  uint32_t choices[30] __attribute__((aligned(64)));
  uint8_t target_lookup[SIZE] __attribute__((aligned(64))) = {0};
  uint8_t unseen[SIZE] __attribute__((aligned(64)));
  uint32_t chain[25] __attribute__((aligned(64)));
  uint32_t expressions[600] __attribute__((aligned(64)));
  uint32_t expressions_size[25] __attribute__((aligned(64)));
  uint32_t tmp_chain_size;
  uint32_t generated_chain_size;
  uint32_t tmp_num_unfulfilled_targets;

#if !PLAN_MODE
  atexit(on_exit);
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
#endif

  chain[0] = 0b0000000011111111 >> (16 - N);
  chain[1] = 0b0000111100001111 >> (16 - N);
  chain[2] = 0b0011001100110011 >> (16 - N);
  chain[3] = 0b0101010101010101 >> (16 - N);
  uint32_t chain_size = 4;
  start_chain_length = chain_size;

#if !PLAN_MODE
  for (uint32_t i = 0; i < chain_size; i++) {
    start_indices[start_indices_size++] = 0;
  }

  // read the progress vector, e.g 5 2 9, commas will be ignored: 5, 2, 9
  for (uint32_t i = 1; i < argc; i++) {
    start_indices[start_indices_size++] = atoi(argv[i]);
  }

  if (start_indices_size != CHUNK_START_LENGTH) {
    printf("expected %d integers as chunk prefix\n",
           CHUNK_START_LENGTH - chain_size);
    return -1;
  }
#endif

  for (uint32_t i = 0; i < SIZE; i++) {
    // flip the logic: 1 means unseen, 0 unseen, that'll avoid one operation
    // when setting this flag
    unseen[i] = 1;
  }

  for (uint32_t i = 0; i < NUM_TARGETS; i++) {
    target_lookup[TARGETS[i]] = 1;
  }

#if PLAN_MODE != 1
  printf("N = %d, MAX_LENGTH: %d, CAPTURE_STATS: %d\n", N, MAX_LENGTH,
         CAPTURE_STATS);
  printf("%d targets:\n", NUM_TARGETS);
  for (uint32_t i = 0; i < NUM_TARGETS; i++) {
    printf("  %s\n", std::bitset<N>(TARGETS[i]).to_string().c_str());
  }
  fflush(stdout);
#endif

#if CAPTURE_STATS
  memset(stats_min_num_expressions, UNDEFINED,
         sizeof(stats_min_num_expressions));
#endif
  unseen[0] = 0;
  for (uint32_t i = 0; i < chain_size; i++) {
    unseen[chain[i]] = 0;
  }

  chain_size--;
  expressions_size[chain_size] = 0;
  for (uint32_t k = 1; k < chain_size; k++) {
    const uint32_t h = chain[k];
    const uint32_t not_h = ~h;
    for (uint32_t j = 0; j < k; j++) {
      const uint32_t g = chain[j];
      const uint32_t not_g = ~g;

      ADD_EXPRESSION(g & h, chain_size)
      ADD_EXPRESSION(g & not_h, chain_size)
      ADD_EXPRESSION(g ^ h, chain_size)
      ADD_EXPRESSION(g | h, chain_size)
      ADD_EXPRESSION(not_g & h, chain_size)
    }
  }

  // just to get the initial branch before the algorithm even starts
  CAPTURE_STATS_CALL(chain_size)
  chain_size++;

#if PLAN_MODE
  uint32_t i3 = 0xffffffff;
#else
  // restore progress
  while (chain_size < start_indices_size) {
    GENERATE_NEW_EXPRESSIONS(chain_size, ADD_EXPRESSION)
    // this can be used to output the first expressions with the index,
    // which allows some custom order for promising expressions
    // if (chain_size == 4) {
    //   for (int m = 0; m < expressions_size[4]; m++) {
    //     uint32_t e = expressions[m];
    //     printf("%d: ", m);
    //     for (uint32_t j = 0; j < 4; j++) {
    //       for (uint32_t k = j + 1; k < 4; k++) {
    //         char op = 0;
    //         if (e == (chain[j] & chain[k])) {
    //           op = '&';
    //         } else if (e == (chain[j] | chain[k])) {
    //           op = '|';
    //         } else if (e == (chain[j] ^ chain[k])) {
    //           op = '^';
    //         } else if (e == ((~chain[j]) & chain[k])) {
    //           op = '<';
    //         } else if (e == (chain[j] & (~chain[k]))) {
    //           op = '>';
    //         } else {
    //           continue;
    //         }
    //         printf(" = x%d %c x%d", j + 1, op, k + 1);
    //       }
    //     }
    //     printf("\n");
    //   }
    // }
    choices[chain_size] = start_indices[chain_size];
    chain[chain_size] = expressions[choices[chain_size]];
    num_unfulfilled_targets -= target_lookup[chain[chain_size]];
    chain_size++;
  }

  uint32_t i8 = choices[chain_size - 1];

#endif

#if PLAN_MODE
  FORWARD_DFS(4, 3, 5)
  FORWARD_DFS(5, 4, 6)
  FORWARD_DFS(6, 5, 7)
  FORWARD_DFS(7, 6, 8)
  FORWARD_DFS(8, 7, 9)
#endif
  FORWARD_DFS(9, 8, 10)
  FORWARD_DFS(10, 9, 11)
  FORWARD_DFS(11, 10, 12)
  FORWARD_DFS(12, 11, 13)
  FORWARD_DFS(13, 12, 14)
  FORWARD_DFS(14, 13, 15)
  FORWARD_DFS(15, 14, 16)
  FORWARD_DFS(16, 15, 17)
  FORWARD_DFS(17, 16, 18)
  FORWARD_DFS(18, 17, 19)
  FORWARD_DFS(19, 18, 20)
  FORWARD_DFS(20, 19, 21)
  FORWARD_DFS(21, 20, 22)
  FORWARD_DFS(22, 21, 23)

  BACKTRACK_DFS(22, 21, 23)
  BACKTRACK_DFS(21, 20, 22)
  BACKTRACK_DFS(20, 19, 21)
  BACKTRACK_DFS(19, 18, 20)
  BACKTRACK_DFS(18, 17, 19)
  BACKTRACK_DFS(17, 16, 18)
  BACKTRACK_DFS(16, 15, 17)
  BACKTRACK_DFS(15, 14, 16)
  BACKTRACK_DFS(14, 13, 15)
  BACKTRACK_DFS(13, 12, 14)
  BACKTRACK_DFS(12, 11, 13)
  BACKTRACK_DFS(11, 10, 12)
  BACKTRACK_DFS(10, 9, 11)
  BACKTRACK_DFS(9, 8, 10)
#if PLAN_MODE
  BACKTRACK_DFS(8, 7, 9)
  BACKTRACK_DFS(7, 6, 8)
  BACKTRACK_DFS(6, 5, 7)
  BACKTRACK_DFS(5, 4, 6)
  BACKTRACK_DFS(4, 3, 5)
#endif

  return 0;
}
