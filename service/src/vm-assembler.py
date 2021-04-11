#!/usr/bin/env python3
import argparse
import collections
import logging
import struct
import sys

l = logging.getLogger("vm-assembler")

Register = collections.namedtuple('Register', ['name', 'spec'])
Opcode = collections.namedtuple('Opcode', ['name', 'spec'])
Syscall = collections.namedtuple('Syscall', ['name', 'spec'])
Literal = collections.namedtuple('Literal', ['name', 'spec'])
Instruction = collections.namedtuple('Instruction', ['opcodes', 'arg1', 'arg2'])

_REGISTER_LIST = [
    Register("a", 1),
    Register("b", 2),
    Register("c", 4),
    Register("d", 8),
    Register("s", 16),
    Register("i", 32),
    Register("f", 64),
]

_OPCODE_LIST = [
    Opcode("imm", 1),
    Opcode("stk", 2),
    Opcode("add", 4),
    Opcode("stm", 8),
    Opcode("ldm", 16),
    Opcode("jmp", 32),
    Opcode("cmp", 64),
    Opcode("sys", 128),
]

_SYSCALL_LIST = [
    Syscall("open", 1),
    Syscall("write", 8),
    Syscall("exit", 32),
]

REGISTERS = { r.name: r for r in _REGISTER_LIST }
OPCODES = { o.name: o for o in _OPCODE_LIST }
SYSCALLS = { s.name: s for s in _SYSCALL_LIST }

def parse_arg(raw_arg):
    if raw_arg in REGISTERS:
        return REGISTERS[raw_arg]
    elif raw_arg in SYSCALLS:
        return SYSCALLS[raw_arg]

    try:
        val = int(raw_arg, base=10)
        return Literal(str(val), val)
    except ValueError:
        pass

    if raw_arg.lower().startswith('0x'):
        try:
            val = int(raw_arg, base=16)
            return Literal(hex(val), val)
        except ValueError:
            pass
    l.error(f"I don't know how to parse {raw_arg=}")
    sys.exit(-1)
    return -1

def parse_into_instructions(input):
    instructions = []
    i = 0
    for line in input:
        i += 1
        line = line.strip()
        l.debug(f"Analyzing line {i}")
        if (not line) or line.startswith("#"):
            continue

        args = line.split()
        l.debug(f"{args=}")
        if len(args) == 1:
            l.error("TODO lables")
        elif len(args) != 3:
            l.error(f"Don't know how to process a line with {len(args)} arguments on line {i}.")
            sys.exit(-1)
        else:
            opcodes = args[0]
            arg1 = args[1]
            arg2 = args[2]

            # Opcodes can be split by , if there's multiple
            inst_opcodes = []
            for op in opcodes.split(","):
                if not op in OPCODES:
                    l.error(f"Unknown opcode {op=} on line {i}")
                    sys.exit(-10)

                inst_opcodes.append(OPCODES[op])
            inst_arg1 = parse_arg(arg1)
            inst_arg2 = parse_arg(arg2)

            l.debug(f"instruction Line {i} {inst_opcodes=} {inst_arg1=} {inst_arg2=}")

            instructions.append(Instruction(inst_opcodes, inst_arg1, inst_arg2))
    l.debug(f"{instructions=}")
    # TODO: resolve lables
    return instructions

def output_instructions(instructions, output_file):
    with open(output_file, 'wb') as output:
        for inst in instructions:
            opcode_value = 0
            for opcode in inst.opcodes:
                opcode_value ^= opcode.spec

            instruction_value = struct.pack('<BBB',
                                            opcode_value,
                                            inst.arg1.spec,
                                            inst.arg2.spec)
            output.write(instruction_value)

def main(input_file, output_file):

    with open(input_file, 'r') as input:
        instructions = parse_into_instructions(input)

    output_instructions(instructions, output_file)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(prog="vm-assembler")
    parser.add_argument("--debug", action="store_true", help="Enable debugging")
    parser.add_argument("--file", type=str, required=True, help="The file to assemble")
    parser.add_argument("--output", type=str, help="Where to write the binary output.")

    args = parser.parse_args()

    if args.debug:
        logging.basicConfig(level=logging.DEBUG)

    main(args.file, args.output or "output.bin")
