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

void push(spu_t* spu, uint8_t opType) {
    assert(spu && spu->stk);

    elem_t arg = 0;
    elem_t temp = 0;

    if (opType & (1 << 5)) { // DEC
        memcpy(&temp, &spu->byteCode[spu->cIp], sizeof(elem_t));
        spu->cIp += sizeof(elem_t);

        if ((opType & (1 << 7)) == 0 && (opType & (1 << 6)) == 0) {
            arg += temp * accuracy;
        } else {
            arg += temp;
        }
    }

    if (opType & (1 << 6)) { // REG
        memcpy(&temp, &spu->byteCode[spu->cIp], sizeof(elem_t));
        spu->cIp += sizeof(elem_t);

        if (opType & (1 << 7)) {
            arg += spu->reg[temp - AX_CODE] / accuracy;
        } else {
            arg += spu->reg[temp - AX_CODE];
        }
    }

    if (opType & (1 << 7)) { // RAM
        if (arg >= 0 && arg < RAM_SIZE)
            arg = spu->ram[arg];
        else {
            fprintf(stderr, "RAM segmentation fault");
            abort();
        }
    }

    push_stack(spu->stk, arg);
}

void pop(spu_t* spu, uint8_t opType) {
    assert(spu && spu->stk);

    elem_t val = 0;
    elem_t arg = 0;
    elem_t temp = 0;
    pop_stack(spu->stk, &val);

    if (opType & (1 << 5)) { // DEC
        memcpy(&temp, &spu->byteCode[spu->cIp], sizeof(elem_t));
        spu->cIp += sizeof(elem_t);

        if ((opType & (1 << 7)) == 0 && (opType & (1 << 6)) == 0) {
            fprintf(stderr, "sintaxis fault POP Const");
            abort();
        } else {
            arg += temp;
        }
    }

    if (opType & (1 << 6)) { // REG
        memcpy(&temp, &spu->byteCode[spu->cIp], sizeof(elem_t));
        spu->cIp += sizeof(elem_t);

        if (opType & (1 << 7)) { // RAM[REG]
            arg += spu->reg[temp - AX_CODE] / accuracy;
        } else {
            spu->reg[temp - AX_CODE] = val;
        }
    }

    if (opType & (1 << 7)) { // RAM
        if (arg >= 0 && arg < RAM_SIZE)
            spu->ram[arg] = val;
        else {
            fprintf(stderr, "RAM segmentation fault");
            abort();
        }
    }
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

    elem_t arg = 0;
    elem_t temp = 0;

    if (opType & (1 << 5)) { // DEC
        memcpy(&temp, &spu->byteCode[spu->cIp], sizeof(elem_t));
        spu->cIp += sizeof(elem_t);

        if ((opType & (1 << 7)) == 0 && (opType & (1 << 6)) == 0) {
            fprintf(stderr, "sintaxis fault IN into Const");
            abort();
        } else {
            arg += temp;
        }
    }

    if (opType & (1 << 6)) { // REG
        memcpy(&temp, &spu->byteCode[spu->cIp], sizeof(elem_t));
        spu->cIp += sizeof(elem_t);

        if (opType & (1 << 7)) {
            arg += spu->reg[temp - AX_CODE] / accuracy;
        } else {
            spu->reg[temp - AX_CODE] = val;
        }
    }

    if (opType & (1 << 7)) { // RAM
        if (arg >= 0 && arg < RAM_SIZE)
            spu->ram[arg] = val;
        else {
            fprintf(stderr, "RAM segmentation fault");
            abort();
        }
    }
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
    for (int i = 0; i < 100; i++) {
        if (i % 10 == 0) {
            printf("\n");
        }
        if (spu->ram[i])
            printf("%c", '#');
        else
            printf("%c", ' ');
    }
}
