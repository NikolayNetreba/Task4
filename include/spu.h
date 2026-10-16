#pragma once

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>

#include "assembly.h"
#include "stack.h"
#include "calc.h"

typedef struct{
    stack_t* stk;
    uint8_t* byteCode;
    size_t cIp;

    elem_t* ram;
    elem_t AX;
    elem_t BX;
    elem_t CX;
} spu_t;
