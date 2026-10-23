#pragma once

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "assembly.h"
#include "stack.h"

#define RAM_SIZE 1024

typedef struct {
    stack_t* stk;
    stack_t* call_stack;
    uint8_t* byteCode;
    size_t cIp;

    elem_t ram[RAM_SIZE];
    elem_t reg[REG_CNT];
} spu_t;

int get_file_dp(char* name);
off_t get_file_size(int fd);
void print_stack(stack_t* stk, FILE* file);
const char* print_command_name(command_code opCode);
const char* print_command_type(uint8_t opType);
const char* get_arg_name(registerCode arg);
void print_byteCode(uint8_t* byteCode, size_t cIp, FILE* file, off_t byte_code_size);
void spu_dump(spu_t* spu, size_t command_ip, command_code opCode, uint8_t opType, FILE* file, off_t byte_code_size);
bool get_condition(command_code opCode, elem_t A, elem_t B);
bool validate_header(int fd, off_t* expected_size);
