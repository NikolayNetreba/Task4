#pragma once

#include "stack.h"
#include "spu.h"
#include "assembly.h"

#define accuracy 1000

typedef enum{
    CALC_OK,
    CALC_INSUFFICIENT_ARG,
    CALC_DIVISION_BY_ZERO,
    CALC_SQRT_ERROR
} ErrorsCalc;

ErrorsCalc addition(stack_t *stack);
ErrorsCalc subtraction(stack_t* stack);
ErrorsCalc division(stack_t* stack);
ErrorsCalc multiply(stack_t* stack);
ErrorsCalc get_sqrt(stack_t* stack);
ErrorsCalc out_value(stack_t* stack);

void push(spu_t* spu, command_type opType);
void pop(spu_t* spu, command_type opType);
