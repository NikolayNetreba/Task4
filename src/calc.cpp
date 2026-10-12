#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "stack.h"

#define accuracy 1000

typedef enum{
    CALC_OK,
    CALC_INSUFFICIENT_ARG,
    CALC_DIVISION_BY_ZERO,
    CALC_SQRT_ERROR
} ErrorsCalc;

ErrorsCalc addition(stack_t *stack) {
    assert( stack );
    if (stack->size < 2){
        return CALC_INSUFFICIENT_ARG;
    }

    elem_t val1 = 0, val2 = 0;
    pop_stack(stack, &val1);
    pop_stack(stack, &val2);

    push_stack(stack, val1 + val2);

    return CALC_OK;
}

ErrorsCalc subtraction(stack_t* stack){
    assert( stack );
    if (stack->size < 2){
        return CALC_INSUFFICIENT_ARG;
    }

    elem_t val1 = 0, val2 = 0;
    pop_stack(stack, &val1);
    pop_stack(stack, &val2);

    push_stack(stack, val2 - val1);

    return CALC_OK;
}

ErrorsCalc division(stack_t* stack){
    assert( stack );
    if (stack->size < 2){
        return CALC_INSUFFICIENT_ARG;
    }

    elem_t val1 = 0, val2 = 0;
    pop_stack(stack, &val1);
    pop_stack(stack, &val2);

    if(val1 == 0){
        return CALC_DIVISION_BY_ZERO;
    }

    push_stack(stack, val2 * accuracy / val1);

    return CALC_OK;
}

ErrorsCalc multiply(stack_t* stack){
    assert( stack );
    if (stack->size < 2){
        return CALC_INSUFFICIENT_ARG;
    }

    elem_t val1 = 0, val2 = 0;
    pop_stack(stack, &val1);
    pop_stack(stack, &val2);

    push_stack(stack, val1 * val2);
}

ErrorsCalc get_sqrt(stack_t* stack){
    assert( stack );
    if (stack->size < 1){
        return  CALC_INSUFFICIENT_ARG;
    }

    elem_t val1 = 0;
    pop_stack(stack, &val1);

    if(val1 < 0){
        return CALC_SQRT_ERROR;
    }

    push_stack(stack, sqrt( val1 / accuracy ) * accuracy);

    return  CALC_OK;
}

ErrorsCalc out_value(stack_t* stack){
    assert( stack );
    if(stack->size < 1){
        return CALC_INSUFFICIENT_ARG;
    }
    elem_t val1 = 0;
    pop_stack(stack, &val1);

    ELEM_PRINT((float)val1 / accuracy);

    return CALC_OK;
}

int main(){
    stack_t* stack = STACK_INIT(10, "stack");

    char buff[10] = {};
    elem_t value = 0;

    while(1){
        scanf("%s", buff);
        if(buff[0] == 'P'){

            scanf("%d", &value);

            push_stack(stack, value * accuracy);

        } else if(buff[0] == 'A'){
            addition(stack);
        } else if(buff[0] == 'S'){
            subtraction(stack);
        } else if(buff[0] == 'D'){
            division(stack);
        } else if(buff[0] == 'O'){
            out_value(stack);
        } else if(buff[0] == 'H'){
            return 0;
        }
    }
}
