import readchar
import re

CHARACTERS = ['>', '<', '+', '-', '.', ',', '[', ']']

def preprocess(string):
    pattern = re.compile("[><+-.,\[\]]+", re.MULTILINE)
    matches = pattern.findall(string)
    preprocessed = ''.join(matches)

    jump_map = {}
    jump_stack = []

    for i in range(len(preprocessed)):
        if preprocessed[i] == '[':
            jump_stack.append(i)
        elif preprocessed[i] == ']':
            open_i = jump_stack.pop()
            jump_map[open_i] = i
            jump_map[i] = open_i

    return preprocessed, jump_map


def interpret_brainfuck(string):
    tape = bytearray(30000)
    pointer = 0

    def incp():
        nonlocal pointer
        pointer = (pointer + 1) % len(tape)
    
    def decp():
        nonlocal pointer
        pointer = (pointer - 1) % len(tape)

    def inctape():
        nonlocal tape
        tape[pointer] = (tape[pointer] + 1) % 256
    
    def dectape():
        nonlocal tape
        tape[pointer] = (tape[pointer] - 1) % 256

    i = 0
    program, jump_map = preprocess(string)
    while i < len(program):
        ch = program[i]
        
        if ch == '>':
            incp()
        elif ch == '<':
            decp()
        elif ch == '+':
            inctape()
        elif ch == '-':
            dectape()
        elif ch == '.':
            print(chr(tape[pointer]), end='', flush=True)
        elif ch == ',':
            tape[pointer] = ord(readchar.readchar())
        elif ch == '[':
            if tape[pointer] == 0:
                i = jump_map[i]
        elif ch == ']':
            if tape[pointer] != 0:
                i = jump_map[i]

        i += 1
