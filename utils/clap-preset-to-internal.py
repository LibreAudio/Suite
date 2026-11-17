#!/usr/bin/env python

import json
import sys

if len(sys.argv) != 3:
    print(f"usage: {sys.argv[0]} <$dsp-parameters.hpp> <preset-file>")
    sys.exit(1)

def untitle(s):
    return s[0].lower() + s[1:]

with open(sys.argv[1], "r") as fh:
    params = [untitle(p.strip().replace("kFaustParameter", "", 1)) for p in fh.read().split("enum FaustParameterIndex : uint16_t {", 1)[1].split("}", 1)[0].split(",")][:-1]

with open(sys.argv[2], "rb") as fh:
    values = json.loads(fh.read().split(b"__dpf_state_begin__", 1)[1].split(b"\"parameters\":",1)[1].split(b"}",1)[0] + b"}")

for symbol in params:
    if symbol in values:
        print(values[symbol], end=", ")
    else:
        print("-0", end=",")

print()
