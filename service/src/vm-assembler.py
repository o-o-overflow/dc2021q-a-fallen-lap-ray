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
Flag = collections.namedtuple('Flag', ['name', 'spec'])

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
    Syscall("read", 2),
    Syscall("write", 8),
    Syscall("exit", 32),
]

_FLAG_LIST = [
    Flag("<", 1),
    Flag(">", 2),
    Flag("==", 4),
    Flag("!=", 8),
    Flag("=0", 16),
]

REGISTERS = { r.name: r for r in _REGISTER_LIST }
OPCODES = { o.name: o for o in _OPCODE_LIST }
SYSCALLS = { s.name: s for s in _SYSCALL_LIST }
FLAGS = { f.name: f for f in _FLAG_LIST }

INSTRUCTION_SIZE = 4

def parse_arg(raw_arg, labels, data_mapping, error_on_fail=True):
    if raw_arg in REGISTERS:
        return REGISTERS[raw_arg]
    elif raw_arg in SYSCALLS:
        return SYSCALLS[raw_arg]
    elif raw_arg in FLAGS:
        return FLAGS[raw_arg]
    elif raw_arg in data_mapping:
        val = data_mapping[raw_arg]
        return Literal(str(val), val)
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

    if raw_arg in labels:
        dest = labels[raw_arg] * INSTRUCTION_SIZE
        return Literal(str(dest), dest)

    if error_on_fail:
        l.error(f"I don't know how to parse {raw_arg=}")
        sys.exit(-1)
        return -1
    else:
        return None

def parse_into_instructions(lines, labels, data_mapping, data):
    instructions = []
    i = 0
    for line in lines:
        i += 1
        line = line.strip()
        l.debug(f"Analyzing line {i}")
        if (not line) or line.startswith("#"):
            continue

        args = line.split()
        l.debug(f"{args=}")
        if len(args) == 1:
            assert(False)

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
            inst_arg1 = parse_arg(arg1, labels, data_mapping)
            inst_arg2 = parse_arg(arg2, labels, data_mapping)

            l.debug(f"instruction Line {i} {inst_opcodes=} {inst_arg1=} {inst_arg2=}")

            instructions.append(Instruction(inst_opcodes, inst_arg1, inst_arg2))
    l.debug(f"{instructions=}")
    # TODO: resolve lables
    return instructions

def output_instructions(instructions, data, output_file):
    size_inst = 0
    with open(output_file, 'wb') as output:
        for inst in instructions:
            opcode_value = 0
            for opcode in inst.opcodes:
                opcode_value ^= opcode.spec

            l.debug(f"{opcode_value=} {inst.arg1.spec=} {inst.arg2.spec=}")
            instruction_value = struct.pack('<BBH',
                                            opcode_value,
                                            inst.arg1.spec,
                                            inst.arg2.spec)
            size_inst += len(instruction_value)
            output.write(instruction_value)
        output.write(data)
    l.info(f"Program output to {output_file} has {size_inst} bytes code and {len(data)} bytes data total {size_inst+len(data)}")

def parse_labels(input):
    filtered_lines = []
    labels = {}
    i = 0
    for line in input:
        i += 1
        line = line.strip()
        args = line.split()

        if (not line) or line.startswith("#"):
            continue

        if len(args) == 1:
            the_label = args[0]
            if the_label == ".data":
                filtered_lines.append(line)
                continue

            if not the_label.endswith(':'):
                l.error(f"Label on line {i} does not end with a colon")
                sys.exit(-1)
            label = the_label.rstrip(':')

            if label in labels:
                l.error(f"Label on line {i} is {label}, however it's already defined")
                sys.exit(-1)

            if parse_arg(label, labels, {}, error_on_fail=False) is not None:
                l.error(f"Label on line {i} is {label}, but that's already a known")
                sys.exit(-1)

            labels[label] = len(filtered_lines)

            l.debug(f"Found {label=} {labels[label]=}")
        else:
            filtered_lines.append(line)
    return labels, filtered_lines

def parse_data_segment(lines):
    filtered_lines = []
    data_mapping = {}
    data_values = {}
    data = b""
    in_data_segment = False
    data_offset = None
    for line in lines:
        line = line.strip()
        if line == ".data":
            in_data_segment = True
            data_offset = len(filtered_lines) * INSTRUCTION_SIZE
        elif in_data_segment:
            args = line.split(maxsplit=1)
            if len(args) != 2:
                l.error(f"Malformed {args} in the .data segment")
                sys.exit(-1)
            name = args[0]
            if name in data_mapping:
                l.error(f"Error in .data segment, {name} already defined")
                sys.exit(-1)
            raw_value = args[1]
            value = eval(raw_value, data_values)
            l.debug(f"{raw_value=} {value=}")
            data_values[name] = value

            if isinstance(value, int):
                data_mapping[name] = data_offset
                stored_data = struct.pack('<q', value)
                data += stored_data
                data_offset += len(stored_data)
            elif isinstance(value, str):
                data_mapping[name] = data_offset
                stored_data = value.encode()
                data += stored_data
                data_offset += len(stored_data)
            else:
                l.error(f"Don't know how to handle the .data segment {name} {type(value)}")
                sys.exit(-1)

        else:
            filtered_lines.append(line)

    return data_mapping, data, filtered_lines

def main(input_file, output_file):

    with open(input_file, 'r') as input:
        labels, lines = parse_labels(input)
        data_mapping, data, lines = parse_data_segment(lines)
        instructions = parse_into_instructions(lines, labels, data_mapping, data)

    output_instructions(instructions, data, output_file)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(prog="vm-assembler")
    parser.add_argument("--debug", action="store_true", help="Enable debugging")
    parser.add_argument("--file", type=str, required=True, help="The file to assemble")
    parser.add_argument("--output", type=str, help="Where to write the binary output.")

    args = parser.parse_args()

    if args.debug:
        logging.basicConfig(level=logging.DEBUG)

    main(args.file, args.output or "output.bin")
