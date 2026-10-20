#pragma once

#include "assembly.h"
#include "spu.h"
#include "stack.h"

#define accuracy 1000

typedef enum {
    CALC_OK,
    CALC_INSUFFICIENT_ARG,
    CALC_DIVISION_BY_ZERO,
    CALC_SQRT_ERROR
} ErrorsCalc;

ErrorsCalc addition(stack_t* stack);
ErrorsCalc subtraction(stack_t* stack);
ErrorsCalc division(stack_t* stack);
ErrorsCalc multiply(stack_t* stack);
ErrorsCalc get_sqrt(stack_t* stack);
ErrorsCalc out_value(stack_t* stack);

elem_t get_jmp_arg(spu_t* spu);

void push(spu_t* spu, uint8_t opType);
void pop(spu_t* spu, uint8_t opType);
void do_in(spu_t* spu, uint8_t opType);
void do_call(spu_t* spu);
void do_ret(spu_t* spu);
void calc_disc(spu_t* spu);
