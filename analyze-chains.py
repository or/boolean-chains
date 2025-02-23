#!/usr/bin/env python3
import re
import sys
from glob import glob

CHAIN_PATTERN = re.compile(r"chain.*:\n( *x.*\n)+", re.MULTILINE)
FUNCTION_PATTERN = re.compile(r"[0-1]{10,16}")


def parse_chain(chain):
    return set(FUNCTION_PATTERN.findall(chain))


def find_chains(data):
    chains = []
    for match in CHAIN_PATTERN.finditer(data):
        chains.append(match.group(0))

    return chains


def stats():
    data = []
    for fn in sys.argv[1:]:
        with open(fn) as f:
            data.append(f.read())

    data = "\n".join(data)

    chains = find_chains(data)
    chain_sets = [set(parse_chain(chain)) for chain in chains]

    print(len(chain_sets), "chains")
    chain_set_map = {}
    for s in chain_sets:
        fs = frozenset(s)
        chain_set_map[fs] = chain_set_map.get(fs, 0) + 1

    print(len(chain_set_map), "unique chains")
    by_size = {}
    for fs in chain_set_map.keys():
        by_size[len(fs)] = by_size.get(len(fs), 0) + 1

    print("Chain sizes:")
    for size, count in by_size.items():
        print(size, ": ", count)


def get_smaller_chains():
    chain_sets = {}
    for fn in glob("output-partial-*-*.txt"):
        # parse out the two integers from the filename
        match = re.match(r"output-partial-(\d+)-(\d+).txt", fn)
        t1, t2 = match.groups()
        with open(fn) as f:
            chains = find_chains(f.read())
            chain_sets[frozenset((t1, t2))] = {
                frozenset(parse_chain(chain)) for chain in chains
            }

    for key, chains in sorted(chain_sets.items()):
        print(key, "has", len(chains), "chains")

    return chain_sets


def solve(chain_sets, current_targets, current_set, index):
    # print(index, len(current_targets), len(current_set))
    if len(current_set) > 23:
        return

    if len(current_targets) == 7:
        print("Found a solution!", len(current_set), current_set)
        return

    if len(current_targets) >= 4 and len(current_set) <= 17:
        print("Promising...", len(current_set), current_set)
        return

    for i in range(index, len(chain_sets)):
        if index == 0:
            print(i, "/", len(chain_sets))
        new_targets, new_set = chain_sets[i]
        combined = current_targets | new_targets
        if combined == current_targets:
            continue

        combined_set = current_set | new_set
        solve(chain_sets, combined, combined_set, i + 1)


def try_puzzle():
    chain_sets = []
    for key, value in get_smaller_chains().items():
        for v in value:
            chain_sets.append((key, v))

    solve(chain_sets, set(), set(), 0)


# try_puzzle()

stats()
