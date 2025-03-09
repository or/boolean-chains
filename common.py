import re

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


def read_chain_for_evaluation(chain_steps):
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

        for [target_bits, target] in TARGETS:
            if target_bits.startswith(bits):
                result[target] = inputs[name]
                break

    return result


def ascii_digits(chain_steps):
    chain = read_chain_for_evaluation(chain_steps)
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
        digits.append((k, s1, s2, s3, result))

    return digits
