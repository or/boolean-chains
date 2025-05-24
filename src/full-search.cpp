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

constexpr uint32_t N = 12;
constexpr uint32_t SIZE = 1 << (N - 1);
constexpr uint32_t MAX_LENGTH = 18;
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

#if PLAN_MODE
size_t plan_depth = 1;
#endif

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
  for (size_t j = start_chain_length; j < chain_size; ++j) {                   \
    printf("%d, ", choices[j]);                                                \
  }                                                                            \
  printf("%d [best: %zu] %" PRIu64 "\n", last, current_best_length,            \
         total_chains);                                                        \
  fflush(stdout);

#define ADD_EXPRESSION(value, chain_size)                                      \
  expressions[expressions_size[chain_size]] = value;                           \
  expressions_size[chain_size] += seen[value];                                 \
  seen[value] = 0;

#define ADD_EXPRESSION_TARGET(value, chain_size)                               \
  {                                                                            \
    const uint32_t v = value;                                                  \
    const uint32_t a = seen[v] & target_lookup[v];                             \
    expressions[expressions_size[chain_size]] = v;                             \
    expressions_size[chain_size] += a;                                         \
    seen[v] &= ~a;                                                             \
  }

#define GENERATE_NEW_EXPRESSIONS(chain_size, add_expression)                   \
  {                                                                            \
    expressions_size[chain_size] = expressions_size[chain_size - 1];           \
    const uint32_t h = chain[chain_size - 1];                                  \
    const uint32_t not_h = ~h;                                                 \
                                                                               \
    size_t j = 0;                                                              \
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

void print_chain(const uint32_t *chain, const uint32_t *target_lookup,
                 const size_t chain_size) {
  printf("chain (%zu):\n", chain_size);
  for (size_t i = 0; i < chain_size; i++) {
    printf("x%zu", i + 1);
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

        printf(" = x%zu %c x%zu", j + 1, op, k + 1);
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
  bool chunk_mode = false;
  uint32_t num_unfulfilled_targets = NUM_TARGETS;
  size_t stop_chain_size;
  size_t current_best_length = 1000;
  size_t start_indices_size __attribute__((aligned(64))) = 0;
  uint16_t start_indices[100] __attribute__((aligned(64))) = {0};
  uint32_t choices[30] __attribute__((aligned(64)));
  uint32_t target_lookup[SIZE] __attribute__((aligned(64))) = {0};
  uint32_t seen[SIZE] __attribute__((aligned(64)));
  uint32_t chain[25] __attribute__((aligned(64)));
  uint32_t expressions[600] __attribute__((aligned(64)));
  uint32_t expressions_size[25] __attribute__((aligned(64)));

#if !PLAN_MODE
  atexit(on_exit);
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
#endif

  chain[0] = 0b0000000011111111 >> (16 - N);
  chain[1] = 0b0000111100001111 >> (16 - N);
  chain[2] = 0b0011001100110011 >> (16 - N);
  chain[3] = 0b0101010101010101 >> (16 - N);
  size_t chain_size = 4;
  start_chain_length = chain_size;

#if PLAN_MODE
  if (argc > 1) {
    plan_depth = atoi(argv[1]);
  }
#else
  size_t start_i = 1;
  // -c for chunk mode, only complete one slice of the depth given by the
  // progress vector
  if (argc > 1 && strcmp(argv[1], "-c") == 0) {
    start_i++;
    chunk_mode = true;
  }

  for (size_t i = 0; i < chain_size; i++) {
    start_indices[start_indices_size++] = 0;
  }

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

  for (size_t i = 0; i < NUM_TARGETS; i++) {
    target_lookup[TARGETS[i]] = 1;
  }

#if PLAN_MODE != 1
  printf("N = %d, MAX_LENGTH: %d, CAPTURE_STATS: %d\n", N, MAX_LENGTH,
         CAPTURE_STATS);
  printf("%d targets:\n", NUM_TARGETS);
  for (size_t i = 0; i < NUM_TARGETS; i++) {
    printf("  %s\n", std::bitset<N>(TARGETS[i]).to_string().c_str());
  }
  fflush(stdout);
#endif

#if CAPTURE_STATS
  memset(stats_min_num_expressions, UNDEFINED,
         sizeof(stats_min_num_expressions));
#endif
  seen[0] = 0;
  for (size_t i = 0; i < chain_size; i++) {
    seen[chain[i]] = 0;
  }

  chain_size--;
  expressions_size[chain_size] = 0;
  for (size_t k = 1; k < chain_size; k++) {
    const uint32_t h = chain[k];
    const uint32_t not_h = ~h;
    for (size_t j = 0; j < k; j++) {
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

  stop_chain_size = start_chain_length;
  if (chunk_mode) {
    stop_chain_size = start_indices_size;
  }

  // restore progress
  if (start_indices_size > start_chain_length) {
    while (chain_size < start_indices_size - 1) {
      GENERATE_NEW_EXPRESSIONS(chain_size, ADD_EXPRESSION)
      choices[chain_size] = start_indices[chain_size];
      chain[chain_size] = expressions[choices[chain_size]];
      num_unfulfilled_targets -= target_lookup[chain[chain_size]];
      chain_size++;
    }

    GENERATE_NEW_EXPRESSIONS(chain_size, ADD_EXPRESSION)

    // will be incremented again in the main loop
    choices[chain_size] = start_indices[chain_size] - 1;

    // this will be counted again
    total_chains--;

    goto restore_progress_4;
  } else {
    // so that the first addition in the loop results in 0 for the first choice
    choices[chain_size] = 0xffffffff;
  }

// Level 4
level_4:
  GENERATE_NEW_EXPRESSIONS(4, ADD_EXPRESSION)
  CAPTURE_STATS_CALL(4)
restore_progress_4:
  while (true) {
    choices[4]++;
    while (choices[4] < expressions_size[4]) {
      chain[4] = expressions[choices[4]];
#if PLAN_MODE
      if (4 + 1 - start_chain_length >= plan_depth) {
        printf("-c");
        for (size_t j = start_chain_length; j <= 4; ++j) {
          printf(" %d", choices[j]);
        }
        printf("\n");
        choices[4]++;
        continue;
      }
#endif
      total_chains++;
      num_unfulfilled_targets -= target_lookup[chain[4]];
      if (4 + num_unfulfilled_targets >= MAX_LENGTH) {
        choices[4]++;
        continue;
      }
      if (__builtin_expect(!num_unfulfilled_targets, 0)) {
        print_chain(chain, target_lookup, 4 + 1);
        if (4 + 1 < current_best_length) {
          current_best_length = 4 + 1;
        }
        num_unfulfilled_targets++;
        goto done_4;
      }
      choices[5] = choices[4];
      goto level_5;
    }
  done_4:
    for (size_t i = expressions_size[3]; i < expressions_size[4]; i++) {
      seen[expressions[i]] = 1;
    }
    num_unfulfilled_targets += target_lookup[chain[3]];
    choices[3] += target_lookup[chain[3]] << 16;
    if (__builtin_expect(3 < stop_chain_size, 0)) {
      return 0;
    }
    break; // Exit main loop
  }

// Level 5
level_5:
  GENERATE_NEW_EXPRESSIONS(5, ADD_EXPRESSION)
  CAPTURE_STATS_CALL(5)
restore_progress_5:
  while (true) {
    choices[5]++;
    while (choices[5] < expressions_size[5]) {
      chain[5] = expressions[choices[5]];
#if PLAN_MODE
      if (5 + 1 - start_chain_length >= plan_depth) {
        printf("-c");
        for (size_t j = start_chain_length; j <= 5; ++j) {
          printf(" %d", choices[j]);
        }
        printf("\n");
        choices[5]++;
        continue;
      }
#endif
      total_chains++;
      PRINT_PROGRESS(5, choices[5])
      num_unfulfilled_targets -= target_lookup[chain[5]];
      if (5 + num_unfulfilled_targets >= MAX_LENGTH) {
        choices[5]++;
        continue;
      }
      if (__builtin_expect(!num_unfulfilled_targets, 0)) {
        print_chain(chain, target_lookup, 5 + 1);
        if (5 + 1 < current_best_length) {
          current_best_length = 5 + 1;
        }
        num_unfulfilled_targets++;
        goto done_5;
      }
      choices[6] = choices[5];
      goto level_6;
    }
  done_5:
    for (size_t i = expressions_size[4]; i < expressions_size[5]; i++) {
      seen[expressions[i]] = 1;
    }
    num_unfulfilled_targets += target_lookup[chain[4]];
    choices[4] += target_lookup[chain[4]] << 16;
    if (__builtin_expect(4 < stop_chain_size, 0)) {
      return 0;
    }
    goto restore_progress_4;
  }

// Level 6
level_6:
  GENERATE_NEW_EXPRESSIONS(6, ADD_EXPRESSION)
  CAPTURE_STATS_CALL(6)
restore_progress_6:
  while (true) {
    choices[6]++;
    while (choices[6] < expressions_size[6]) {
      chain[6] = expressions[choices[6]];
#if PLAN_MODE
      if (6 + 1 - start_chain_length >= plan_depth) {
        printf("-c");
        for (size_t j = start_chain_length; j <= 6; ++j) {
          printf(" %d", choices[j]);
        }
        printf("\n");
        choices[6]++;
        continue;
      }
#endif
      total_chains++;
      num_unfulfilled_targets -= target_lookup[chain[6]];
      if (6 + num_unfulfilled_targets >= MAX_LENGTH) {
        choices[6]++;
        continue;
      }
      if (__builtin_expect(!num_unfulfilled_targets, 0)) {
        print_chain(chain, target_lookup, 6 + 1);
        if (6 + 1 < current_best_length) {
          current_best_length = 6 + 1;
        }
        num_unfulfilled_targets++;
        goto done_6;
      }
      choices[7] = choices[6];
      goto level_7;
    }
  done_6:
    for (size_t i = expressions_size[5]; i < expressions_size[6]; i++) {
      seen[expressions[i]] = 1;
    }
    num_unfulfilled_targets += target_lookup[chain[5]];
    choices[5] += target_lookup[chain[5]] << 16;
    if (__builtin_expect(5 < stop_chain_size, 0)) {
      return 0;
    }
    goto restore_progress_5;
  }

// Level 7
level_7:
  GENERATE_NEW_EXPRESSIONS(7, ADD_EXPRESSION)
  CAPTURE_STATS_CALL(7)
restore_progress_7:
  while (true) {
    choices[7]++;
    while (choices[7] < expressions_size[7]) {
      chain[7] = expressions[choices[7]];
#if PLAN_MODE
      if (7 + 1 - start_chain_length >= plan_depth) {
        printf("-c");
        for (size_t j = start_chain_length; j <= 7; ++j) {
          printf(" %d", choices[j]);
        }
        printf("\n");
        choices[7]++;
        continue;
      }
#endif
      total_chains++;
      num_unfulfilled_targets -= target_lookup[chain[7]];
      if (7 + num_unfulfilled_targets >= MAX_LENGTH) {
        choices[7]++;
        continue;
      }
      if (__builtin_expect(!num_unfulfilled_targets, 0)) {
        print_chain(chain, target_lookup, 7 + 1);
        if (7 + 1 < current_best_length) {
          current_best_length = 7 + 1;
        }
        num_unfulfilled_targets++;
        goto done_7;
      }
      choices[8] = choices[7];
      goto level_8;
    }
  done_7:
    for (size_t i = expressions_size[6]; i < expressions_size[7]; i++) {
      seen[expressions[i]] = 1;
    }
    num_unfulfilled_targets += target_lookup[chain[6]];
    choices[6] += target_lookup[chain[6]] << 16;
    if (__builtin_expect(6 < stop_chain_size, 0)) {
      return 0;
    }
    goto restore_progress_6;
  }

// Level 8
level_8:
  GENERATE_NEW_EXPRESSIONS(8, ADD_EXPRESSION)
  CAPTURE_STATS_CALL(8)
restore_progress_8:
  while (true) {
    choices[8]++;
    while (choices[8] < expressions_size[8]) {
      chain[8] = expressions[choices[8]];
#if PLAN_MODE
      if (8 + 1 - start_chain_length >= plan_depth) {
        printf("-c");
        for (size_t j = start_chain_length; j <= 8; ++j) {
          printf(" %d", choices[j]);
        }
        printf("\n");
        choices[8]++;
        continue;
      }
#endif
      total_chains++;
      num_unfulfilled_targets -= target_lookup[chain[8]];
      if (8 + num_unfulfilled_targets >= MAX_LENGTH) {
        choices[8]++;
        continue;
      }
      if (__builtin_expect(!num_unfulfilled_targets, 0)) {
        print_chain(chain, target_lookup, 8 + 1);
        if (8 + 1 < current_best_length) {
          current_best_length = 8 + 1;
        }
        num_unfulfilled_targets++;
        goto done_8;
      }
      choices[9] = choices[8];
      goto level_9;
    }
  done_8:
    for (size_t i = expressions_size[7]; i < expressions_size[8]; i++) {
      seen[expressions[i]] = 1;
    }
    num_unfulfilled_targets += target_lookup[chain[7]];
    choices[7] += target_lookup[chain[7]] << 16;
    if (__builtin_expect(7 < stop_chain_size, 0)) {
      return 0;
    }
    goto restore_progress_7;
  }

// Level 9
level_9:
  GENERATE_NEW_EXPRESSIONS(9, ADD_EXPRESSION)
  CAPTURE_STATS_CALL(9)
restore_progress_9:
  while (true) {
    choices[9]++;
    while (choices[9] < expressions_size[9]) {
      chain[9] = expressions[choices[9]];
#if PLAN_MODE
      if (9 + 1 - start_chain_length >= plan_depth) {
        printf("-c");
        for (size_t j = start_chain_length; j <= 9; ++j) {
          printf(" %d", choices[j]);
        }
        printf("\n");
        choices[9]++;
        continue;
      }
#endif
      total_chains++;
      num_unfulfilled_targets -= target_lookup[chain[9]];
      if (9 + num_unfulfilled_targets >= MAX_LENGTH) {
        choices[9]++;
        continue;
      }
      if (__builtin_expect(!num_unfulfilled_targets, 0)) {
        print_chain(chain, target_lookup, 9 + 1);
        if (9 + 1 < current_best_length) {
          current_best_length = 9 + 1;
        }
        num_unfulfilled_targets++;
        goto done_9;
      }
      choices[10] = choices[9];
      goto level_10;
    }
  done_9:
    for (size_t i = expressions_size[8]; i < expressions_size[9]; i++) {
      seen[expressions[i]] = 1;
    }
    num_unfulfilled_targets += target_lookup[chain[8]];
    choices[8] += target_lookup[chain[8]] << 16;
    if (__builtin_expect(8 < stop_chain_size, 0)) {
      return 0;
    }
    goto restore_progress_8;
  }

// Level 10
level_10:
  GENERATE_NEW_EXPRESSIONS(10, ADD_EXPRESSION)
  CAPTURE_STATS_CALL(10)
restore_progress_10:
  while (true) {
    choices[10]++;
    while (choices[10] < expressions_size[10]) {
      chain[10] = expressions[choices[10]];
#if PLAN_MODE
      if (10 + 1 - start_chain_length >= plan_depth) {
        printf("-c");
        for (size_t j = start_chain_length; j <= 10; ++j) {
          printf(" %d", choices[j]);
        }
        printf("\n");
        choices[10]++;
        continue;
      }
#endif
      total_chains++;
      num_unfulfilled_targets -= target_lookup[chain[10]];
      if (10 + num_unfulfilled_targets >= MAX_LENGTH) {
        choices[10]++;
        continue;
      }
      if (__builtin_expect(!num_unfulfilled_targets, 0)) {
        print_chain(chain, target_lookup, 10 + 1);
        if (10 + 1 < current_best_length) {
          current_best_length = 10 + 1;
        }
        num_unfulfilled_targets++;
        goto done_10;
      }
      choices[11] = choices[10];
      goto level_11;
    }
  done_10:
    for (size_t i = expressions_size[9]; i < expressions_size[10]; i++) {
      seen[expressions[i]] = 1;
    }
    num_unfulfilled_targets += target_lookup[chain[9]];
    choices[9] += target_lookup[chain[9]] << 16;
    if (__builtin_expect(9 < stop_chain_size, 0)) {
      return 0;
    }
    goto restore_progress_9;
  }

// Level 11
level_11:
  GENERATE_NEW_EXPRESSIONS(11, ADD_EXPRESSION)
  CAPTURE_STATS_CALL(11)
restore_progress_11:
  while (true) {
    choices[11]++;
    while (choices[11] < expressions_size[11]) {
      chain[11] = expressions[choices[11]];
#if PLAN_MODE
      if (11 + 1 - start_chain_length >= plan_depth) {
        printf("-c");
        for (size_t j = start_chain_length; j <= 11; ++j) {
          printf(" %d", choices[j]);
        }
        printf("\n");
        choices[11]++;
        continue;
      }
#endif
      total_chains++;
      num_unfulfilled_targets -= target_lookup[chain[11]];
      if (11 + num_unfulfilled_targets >= MAX_LENGTH) {
        choices[11]++;
        continue;
      }
      if (__builtin_expect(!num_unfulfilled_targets, 0)) {
        print_chain(chain, target_lookup, 11 + 1);
        if (11 + 1 < current_best_length) {
          current_best_length = 11 + 1;
        }
        num_unfulfilled_targets++;
        goto done_11;
      }
      choices[12] = choices[11];
      goto level_12;
    }
  done_11:
    for (size_t i = expressions_size[10]; i < expressions_size[11]; i++) {
      seen[expressions[i]] = 1;
    }
    num_unfulfilled_targets += target_lookup[chain[10]];
    choices[10] += target_lookup[chain[10]] << 16;
    if (__builtin_expect(10 < stop_chain_size, 0)) {
      return 0;
    }
    goto restore_progress_10;
  }

// Level 12
level_12:
  GENERATE_NEW_EXPRESSIONS(12, ADD_EXPRESSION)
  CAPTURE_STATS_CALL(12)
restore_progress_12:
  while (true) {
    choices[12]++;
    while (choices[12] < expressions_size[12]) {
      chain[12] = expressions[choices[12]];
#if PLAN_MODE
      if (12 + 1 - start_chain_length >= plan_depth) {
        printf("-c");
        for (size_t j = start_chain_length; j <= 12; ++j) {
          printf(" %d", choices[j]);
        }
        printf("\n");
        choices[12]++;
        continue;
      }
#endif
      total_chains++;
      num_unfulfilled_targets -= target_lookup[chain[12]];
      if (12 + num_unfulfilled_targets >= MAX_LENGTH) {
        choices[12]++;
        continue;
      }
      if (__builtin_expect(!num_unfulfilled_targets, 0)) {
        print_chain(chain, target_lookup, 12 + 1);
        if (12 + 1 < current_best_length) {
          current_best_length = 12 + 1;
        }
        num_unfulfilled_targets++;
        goto done_12;
      }
      choices[13] = choices[12];
      goto level_13;
    }
  done_12:
    for (size_t i = expressions_size[11]; i < expressions_size[12]; i++) {
      seen[expressions[i]] = 1;
    }
    num_unfulfilled_targets += target_lookup[chain[11]];
    choices[11] += target_lookup[chain[11]] << 16;
    if (__builtin_expect(11 < stop_chain_size, 0)) {
      return 0;
    }
    goto restore_progress_11;
  }

// Level 13
level_13:
  GENERATE_NEW_EXPRESSIONS(13, ADD_EXPRESSION)
  CAPTURE_STATS_CALL(13)
restore_progress_13:
  while (true) {
    choices[13]++;
    while (choices[13] < expressions_size[13]) {
      chain[13] = expressions[choices[13]];
#if PLAN_MODE
      if (13 + 1 - start_chain_length >= plan_depth) {
        printf("-c");
        for (size_t j = start_chain_length; j <= 13; ++j) {
          printf(" %d", choices[j]);
        }
        printf("\n");
        choices[13]++;
        continue;
      }
#endif
      total_chains++;
      num_unfulfilled_targets -= target_lookup[chain[13]];
      if (13 + num_unfulfilled_targets >= MAX_LENGTH) {
        choices[13]++;
        continue;
      }
      if (__builtin_expect(!num_unfulfilled_targets, 0)) {
        print_chain(chain, target_lookup, 13 + 1);
        if (13 + 1 < current_best_length) {
          current_best_length = 13 + 1;
        }
        num_unfulfilled_targets++;
        goto done_13;
      }
      choices[14] = choices[13];
      goto level_14;
    }
  done_13:
    for (size_t i = expressions_size[12]; i < expressions_size[13]; i++) {
      seen[expressions[i]] = 1;
    }
    num_unfulfilled_targets += target_lookup[chain[12]];
    choices[12] += target_lookup[chain[12]] << 16;
    if (__builtin_expect(12 < stop_chain_size, 0)) {
      return 0;
    }
    goto restore_progress_12;
  }

// Level 14
level_14:
  GENERATE_NEW_EXPRESSIONS(14, ADD_EXPRESSION)
  CAPTURE_STATS_CALL(14)
restore_progress_14:
  while (true) {
    choices[14]++;
    while (choices[14] < expressions_size[14]) {
      chain[14] = expressions[choices[14]];
#if PLAN_MODE
      if (14 + 1 - start_chain_length >= plan_depth) {
        printf("-c");
        for (size_t j = start_chain_length; j <= 14; ++j) {
          printf(" %d", choices[j]);
        }
        printf("\n");
        choices[14]++;
        continue;
      }
#endif
      total_chains++;
      num_unfulfilled_targets -= target_lookup[chain[14]];
      if (14 + num_unfulfilled_targets >= MAX_LENGTH) {
        choices[14]++;
        continue;
      }
      if (__builtin_expect(!num_unfulfilled_targets, 0)) {
        print_chain(chain, target_lookup, 14 + 1);
        if (14 + 1 < current_best_length) {
          current_best_length = 14 + 1;
        }
        num_unfulfilled_targets++;
        goto done_14;
      }
      choices[15] = choices[14];
      goto level_15;
    }
  done_14:
    for (size_t i = expressions_size[13]; i < expressions_size[14]; i++) {
      seen[expressions[i]] = 1;
    }
    num_unfulfilled_targets += target_lookup[chain[13]];
    choices[13] += target_lookup[chain[13]] << 16;
    if (__builtin_expect(13 < stop_chain_size, 0)) {
      return 0;
    }
    goto restore_progress_13;
  }

// Level 15
level_15:
  GENERATE_NEW_EXPRESSIONS(15, ADD_EXPRESSION)
  CAPTURE_STATS_CALL(15)
restore_progress_15:
  while (true) {
    choices[15]++;
    while (choices[15] < expressions_size[15]) {
      chain[15] = expressions[choices[15]];
#if PLAN_MODE
      if (15 + 1 - start_chain_length >= plan_depth) {
        printf("-c");
        for (size_t j = start_chain_length; j <= 15; ++j) {
          printf(" %d", choices[j]);
        }
        printf("\n");
        choices[15]++;
        continue;
      }
#endif
      total_chains++;
      num_unfulfilled_targets -= target_lookup[chain[15]];
      if (15 + num_unfulfilled_targets >= MAX_LENGTH) {
        choices[15]++;
        continue;
      }
      if (__builtin_expect(!num_unfulfilled_targets, 0)) {
        print_chain(chain, target_lookup, 15 + 1);
        if (15 + 1 < current_best_length) {
          current_best_length = 15 + 1;
        }
        num_unfulfilled_targets++;
        goto done_15;
      }
      choices[16] = choices[15];
      goto level_16;
    }
  done_15:
    for (size_t i = expressions_size[14]; i < expressions_size[15]; i++) {
      seen[expressions[i]] = 1;
    }
    num_unfulfilled_targets += target_lookup[chain[14]];
    choices[14] += target_lookup[chain[14]] << 16;
    if (__builtin_expect(14 < stop_chain_size, 0)) {
      return 0;
    }
    goto restore_progress_14;
  }

// Level 16
level_16:
  GENERATE_NEW_EXPRESSIONS(16, ADD_EXPRESSION)
  CAPTURE_STATS_CALL(16)
restore_progress_16:
  while (true) {
    choices[16]++;
    while (choices[16] < expressions_size[16]) {
      chain[16] = expressions[choices[16]];
#if PLAN_MODE
      if (16 + 1 - start_chain_length >= plan_depth) {
        printf("-c");
        for (size_t j = start_chain_length; j <= 16; ++j) {
          printf(" %d", choices[j]);
        }
        printf("\n");
        choices[16]++;
        continue;
      }
#endif
      total_chains++;
      num_unfulfilled_targets -= target_lookup[chain[16]];
      if (16 + num_unfulfilled_targets >= MAX_LENGTH) {
        choices[16]++;
        continue;
      }
      if (__builtin_expect(!num_unfulfilled_targets, 0)) {
        print_chain(chain, target_lookup, 16 + 1);
        if (16 + 1 < current_best_length) {
          current_best_length = 16 + 1;
        }
        num_unfulfilled_targets++;
        goto done_16;
      }
      choices[17] = choices[16];
      goto level_17;
    }
  done_16:
    for (size_t i = expressions_size[15]; i < expressions_size[16]; i++) {
      seen[expressions[i]] = 1;
    }
    num_unfulfilled_targets += target_lookup[chain[15]];
    choices[15] += target_lookup[chain[15]] << 16;
    if (__builtin_expect(15 < stop_chain_size, 0)) {
      return 0;
    }
    goto restore_progress_15;
  }

// Level 17 (final level since MAX_LENGTH = 18)
level_17:
  GENERATE_NEW_EXPRESSIONS(17, ADD_EXPRESSION)
  CAPTURE_STATS_CALL(17)
restore_progress_17:
  while (true) {
    choices[17]++;
    while (choices[17] < expressions_size[17]) {
      chain[17] = expressions[choices[17]];
#if PLAN_MODE
      if (17 + 1 - start_chain_length >= plan_depth) {
        printf("-c");
        for (size_t j = start_chain_length; j <= 17; ++j) {
          printf(" %d", choices[j]);
        }
        printf("\n");
        choices[17]++;
        continue;
      }
#endif
      total_chains++;
      num_unfulfilled_targets -= target_lookup[chain[17]];
      if (17 + num_unfulfilled_targets >= MAX_LENGTH) {
        choices[17]++;
        continue;
      }
      if (__builtin_expect(!num_unfulfilled_targets, 0)) {
        print_chain(chain, target_lookup, 17 + 1);
        if (17 + 1 < current_best_length) {
          current_best_length = 17 + 1;
        }
        num_unfulfilled_targets++;
        goto done_17;
      }
      // No next level - we've reached MAX_LENGTH
      goto done_17;
    }
  done_17:
    for (size_t i = expressions_size[16]; i < expressions_size[17]; i++) {
      seen[expressions[i]] = 1;
    }
    num_unfulfilled_targets += target_lookup[chain[16]];
    choices[16] += target_lookup[chain[16]] << 16;
    if (__builtin_expect(16 < stop_chain_size, 0)) {
      return 0;
    }
    goto restore_progress_16;
  }

  return 0;
}
