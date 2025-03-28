#!/usr/bin/env python3
import re
import sys


def negate(bits):
    return "".join("1" if b == "0" else "0" for b in bits)


TARGETS = [
    ["0010010000010000", "1"],
    ["0100000000000001", "2"],
    ["0001000100000100", "3"],
    ["0000000001000001", "4"],
    ["0000000000000100", "5"],
    ["0000010000010000", "6"],
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
        print("Usage: test-chain-ex-59.py <chain-file>")
        sys.exit(1)

    chain = read_chain(sys.argv[1])

    for k in range(64):
        inputs = {
            "x5": 1 if k & 32 else 0,
            "x6": 1 if k & 16 else 0,
            "x1": 1 if k & 8 else 0,
            "x2": 1 if k & 4 else 0,
            "x3": 1 if k & 2 else 0,
            "x4": 1 if k & 1 else 0,
        }

        t = "{}{}".format(inputs["x5"], inputs["x6"])
        s = "{}{}{}{}".format(inputs["x1"], inputs["x2"], inputs["x3"], inputs["x4"])
        result = evaluate_chain(chain, inputs)
        is_prime_raw = (
            (t == "00" and s in ["0010", "0101", "1011"])
            or (t == "01" and s in ["0001", "1111"])
            or (t in ["00", "01"] and s in ["0011", "0111", "1101"])
            or (t == "10" and s in ["1001", "1111"])
            or (t == "11" and s in ["1101"])
            or (t in ["10", "11"] and s in ["0101", "1011"])
        )
        is_prime = (
            (t == "00" and result["1"] == 1)
            or (t == "01" and result["2"] == 1)
            or (t in ["00", "01"] and result["3"] == 1)
            or (t == "10" and result["4"] == 1)
            or (t == "11" and result["5"] == 1)
            or (t in ["10", "11"] and result["6"] == 1)
        )
        print(f"{k}, {k:06b}, {t}, {s}:", is_prime_raw, is_prime)
        assert is_prime_raw == is_prime
