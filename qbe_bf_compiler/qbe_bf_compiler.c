#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 2^15 = 32768, MUST be a power of 2!
#define TAPELEN (1 << 15)
#define MAX_JUMPS TAPELEN
#define CMD_BUF_SIZE 1024

typedef struct {
    int from;
    int to;
} jump_map_entry;

typedef enum {
    INC_PTR = '>',
    DEC_PTR = '<',
    INC_VAL = '+',
    DEC_VAL = '-',
    OUTPUT  = '.',
    INPUT   = ',',
    JUMP_FWD= '[',
    JUMP_BCK= ']'
} CHARACTER;

typedef struct Node {
    int position;
    struct Node *next;
} Node;

typedef enum {
    OP_NONE,
    OP_PTR,
    OP_VAL
} OpType;

OpType get_op_type(char ch) {
    if (ch == INC_PTR || ch == DEC_PTR) return OP_PTR;
    if (ch == INC_VAL || ch == DEC_VAL) return OP_VAL;
    return OP_NONE;
}

int CONTINUE_COUNTER = 0;

void preprocess(FILE *fi_ptr, jump_map_entry *jump_map, int *jump_map_size) {
    int i = 0;
    int j = 0;
    char ch;
    Node *stack = NULL;

    while (1) {
        ch = fgetc(fi_ptr);
        if (ch == EOF) {
            break;
        }

        if (ch == JUMP_FWD) {
            Node *new_node = (Node *)malloc(sizeof(Node));
            if (!new_node) {
                perror("malloc at preprocess");
                exit(EXIT_FAILURE);
            }

            new_node->position = i;
            new_node->next = stack;
            stack = new_node;
        }
        else if (ch == JUMP_BCK) {
            if (stack == NULL) {
                fprintf(stderr, "Error: Unmatched ']' at position %d\n", i);
                exit(EXIT_FAILURE);
            }
            if (j >= MAX_JUMPS) {
                fprintf(stderr, "Error: Too many loops (max %d)\n", MAX_JUMPS);
                exit(EXIT_FAILURE);
            }

            Node *temp = stack;
            int open_i = stack->position;
            stack = stack->next;
            free(temp);

            jump_map[j++] = (jump_map_entry){open_i, i};
            jump_map[j++] = (jump_map_entry){i, open_i};
        }

        i++;
    }

    *jump_map_size = j;
}


int find_jump(int position, jump_map_entry *jumpmap, int jump_map_size) {
    for (int i = 0; i < jump_map_size; i++) {
        if (jumpmap[i].from == position) {
            return jumpmap[i].to;
        }
    }
    return -1;
}


void generate_operation(FILE *fo_ptr, OpType *op, int *inc) {
    if (*inc == 0 || *op == OP_NONE) {
        *op = OP_NONE;
        *inc = 0;
        return;
    }

    if (*op == OP_PTR) {
        fprintf(fo_ptr, "    # pointer += %%d;\n");
        fprintf(fo_ptr, "    %%tape_idx =l loadl $p\n");
        fprintf(fo_ptr, "    %%tape_idx =l add %%tape_idx, %d\n", *inc * 4);
        fprintf(fo_ptr, "    %%tape_idx =l and %%tape_idx, %d\n", (TAPELEN - 1) * 4);
        fprintf(fo_ptr, "    storel %%tape_idx, $p\n");
    }
    else if (*op == OP_VAL) {
        fprintf(fo_ptr, "    # tape[pointer] += %%d;\n");
        fprintf(fo_ptr, "    %%tape_idx =l loadl $p\n");
        fprintf(fo_ptr, "    %%tape_pointer =l add %%base, %%tape_idx\n");
        fprintf(fo_ptr, "    %%temp_val =w loadw %%tape_pointer\n");
        fprintf(fo_ptr, "    %%temp_val =w add %%temp_val, %d\n", *inc);
        fprintf(fo_ptr, "    %%temp_val =w and %%temp_val, 255\n");
        fprintf(fo_ptr, "    storew %%temp_val, %%tape_pointer\n");
    }
    *op = OP_NONE;
    *inc = 0;
}


