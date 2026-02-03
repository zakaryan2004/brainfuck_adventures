import io
import sys
from brainfuck_interpreter import interpret_brainfuck
from unittest.mock import MagicMock

TEST_COUNT = 1
ORIG_STDIN = sys.stdin
ORIG_STDOUT = sys.stdout

def test(program, input, expected_output):
    global TEST_COUNT
    sys.stdin = io.StringIO(input)
    sys.stdin.fileno = MagicMock(return_value=0)
    sys.stdout = io.StringIO()
    
    interpret_brainfuck(program)
    output = sys.stdout.getvalue()
    print(f"Test {TEST_COUNT}...", file=ORIG_STDOUT)
    assert output == expected_output, f"Expected: {expected_output}, Got: {output}"
    print("Passed!", file=ORIG_STDOUT)
    TEST_COUNT += 1



test("++++++++++.", "", chr(10))
test(",.", "A", "A")
test(",.,.,.,.,.", "Hello", "Hello")
test("+++++[>,.<-]", "Hello", "Hello")
test("-.", "", chr(255))
test("-.+.", "", chr(255) + chr(0))

test("""
+++++ +++++       10
>+++++ +++++ ++   12
<
[>[>+>+<<-]>>[<<+>>-]<<<-]     multiply two cells
>>.
""", "", chr(120)) # also 'x'



