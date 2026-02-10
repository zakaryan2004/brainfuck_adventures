#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAPELEN 65536

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
            new_node->position = i;
            new_node->next = stack;
            stack = new_node;
        }
        else if (ch == JUMP_BCK) {
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
        char sign = *inc > 0 ? '+' : '-';
        fprintf(fo_ptr, "    pointer %c= %d;\n", sign, abs(*inc));
    }
    else if (*op == OP_VAL) {
        fprintf(fo_ptr, "    tape[pointer] += %d;\n", *inc);
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

    fprintf(fo_ptr, "#include <stdio.h>\n#include <stdint.h>\n\n");
    fprintf(fo_ptr, "int main() {\n");
    fprintf(fo_ptr, "    unsigned char tape[%d] = {0};\n", TAPELEN);
    fprintf(fo_ptr, "    uint16_t pointer = 0;\n\n");

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
                fprintf(fo_ptr, "    putchar(tape[pointer]);\n");
                break;
            }
            case INPUT: {
                fprintf(fo_ptr, "    tape[pointer] = getchar();\n");
                break;
            }
            case JUMP_FWD: {
                fprintf(fo_ptr, "    if (tape[pointer] == 0) { goto l%d; }\n    l%d:\n", find_jump(i, jump_map, jump_map_size), i);
                break;
            }
            case JUMP_BCK: {
                fprintf(fo_ptr, "    if (tape[pointer] != 0) { goto l%d; }\n    l%d:", find_jump(i, jump_map, jump_map_size), i);
                break;
            }
        }

        i++;
    }

    fprintf(fo_ptr, "\n    return 0;\n}");
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

    char cmdbuf[512];

    sprintf(cmdbuf, "%s.c", fileName);
    FILE *fo_ptr = fopen(cmdbuf, "w");

    compile_brainfuck(fi_ptr, fo_ptr);

    fflush(fo_ptr);
    fclose(fo_ptr);
    fclose(fi_ptr);

    sprintf(cmdbuf, "cc -o %s %s.c", fileName, fileName);
    system(cmdbuf);

    // sprintf(cmdbuf, "rm %s.c", fileName);
    // system(cmdbuf);

    return 0;
}
