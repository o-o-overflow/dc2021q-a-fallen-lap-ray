#!/usr/bin/env python3

import struct

target = 0x67616C66

def jenikns_hash_function(input, length):
    i = 0
    hash = 0
    while i != length:
        hash = hash + input[i]
        hash = hash + (hash << 10)
        hash = hash ^ (hash >> 6)
        i = i + 1

    hash = hash + (hash << 3)
    hash = hash ^ (hash >> 11)
    hash = hash + (hash << 15)

    return hash & 0xFFFFFFFF

print(jenikns_hash_function(struct.pack('<I', 88997722), 4))

for i in range(1582000000, 2**32):
    if i % 1000000 == 0:
        print(i)
    input = struct.pack('<I', i)
    hash = jenikns_hash_function(input, 4)
    if hash == target:
        print(f"FOUND: {i} {input} {hash}")
    if (hash ^ i) == target:
        print(f"FOUND CRAZY: {i} {input} {hash}")
            
