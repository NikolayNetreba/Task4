#include <assert.h>
#include <math.h>
#include <stdio.h>

#include "commands.h"

ErrorsCalc addition(stack_t* stack) {
    assert(stack);
    if (stack->size < 2) {
        return CALC_INSUFFICIENT_ARG;
    }

    elem_t val1 = 0, val2 = 0;
    pop_stack(stack, &val1);
    pop_stack(stack, &val2);

    push_stack(stack, val1 + val2);

    return CALC_OK;
}

ErrorsCalc subtraction(stack_t* stack) {
    assert(stack);
    if (stack->size < 2) {
        return CALC_INSUFFICIENT_ARG;
    }

    elem_t val1 = 0, val2 = 0;
    pop_stack(stack, &val2);
    pop_stack(stack, &val1);

    push_stack(stack, val1 - val2);

    return CALC_OK;
}

ErrorsCalc division(stack_t* stack) {
    assert(stack);
    if (stack->size < 2) {
        return CALC_INSUFFICIENT_ARG;
    }

    elem_t val1 = 0, val2 = 0;
    pop_stack(stack, &val1);
    pop_stack(stack, &val2);

    if (val1 == 0) {
        return CALC_DIVISION_BY_ZERO;
    }

    push_stack(stack, val2 * accuracy / val1);

    return CALC_OK;
}

ErrorsCalc multiply(stack_t* stack) {
    assert(stack);
    if (stack->size < 2) {
        return CALC_INSUFFICIENT_ARG;
    }

    elem_t val1 = 0, val2 = 0;
    pop_stack(stack, &val1);
    pop_stack(stack, &val2);

    push_stack(stack, val1 * val2 / accuracy);

    return CALC_OK;
}

ErrorsCalc get_sqrt(stack_t* stack) {
    assert(stack);
    if (stack->size < 1) {
        return CALC_INSUFFICIENT_ARG;
    }

    elem_t val1 = 0;
    pop_stack(stack, &val1);

    if (val1 < 0) {
        return CALC_SQRT_ERROR;
    }

    push_stack(stack, sqrt(val1 / accuracy) * accuracy);

    return CALC_OK;
}

ErrorsCalc out_value(stack_t* stack) {
    assert(stack);
    if (stack->size < 1) {
        return CALC_INSUFFICIENT_ARG;
    }
    elem_t val1 = 0;
    pop_stack(stack, &val1);

    ELEM_PRINT((float)val1 / accuracy);

    return CALC_OK;
}

elem_t* get_arg_ptr(spu_t* spu, uint8_t opType, bool is_modify) {
    static elem_t arg = 0;
    elem_t temp = 0;
    elem_t* res = NULL;

    if (opType & (1 << 5)) {
        memcpy(&temp, &spu->byteCode[spu->cIp], sizeof(elem_t));
        spu->cIp += sizeof(elem_t);

        if ((opType & (1 << 6)) == 0 && (opType & (1 << 7)) == 0) {
            if (is_modify) {
                assert(0 && "sintaxis fault POP/IN into Const");
            }

            arg = temp * accuracy;
            res = &arg;
        } else {
            arg += temp;
        }
    }

    if (opType & (1 << 6)) {
        memcpy(&temp, &spu->byteCode[spu->cIp], sizeof(elem_t));
        spu->cIp += sizeof(elem_t);

        if (opType & (1 << 7)) {
            arg += spu->reg[temp - AX_CODE] / accuracy;
        } else {
            res = &spu->reg[temp - AX_CODE];
        }
    }

    if (opType & (1 << 7)) {
        if (arg >= 0 && arg < RAM_SIZE) {
            res = &spu->ram[arg];
        } else {
            assert(0 && "RAM segmentation fault");
        }
    }

    return res;
}

void push(spu_t* spu, uint8_t opType) {
    assert(spu && spu->stk);

    elem_t* arg = get_arg_ptr(spu, opType, false);

    push_stack(spu->stk, *arg);
}

void pop(spu_t* spu, uint8_t opType) {
    assert(spu && spu->stk);

    elem_t val = 0;
    pop_stack(spu->stk, &val);

    elem_t* arg = get_arg_ptr(spu, opType, true);

    *arg = val;
}

void do_in(spu_t* spu, uint8_t opType) {
    assert(spu);

    elem_t val = 0;

    int cnt = 0;
    do {
        printf("\nenter a number: ");
        cnt = scanf("%d", &val);
    } while (cnt != 1);

    val *= accuracy;

    if (opType == NO_ARG) {
        push_stack(spu->stk, val);
        return;
    }

    elem_t* arg = get_arg_ptr(spu, opType, true);

    *arg = val;
}

elem_t get_jmp_arg(spu_t* spu) {
    elem_t arg = 0;
    memcpy(&arg, &spu->byteCode[spu->cIp], sizeof(elem_t));
    spu->cIp += sizeof(elem_t);

    return arg;
}

void do_call(spu_t* spu) {
    elem_t target = get_jmp_arg(spu);

    push_stack(spu->call_stack, spu->cIp);

    spu->cIp = target;
}

void do_ret(spu_t* spu) {
    elem_t target = 0;

    pop_stack(spu->call_stack, &target);

    spu->cIp = target;
}

void calc_disc(spu_t* spu) {
    elem_t A = 0, B = 0, C = 0;

    pop_stack(spu->stk, &A);
    pop_stack(spu->stk, &B);
    pop_stack(spu->stk, &C);

    A /= accuracy;
    B /= accuracy;
    C /= accuracy;

    elem_t DD = B * B - 4 * A * C;
    push_stack(spu->stk, DD * accuracy);
}
void print_ram(spu_t* spu) {
    for (int i = 0; i < 400; i++) {
        if (i % 20 == 0) {
            printf("\n");
        }
        if (spu->ram[i])
            printf("%c", '#');
        else
            printf("%c", ' ');
    }
}
