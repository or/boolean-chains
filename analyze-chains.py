#!/usr/bin/env python3
import os
import re
import sys
from glob import glob

from common import TARGETS, ascii_digits

CHAIN_PATTERN = re.compile(r"chain.*:\n( *x.*\n)+", re.MULTILINE)
FUNCTION_PATTERN = re.compile(r"[0-1]{10,16}")


def parse_chain(chain):
    return FUNCTION_PATTERN.findall(chain)


def find_chains(data):
    chains = []
    for match in CHAIN_PATTERN.finditer(data):
        chains.append(match.group(0))

    return chains


def apply_operation(op, g, h):
    if op == "&":
        return g & h

    if op == "|":
        return g | h

    if op == "^":
        return g ^ h

    if op == "<":
        return (~g) & h

    if op == ">":
        return g & (~h)


def extract_possible_chains(
    chain,
    so_far=None,
    remaining=None,
    expressions=None,
    expressions_index=None,
    seen=None,
):
    result = []

    if not seen:
        seen = set()

    if so_far is None:
        chain = [int(x, 2) for x in chain]
        so_far = [
            (None, None, None, chain[0], int("0000000011111111", 2)),
            (None, None, None, chain[1], int("0000111100001111", 2)),
            (None, None, None, chain[2], int("0011001100110011", 2)),
            (None, None, None, chain[3], int("0101010101010101", 2)),
        ]

    if remaining is None:
        remaining = {chain[i] for i in range(4, len(chain))}

    if not remaining:
        return [so_far]

    if not expressions:
        expressions = []
        expressions_index = 0
        for k in range(1, len(so_far)):
            h = so_far[k][3]
            full_h = so_far[k][4]
            for j in range(0, k):
                g = so_far[j][3]
                full_g = so_far[j][4]

                f = g & h
                full_f = full_g & full_h
                if f in remaining and full_f not in seen:
                    expressions.append((j, "&", k, f, full_f))

                f = g | h
                full_f = full_g | full_h
                if f in remaining and full_f not in seen:
                    expressions.append((j, "|", k, f, full_f))

                f = g ^ h
                full_f = full_g ^ full_h
                if f in remaining and full_f not in seen:
                    expressions.append((j, "^", k, f, full_f))

                f = (~g) & h
                full_f = (~full_g) & full_h
                if f in remaining and full_f not in seen:
                    expressions.append((j, "<", k, f, full_f))

                f = g & (~h)
                full_f = full_g & (~full_h)
                if f in remaining and full_f not in seen:
                    expressions.append((j, ">", k, f, full_f))

    else:
        expressions = expressions[:]
        k = len(so_far) - 1
        h = so_far[k][3]
        full_h = so_far[k][4]
        for j in range(0, k):
            g = so_far[j][3]
            full_g = so_far[j][4]

            f = g & h
            full_f = full_g & full_h
            if f in remaining and full_f not in seen:
                expressions.append((j, "&", k, f, full_f))

            f = g | h
            full_f = full_g | full_h
            if f in remaining and full_f not in seen:
                expressions.append((j, "|", k, f, full_f))

            f = g ^ h
            full_f = full_g ^ full_h
            if f in remaining and full_f not in seen:
                expressions.append((j, "^", k, f, full_f))

            f = (~g) & h
            full_f = (~full_g) & full_h
            if f in remaining and full_f not in seen:
                expressions.append((j, "<", k, f, full_f))

            f = g & (~h)
            full_f = full_g & (~full_h)
            if f in remaining and full_f not in seen:
                expressions.append((j, ">", k, f, full_f))

    for i in range(expressions_index, len(expressions)):
        j, op, k, f, full_f = expressions[i]
        if f not in remaining:
            continue
        next_remaining = remaining - {f}
        next_seen = seen | {full_f}
        result += extract_possible_chains(
            chain,
            so_far=so_far + [expressions[i]],
            remaining=next_remaining,
            expressions=expressions,
            expressions_index=i + 1,
            seen=next_seen,
        )

    return result


def extracted_chain_to_steps(extracted_chain):
    steps = []
    for i, (j, op, k, f, full_f) in enumerate(extracted_chain):
        s = f"x{i + 1}"
        if op:
            s += f" = x{j + 1} {op} x{k + 1}"

        s += f" = {f}"
        if len(f) != len(full_f):
            s += f" [{full_f}]"

        for [target_bits, target] in TARGETS:
            if target_bits.startswith(f):
                s += f" = {target}"
                break

        steps.append(s)

    return steps


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

    min_size = 1000
    print("Chain sizes:")
    for size, count in by_size.items():
        print(size, ": ", count)
        min_size = min(min_size, size)

    os.makedirs("generated", exist_ok=True)
    uniquely_generated = set()
    for i, chain in enumerate(chains):
        parsed_chain = parse_chain(chain)
        fs = frozenset(parsed_chain)
        if fs in uniquely_generated:
            continue
        uniquely_generated.add(fs)
        with open(f"generated-{len(parsed_chain)}-{i}.txt", "w") as f:
            f.write(chain)


