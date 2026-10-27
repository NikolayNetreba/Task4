#pragma once

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "config.h"
#include "stack.h"

#define ERROR_VALUE -1
#define BUFFER_SIZE 64
#define SPEC_BUFFER_SIZE "63"

#define SPU_SIGNATURE 0x31555053 // "SPU1"
#define SPU_VERSION 1

typedef struct {
    uint32_t signature;
    uint32_t version;
    size_t code_size;
    uint64_t hash;
} SPU_Header;

typedef struct {
    const char* name;
    size_t ip;
} label;

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
    LABEL_CODE = 17,
    IN_CODE = 18,
    CALL_CODE = 19,
    RET_CODE = 20,
    DISC_CODE = 21,
    PRAM_CODE = 22,
} command_code;

typedef enum {
    NO_ARG,
    DEC_ARG,
    REG_ARG,
    RAM_DEC_ARG,
    RAM_REG_ARG,
    RAM_REG_DEC_ARG,
} command_type;

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
    { "JNE", JNE_CODE },

    { "IN", IN_CODE },
    { "RET", RET_CODE },
    { "CALL", CALL_CODE },
    { "DISC", DISC_CODE },

    { "PRAM", PRAM_CODE }
};

typedef enum {
    AX_CODE = 101,
    BX_CODE = 102,
    CX_CODE = 103,
    JF_CODE = 104,
} registerCode;

typedef struct {
    const char* name;
    registerCode argCode;
} argOp;

const argOp argView[] = {
    { "AX", AX_CODE },
    { "BX", BX_CODE },
    { "CX", CX_CODE },
    { "JF", JF_CODE }
};

const size_t REG_CNT = sizeof(argView) / sizeof(argOp);

typedef struct {
    char* source_code; // буфер с исходником
    size_t source_size;
    size_t current_pos; // Текущая позиция в буфере
    size_t current_line; // Текущая строка (для вывода ошибок)

    uint8_t* byteCode; // Выходной массив байткода
    size_t currentByte; // Текущая позиция в массиве байткода
    label* labelTable; // Таблица меток
    size_t currentNumber; // Текущее количество меток
    int pass; // Номер прохода (1 или 2)
    int error_count;
} AsmContext;

typedef struct {
    command_code opCode;
    command_type opType;
    elem_t argValue1;
    elem_t argValue2;
} Instruction;

bool check_line(FILE* input);
void get_text_arg(elem_t* argValue, size_t argCnt, FILE* input);

off_t get_file_size(FILE* file);
void write_to_file(uint8_t* byteCode, size_t currentByte);

void assembly(AsmContext* ctx);
bool is_jump_command(command_code opCode);
