#include "bit_set_template.h"
#include "chain.h"
#include "expression.h"
#include "full_search.h"
#include "function.h"
#include <cstdint>
#include <ctime>
#include <iostream>
using namespace std;

const uint32_t N = 16;
const uint32_t S = ((1 << (N - 1)) + 31) / 32;
const uint32_t MAX_LENGTH = 24;

int main(int argc, char *argv[]) {
  Chain<N> chain({
      Function<N>(~0b1011011111100011),
      Function<N>(~0b1111100111100100),
      Function<N>(~0b1101111111110100),
      Function<N>(~0b1011011011011110),
      Function<N>(~0b1010001010111111),
      Function<N>(~0b1000111111110011),
      Function<N>(0b0011111011111111),
  });

  // parse a vector of integers passed as arguments
  vector<uint32_t> start_indices;
  for (int i = 1; i < argc; i++) {
    start_indices.push_back(atoi(argv[i]));
  }

  chain.add(Expression<N>(Function<N>(0b0000000011111111)));
  chain.add(Expression<N>(Function<N>(0b0000111100001111)));
  chain.add(Expression<N>(Function<N>(0b0011001100110011)));
  chain.add(Expression<N>(Function<N>(0b0101010101010101)));

  size_t current_best_length = 1000;
  vector<uint32_t> choices;
  uint32_t total_chains = 0;
  time_t last_print = time(NULL);
  BitSet<S> seen;
  find_optimal_chain(chain, current_best_length, choices, start_indices, seen,
                     0, total_chains, last_print, MAX_LENGTH);

  cout << "total chains: " << total_chains << endl;
  return 0;
}
