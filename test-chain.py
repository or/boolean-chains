#!/usr/bin/env python3
import re
import sys


def negate(bits):
    return "".join("1" if b == "0" else "0" for b in bits)


TARGETS = [
    [negate("1011011111100011"), "a"],
    [negate("1111100111100100"), "b"],
    [negate("1101111111110100"), "c"],
    [negate("1011011011011110"), "d"],
    [negate("1010001010111111"), "e"],
    [negate("1000111111110011"), "f"],
    ["0011111011111111", "g"],
]

for target, name in TARGETS:
    print(f"{name}: {target}")

# matches lines like x10 = x6 | x8 = 0011111011111111, parsing out the variable names and bits
OPERATION_PATTERN = re.compile(
    r"^ *(x[0-9]+|[!]?[a-g]) = (x[0-9]+) (.) (x[0-9]+) .*= ([01]+)"
)


def read_chain(filename):
    data = open(filename).read().split("\n")
    chain = []
    for line in data:
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
            print(f"{name} = {operand1} {operator} {operand2} = {bits}")

    return chain


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


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: test-chain.py <chain-file>")
        sys.exit(1)

    chain = read_chain(sys.argv[1])

    print("chain length: {0}".format(len(chain)))
    for k in range(16):
        inputs = {
            "x1": 1 if k & 8 else 0,
            "x2": 1 if k & 4 else 0,
            "x3": 1 if k & 2 else 0,
            "x4": 1 if k & 1 else 0,
        }

        result = evaluate_chain(chain, inputs)
        print(f"{k}, {k:04b}:")
        print(" {a} ".format(a="_" if not result["a"] else " "))
        print(
            "{f}{g}{b}".format(
                b="|" if not result["b"] else " ",
                f="|" if not result["f"] else " ",
                g="_" if result["g"] else " ",
            )
        )
        print(
            "{e}{d}{c}".format(
                c="|" if not result["c"] else " ",
                e="|" if not result["e"] else " ",
                d="_" if not result["d"] else " ",
            )
        )
        print()