void compile_brainfuck(FILE *fi_ptr, FILE *fo_ptr) {
    int i = 0;
    char ch;
    jump_map_entry jump_map[TAPELEN];
    int jump_map_size = 0;

    OpType current_op;
    int current_inc = 0;

    preprocess(fi_ptr, jump_map, &jump_map_size);
    fseek(fi_ptr, 0, SEEK_SET);

    fprintf(fo_ptr, "data $p = { l 0 }\n");
    fprintf(fo_ptr, "data $real_tape = { z %d }\n", TAPELEN * 4);
    fprintf(fo_ptr, "data $tape = { l $real_tape }\n\n");
    fprintf(fo_ptr, "export function w $main() {\n");
    fprintf(fo_ptr, "@start\n");
    fprintf(fo_ptr, "    %%base =l loadl $tape\n");

    while (1) {
        ch = fgetc(fi_ptr);

        if (ch == EOF) {
            break;
        }

        if (ch != INC_PTR && ch != DEC_PTR &&
            ch != INC_VAL && ch != DEC_VAL &&
            ch != OUTPUT && ch != INPUT &&
            ch != JUMP_FWD && ch != JUMP_BCK
        ) {
            i++;
            continue;
        }

        OpType new_op = get_op_type(ch);

        if (new_op != current_op) {
            generate_operation(fo_ptr, &current_op, &current_inc);
        }

        switch (ch) {
            case INC_PTR: {
                current_op = OP_PTR;
                current_inc += 1;
                break;
            }
            case DEC_PTR: {
                current_op = OP_PTR;
                current_inc -= 1;
                break;
            }
            case INC_VAL: {
                current_op = OP_VAL;
                current_inc += 1;
                break;
            }
            case DEC_VAL: {
                current_op = OP_VAL;
                current_inc -= 1;
                break;
            }
            case OUTPUT: {
                fprintf(fo_ptr, "    # putchar(tape[pointer]);\n");
                fprintf(fo_ptr, "    %%tape_idx =l loadl $p\n");
                fprintf(fo_ptr, "    %%tape_pointer =l add %%base, %%tape_idx\n");
                fprintf(fo_ptr, "    %%temp_val =w loadw %%tape_pointer\n");
                fprintf(fo_ptr, "    call $putchar(w %%temp_val)\n");
                break;
            }
            case INPUT: {
                fprintf(fo_ptr, "    # tape[pointer] = getchar();\n");
                fprintf(fo_ptr, "    %%temp_val =w call $getchar()\n");
                fprintf(fo_ptr, "    %%tape_idx =l loadl $p\n");
                fprintf(fo_ptr, "    %%tape_pointer =l add %%base, %%tape_idx\n");
                fprintf(fo_ptr, "    storew %%temp_val, %%tape_pointer\n");
                break;
            }
            case JUMP_FWD: {
                int jump_idx = find_jump(i, jump_map, jump_map_size);
                fprintf(fo_ptr, "    # if (tape[pointer] == 0) { goto l%d; }    l%d:\n", jump_idx, i);
                fprintf(fo_ptr, "    %%tape_idx =l loadl $p\n");
                fprintf(fo_ptr, "    %%tape_pointer =l add %%base, %%tape_idx\n");
                fprintf(fo_ptr, "    %%temp_val =w loadw %%tape_pointer\n");
                fprintf(fo_ptr, "    jnz %%temp_val, @continue%d, @l%d\n", CONTINUE_COUNTER, jump_idx);
                fprintf(fo_ptr, "@continue%d\n", CONTINUE_COUNTER++);
                fprintf(fo_ptr, "@l%d\n", i);
                break;
            }
            case JUMP_BCK: {
                int jump_idx = find_jump(i, jump_map, jump_map_size);
                fprintf(fo_ptr, "    # if (tape[pointer] != 0) { goto l%d; }    l%d:\n", jump_idx, i);
                fprintf(fo_ptr, "    %%tape_idx =l loadl $p\n");
                fprintf(fo_ptr, "    %%tape_pointer =l add %%base, %%tape_idx\n");
                fprintf(fo_ptr, "    %%temp_val =w loadw %%tape_pointer\n");
                fprintf(fo_ptr, "    jnz %%temp_val, @l%d, @continue%d\n", jump_idx, CONTINUE_COUNTER);
                fprintf(fo_ptr, "@continue%d\n", CONTINUE_COUNTER++);
                fprintf(fo_ptr, "@l%d\n", i);
                break;
            }
        }

        i++;
    }
    fprintf(fo_ptr, "\n    ret 0\n}");
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: bfc <source>\n");
        exit(EXIT_FAILURE);
    }

    FILE *fi_ptr = fopen(argv[1], "r");

    if (!fi_ptr) {
        printf("Cannot open source file.\n");
        exit(EXIT_FAILURE);
    }

    char *source_name_copy = strdup(argv[1]);
    char *fileName = strtok(source_name_copy, ".");

    char cmdbuf[CMD_BUF_SIZE];

    snprintf(cmdbuf, CMD_BUF_SIZE, "%s.ssa", fileName);
    FILE *fo_ptr = fopen(cmdbuf, "w");

    printf("Compiling BF...\n");
    compile_brainfuck(fi_ptr, fo_ptr);
    printf("Compilation finished.\n");

    fflush(fo_ptr);
    fclose(fo_ptr);
    fclose(fi_ptr);

    printf("Generating assembly with QBE...\n");
    snprintf(cmdbuf, CMD_BUF_SIZE, "qbe %s.ssa > %s.s", fileName, fileName);
    system(cmdbuf);
    printf("Assembly generation finished.\n");

    printf("Assembling and linking with cc...\n");
    snprintf(cmdbuf, CMD_BUF_SIZE, "cc -o %s %s.s", fileName, fileName);
    system(cmdbuf);
    printf("Executable generation finished.\n");
    printf("All done!\n");

    // snprintf(cmdbuf, CMD_BUF_SIZE, "rm %s.c", fileName);
    // system(cmdbuf);

    return 0;
}
