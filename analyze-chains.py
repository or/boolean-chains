#!/usr/bin/env python3
import re
import sys

CHAIN_PATTERN = re.compile(r"chain:\n( *x.*\n)+", re.MULTILINE)
FUNCTION_PATTERN = re.compile(r"[0-1]{16}")


def parse_chain(chain):
    return set(FUNCTION_PATTERN.findall(chain))


def find_chains(data):
    chains = []
    for match in CHAIN_PATTERN.finditer(data):
        chains.append(match.group(0))

    return chains


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
    print(f"{size}: {count}")
