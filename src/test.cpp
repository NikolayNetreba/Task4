#include <stdio.h>
#include <string.h>

typedef enum {
    NO_ARG,
    DEC_ARG,
    REG_ARG,
    RAM_DEC_ARG,
    RAM_REG_ARG,
    RAM_REG_DEC_ARG
} command_type;

typedef enum {
    NOT_A_CODE = 0,
    PUSH_CODE = 1,
    POP_CODE = 2,
    OUT_CODE = 3,
    HLT_CODE = 4,
    ADD_CODE = 5,
    SUB_CODE = 6,
    MUL_CODE = 7,
    DIV_CODE = 8,
    SQRT_CODE = 9,
    JMP_CODE = 10,
    JA_CODE = 11,
    JB_CODE = 12,
    JE_CODE = 13,
    JAE_CODE = 14,
    JBE_CODE = 15,
    JNE_CODE = 16,
    LABEL_CODE = 17
} command_code;

typedef struct {
    const char* name;
    command_code opCode;
} processorOP;

const processorOP commands[] = {
    { "PUSH", PUSH_CODE },
    { "POP", POP_CODE },
    { "OUT", OUT_CODE },
    { "HLT", HLT_CODE },

    { "ADD", ADD_CODE },
    { "SUB", SUB_CODE },
    { "MUL", MUL_CODE },
    { "DIV", DIV_CODE },
    { "SQRT", SQRT_CODE },

    { "JMP", JMP_CODE },
    { "JA", JA_CODE },
    { "JB", JB_CODE },
    { "JE", JE_CODE },
    { "JAE", JAE_CODE },
    { "JBE", JBE_CODE },
    { "JNE", JNE_CODE }
};

command_code get_opCode(char* temp, size_t commandsCnt) {
    for (size_t i = 0; i < commandsCnt; i++) {
        if (strcmp(temp, commands[i].name) == 0) {
            return commands[i].opCode;
        }
    }

    return NOT_A_CODE;
}

int main() {
    int a;
    scanf("%d", &a);
    printf("%lg", (float)a);
}
