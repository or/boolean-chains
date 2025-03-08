#!/usr/bin/env python3
import os
import re
import sys
from glob import glob

CHAIN_PATTERN = re.compile(r"chain.*:\n( *x.*\n)+", re.MULTILINE)
FUNCTION_PATTERN = re.compile(r"[0-1]{10,16}")
OPERATION_PATTERN = re.compile(
    r"^ *(x[0-9]+|[!]?[a-g]) = (x[0-9]+) (.) (x[0-9]+) .*= ([01]+)"
)


def negate(bits):
    return "".join("1" if b == "0" else "0" for b in bits)


TARGETS = [
    [negate("1011011111100011"), "~a"],
    [negate("1111100111100100"), "~b"],
    [negate("1101111111110100"), "~c"],
    [negate("1011011011011110"), "~d"],
    [negate("1010001010111111"), "~e"],
    [negate("1000111111110011"), "~f"],
    ["0011111011111111", "g"],
]


def parse_chain(chain):
    return FUNCTION_PATTERN.findall(chain)


def find_chains(data):
    chains = []
    for match in CHAIN_PATTERN.finditer(data):
        chains.append(match.group(0))

    return chains


def apply_operation(op, g, h):
    g = int(g, 2)
    h = int(h, 2)
    if op == "&":
        r = g & h
    elif op == "|":
        r = g | h
    elif op == "^":
        r = g ^ h
    elif op == "<":
        r = (~g) & h
    elif op == ">":
        r = g & (~h)

    return bin(r)[2:].rjust(16, "0")


def extract_possible_chains(chain, so_far=None, used=None):
    result = []

    if so_far is None:
        so_far = [
            (0, None, None, None, chain[0], "0000000011111111"),
            (1, None, None, None, chain[1], "0000111100001111"),
            (2, None, None, None, chain[2], "0011001100110011"),
            (3, None, None, None, chain[3], "0101010101010101"),
        ]

    if used is None:
        used = set()

    if len(used) == len(chain) - 4:
        return [so_far]

    for i in range(len(so_far), len(chain)):
        if chain[i] in used:
            continue

        f = int(chain[i], 2)

        next_steps = []
        for j in range(len(so_far)):
            g = int(chain[j], 2)
            for k in range(j + 1, len(so_far)):
                h = int(chain[k], 2)
                ops = []
                if g & h == f:
                    ops.append("&")

                if g | h == f:
                    ops.append("|")

                if g ^ h == f:
                    ops.append("^")

                if (~g) & h == f:
                    ops.append("<")

                if g & (~h) == f:
                    ops.append(">")

                if ops:
                    for op in ops:
                        full_function = apply_operation(op, so_far[j][5], so_far[k][5])
                        next_steps.append((i, op, j, k, chain[i], full_function))

        for step in next_steps:
            result += extract_possible_chains(
                chain, so_far=so_far + [step], used=used | {chain[i]}
            )

    return result


def evaluate_chain(chain, inputs):
    result = {}
    for operation in chain:
        # print(inputs, operation)
        input1 = inputs[operation["operand1"]]
        input2 = inputs[operation["operand2"]]
        operator = operation["operator"]
        name = operation["name"]
        bits = operation["bits"]
        if operator == "|":
            inputs[name] = input1 | input2
        elif operator == "&":
            inputs[name] = input1 & input2
        elif operator == "^":
            inputs[name] = input1 ^ input2
        elif operator == "<":
            inputs[name] = 1 if input1 < input2 else 0
        elif operator == ">":
            inputs[name] = 1 if input1 > input2 else 0

        # print(bits)
        for [target_bits, target] in TARGETS:
            if target_bits.startswith(bits):
                result[target] = inputs[name]
                break

    # print(result)
    return result


def read_chain(chain_steps):
    chain = []
    for line in chain_steps:
        match = OPERATION_PATTERN.match(line)
        if match:
            name, operand1, operator, operand2, bits = match.groups()
            chain.append(
                {
                    "name": name,
                    "operand1": operand1,
                    "operator": operator,
                    "operand2": operand2,
                    "bits": bits,
                }
            )

    return chain


def extracted_chain_to_steps(extracted_chain):
    steps = []
    for i, op, j, k, result, full_result in extracted_chain:
        s = f"x{i + 1}"
        if op:
            s += f" = x{j + 1} {op} x{k + 1}"

        s += f" = {result}"
        if len(result) != len(full_result):
            s += f" [{full_result}]"

        for [target_bits, target] in TARGETS:
            if target_bits.startswith(result):
                s += f" = {target}"
                break

        steps.append(s)

    return steps


def ascii_digits(chain_steps):
    chain = read_chain(chain_steps)
    digits = []
    for k in range(16):
        inputs = {
            "x1": 1 if k & 8 else 0,
            "x2": 1 if k & 4 else 0,
            "x3": 1 if k & 2 else 0,
            "x4": 1 if k & 1 else 0,
        }

        result = evaluate_chain(chain, inputs)
        s1 = " {a} ".format(a="_" if not result["~a"] else " ")
        s2 = "{f}{g}{b}".format(
            b="|" if not result["~b"] else " ",
            f="|" if not result["~f"] else " ",
            g="_" if result["g"] else " ",
        )
        s3 = "{e}{d}{c}".format(
            c="|" if not result["~c"] else " ",
            e="|" if not result["~e"] else " ",
            d="_" if not result["~d"] else " ",
        )
        digits.append((k, s1, s2, s3))

    return digits


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

    extracted_chains = []
    for parsed_chain in parsed_chains:
        extracted_chains += extract_possible_chains(parsed_chain)

    grouped_by_full_output_sets = {}
    for extracted_chain in extracted_chains:
        full_output = tuple(
            sorted(frozenset(x[5] for x in extracted_chain if is_target(x[4])))
        )
        if full_output not in grouped_by_full_output_sets:
            grouped_by_full_output_sets[full_output] = {}

        unique_operations_set = frozenset(
            extracted_chain[x[2]][5] + x[1] + extracted_chain[x[3]][5]
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

extract_chains()
