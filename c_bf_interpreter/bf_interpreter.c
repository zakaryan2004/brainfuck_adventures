#include <stdio.h>
#include <stdlib.h>

#define TAPELEN 30000

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
} CHARACTERS;

typedef struct Node {
    int position;
    struct Node *next;
} Node;


void preprocess(const unsigned char *string, int proglen, jump_map_entry *jump_map, int *jump_map_size) {
    int i = 0;
    // while (i < proglen) {
    //     switch (string[i]) {
    //         case INC_PTR:
    //         case DEC_PTR:
    //         case INC_VAL:
    //         case DEC_VAL:
    //         case OUTPUT:
    //         case INPUT:
    //         case JUMP_FWD:
    //         case JUMP_BCK: {

    //         }
    //     }
    // }

    int j = 0;

    Node *stack = NULL;

    while (i < proglen) {
        if (string[i] == JUMP_FWD) {
            Node *new_node = (Node *)malloc(sizeof(Node));
            new_node->position = i;
            new_node->next = stack;
            stack = new_node;
        }
        else if (string[i] == JUMP_BCK) {
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

void interpret_brainfuck(const unsigned char *code, int proglen) {
    unsigned char tape[TAPELEN] = {0};
    int pointer = 0;

    int i = 0;

    jump_map_entry jump_map[proglen];
    int jump_map_size = 0;
    preprocess(code, proglen, jump_map, &jump_map_size);

    while (i < proglen) {
        switch (code[i]) {
            case INC_PTR: {
                pointer = (pointer + 1) % TAPELEN;
                break;
            }
            case DEC_PTR: {
                pointer = (pointer - 1) % TAPELEN;
                break;
            }
            case INC_VAL: {
                tape[pointer]++;
                break;
            }
            case DEC_VAL: {
                tape[pointer]--;
                break;
            }
            case OUTPUT: {
                putchar(tape[pointer]);
                break;
            }
            case INPUT: {
                tape[pointer] = getchar();
                break;
            }
            case JUMP_FWD: {
                if (tape[pointer] == 0) {
                    i = find_jump(i, jump_map, jump_map_size);
                }
                break;
            }
            case JUMP_BCK: {
                if (tape[pointer] != 0) {
                    i = find_jump(i, jump_map, jump_map_size);
                }
                break;
            }
        }

        i++;
    }
}

int main() {
    unsigned char code[] = "+++++[>,.<-]";
    interpret_brainfuck(code, sizeof(code) - 1);
    printf("\n");

    return 0;
}