def is_target(function):
    for [target_bits, _target] in TARGETS:
        if target_bits.startswith(function):
            return True

    return False


def extract_chains():
    data = []
    for fn in sys.argv[1:]:
        with open(fn) as f:
            data.append(f.read())

    data = "\n".join(data)

    chains = find_chains(data)
    parsed_chains = [parse_chain(chain) for chain in chains]

    min_size = min(len(x) for x in parsed_chains)
    num_bits = len(parsed_chains[0][0])

    parsed_chains = [x for x in parsed_chains if len(x) == min_size]

    raw_extracted_chains = []
    for parsed_chain in parsed_chains:
        raw_extracted_chains += extract_possible_chains(parsed_chain)

    extracted_chains = []
    for chain in raw_extracted_chains:
        new_chain = []
        for j, op, k, f, full_f in chain:
            new_chain.append(
                (
                    j,
                    op,
                    k,
                    bin(f)[2:].rjust(num_bits, "0"),
                    bin(full_f)[2:].rjust(16, "0"),
                )
            )

        extracted_chains.append(new_chain)

    grouped_by_full_output_sets = {}
    for extracted_chain in extracted_chains:
        full_output = tuple(
            sorted(frozenset(x[4] for x in extracted_chain if is_target(x[3])))
        )
        if full_output not in grouped_by_full_output_sets:
            grouped_by_full_output_sets[full_output] = {}

        unique_operations_set = frozenset(
            extracted_chain[x[0]][4] + x[1] + extracted_chain[x[2]][4]
            for x in extracted_chain
            if x[1]
        )

        if (
            unique_operations_set not in grouped_by_full_output_sets[full_output]
            or extracted_chain
            < grouped_by_full_output_sets[full_output][unique_operations_set]
        ):
            grouped_by_full_output_sets[full_output][unique_operations_set] = (
                extracted_chain
            )

    N = len(parsed_chains[0][0])
    filename = f"publish/release/chains-{N}-{min_size}.txt"
    print(f"writing to {filename}")
    with open(filename, "w") as f:
        for group_id, key in enumerate(sorted(grouped_by_full_output_sets.keys())):
            unique_chains = grouped_by_full_output_sets[key].values()
            f.write(f"Group {group_id} ({len(unique_chains)} unique chains):\n\n")
            for candidate in sorted(unique_chains):
                chain_steps = extracted_chain_to_steps(candidate)
                f.write("\n".join(chain_steps))
                f.write("\n\n")
                digits = ascii_digits(chain_steps)
                for i in range(16):
                    d = digits[i][0]
                    f.write(f"{d: 3d} ")
                f.write("\n")
                for i in range(16):
                    s1 = digits
                    s1 = digits[i][1]
                    f.write(f"{s1.rjust(4)}")
                f.write("\n")
                for i in range(16):
                    s2 = digits
                    s2 = digits[i][2]
                    f.write(f"{s2.rjust(4)}")
                f.write("\n")
                for i in range(16):
                    s3 = digits
                    s3 = digits[i][3]
                    f.write(f"{s3.rjust(4)}")
                f.write("\n")
                f.write("\n------\n\n")


def find_shortest_chain(chain, target):
    involved_steps = set()
    next_steps = []
    for i, step in enumerate(chain):
        if target.startswith(step[-2]):
            next_steps.append(i)
            break

    while next_steps:
        # print(target, next_steps, involved_steps)
        index = next_steps.pop(0)
        (left, _, right, *_) = chain[index]
        involved_steps.add(index)

        if (
            left is not None
            and left >= 4
            and left not in involved_steps
            and left not in next_steps
        ):
            next_steps.append(left)
        if (
            right is not None
            and right >= 4
            and right not in involved_steps
            and right not in next_steps
        ):
            next_steps.append(right)

    return involved_steps


def find_single_target_chain_lengths():
    data = []
    for fn in sys.argv[1:]:
        with open(fn) as f:
            data.append(f.read())

    data = "\n".join(data)

    chains = find_chains(data)
    parsed_chains = [parse_chain(chain) for chain in chains]

    min_size = min(len(x) for x in parsed_chains)
    num_bits = len(parsed_chains[0][0])

    parsed_chains = [x for x in parsed_chains if len(x) == min_size]

    raw_extracted_chains = []
    for parsed_chain in parsed_chains:
        raw_extracted_chains += extract_possible_chains(parsed_chain)

    extracted_chains = []
    for chain in raw_extracted_chains:
        new_chain = []
        for j, op, k, f, full_f in chain:
            new_chain.append(
                (
                    j,
                    op,
                    k,
                    bin(f)[2:].rjust(num_bits, "0"),
                    bin(full_f)[2:].rjust(16, "0"),
                )
            )

        extracted_chains.append(new_chain)

    for extracted_chain in extracted_chains:
        for target in TARGETS:
            target_chain = find_shortest_chain(extracted_chain, target[0])
            print(target, len(target_chain))


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

# stats()

# extract_chains()

find_single_target_chain_lengths()
