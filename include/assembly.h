#pragma once

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>

#include "config.h"

#define ERROR_VALUE -1
#define BUFFER_SIZE 10
#define SPEC_BUFFER_SIZE "9"

typedef struct{
    const char* name;
    size_t ip;
} label;

typedef enum{
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

typedef enum{
    NO_ARG = 0,
    PUSH_DEC_ARG = 51,
    PUSH_REG_ARG = 52,
    POP_REG_ARG = 53,
    J_POINT_ARG = 54,
} command_type;

typedef struct{
    const char* name;
    command_code opCode;
} processorOP;

processorOP commands[] = {
    {"PUSH", PUSH_CODE},
    {"POP", POP_CODE},
    {"OUT", OUT_CODE},
    {"HLT", HLT_CODE},

    {"ADD", ADD_CODE},
    {"SUB", SUB_CODE},
    {"MUL", MUL_CODE},
    {"DIV", DIV_CODE},
    {"SQRT", SQRT_CODE},

    {"JMP", JMP_CODE},
    {"JA",  JA_CODE},
    {"JB",  JB_CODE},
    {"JE",  JE_CODE},
    {"JAE", JAE_CODE},
    {"JBE", JBE_CODE},
    {"JNE", JNE_CODE}
};

typedef enum{
    AX_CODE = 101,
    BX_CODE = 102,
    CX_CODE = 103
} argumentCode;

typedef struct{
    const char* name;
    argumentCode argCode;
} argOp;

argOp arg[] = {
    {"AX", AX_CODE},
    {"BX", BX_CODE},
    {"CX", CX_CODE},
};

bool check_line(FILE* input);
void get_text_arg(elem_t* argValue, size_t argCnt, FILE* input);

off_t get_file_size(FILE* file);
void write_to_file(uint8_t* byteCode, size_t currentByte);

void assembly(FILE* input, uint8_t* byteCode, label* labelTable, size_t* currentByte, size_t* currentNumber, const int number);

bool is_jump_command(command_code opCode);
