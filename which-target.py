#!/usr/bin/env python3
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

bits = sys.argv[1]
for [target_bits, target] in TARGETS:
    if target_bits.startswith(bits):
        print(target)
