import readchar
from collections import deque

CHARACTERS = ['>', '<', '+', '-', '.', ',', '[', ']']

def interpret_brainfuck(string):
    tape = bytearray(30000)
    pointer = 0
    must_skip_count = 0
    jump_stack = deque()

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
    while i < len(string):
        ch = string[i]
        if ch not in CHARACTERS:
            i += 1
            continue

        if must_skip_count > 0:
            if ch == '[':
                must_skip_count += 1
            elif ch == ']':
                must_skip_count -= 1

            i += 1
            continue
        
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
                must_skip_count += 1

                i += 1
                continue
            jump_stack.appendleft(i)
        elif ch == ']':
            if tape[pointer] != 0:
                i = jump_stack.popleft()
                continue
            else:
                jump_stack.popleft()

        i += 1


