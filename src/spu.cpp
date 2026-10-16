#include "spu.h"

int get_file_dp(char* name){
    int fd = open(name, O_RDONLY | O_BINARY);
    assert(fd != ERROR_VALUE && "Error file descriptor");

    return fd;
}

off_t get_file_size(int fd){
    struct stat fileInfo = {};
    assert(fstat(fd, &fileInfo) != ERROR_VALUE && "Error fstat");

    return fileInfo.st_size;
}

elem_t get_jmp_arg(spu_t* spu, command_type* opType){
    *opType = (command_type) spu->byteCode[(spu->cIp)++];
    elem_t arg = 0;
    memcpy(&arg, &spu->byteCode[spu->cIp], sizeof(elem_t));
    spu->cIp += sizeof(elem_t);

    return arg;
}

void print_stack(stack_t* stack, spu_t spu, char* commandName){
    fprintf(stderr, LINE);
    fprintf(stderr, "%s", commandName);
    fprintf(stderr, "AX = %d\nBX = %d\nCX = %d\n", spu.AX, spu.BX, spu.CX);
    for(size_t i = 0; i < stack->capacity; i++){
        if(i < stack->size){
            fprintf(stderr, "  *[%3zu] - ", i);
            ELEM_PRINT(stack->data[i]);
        } else {
            fprintf(stderr, "   [%3zu] - POISON: ", i);
            ELEM_PRINT(stack->data[i]);
        }
    }
    fprintf(stderr, LINE);
}

int main(){
    const int byte_code_fd = get_file_dp("byte_code.bin");
    off_t     byte_code_size = get_file_size(byte_code_fd);

    FILE* input = fopen("input.txt", "r");
    assert(input);

    spu_t spu = {.AX = 0, .BX = 0, .CX = 0};

    spu.byteCode = (uint8_t*) calloc(byte_code_size, sizeof(uint8_t));
    read(byte_code_fd, spu.byteCode, byte_code_size);

    spu.stk = STACK_INIT(10, "stack");
    spu.cIp = 0;

    while(spu.cIp < byte_code_size){
        command_code opCode = (command_code) spu.byteCode[spu.cIp++];
        command_type opType = NO_ARG;
        // char comandName[32] = {};
        // do {
        //     fgets(comandName, 31, input);
        //     if(comandName[0] == ':')
        //         fprintf(stderr, "flag = %s\n", comandName);
        // } while(comandName[0] == ':');

        switch(opCode){ // TODO сделать бинпоиск по структуре из кода команды и указателя на функцию.
            case PUSH_CODE:{
                // fprintf(stderr, "push\n");
                opType = (command_type) spu.byteCode[spu.cIp++];
                push(&spu, opType);
                break;
            }
            case POP_CODE:{
                // fprintf(stderr, "pop\n");
                opType = (command_type) spu.byteCode[spu.cIp++];
                pop(&spu, opType);
                break;
            }
            case OUT_CODE:
                // fprintf(stderr, "out\n");
                out_value(spu.stk);
                break;
            case HLT_CODE:
                return 0;
            case ADD_CODE:
                // fprintf(stderr, "add\n");
                addition(spu.stk);
                break;
            case SUB_CODE:
                subtraction(spu.stk);
                break;
            case MUL_CODE:
                multiply(spu.stk);
                break;
            case DIV_CODE:
                division(spu.stk);
                break;
            case SQRT_CODE:
                get_sqrt(spu.stk);
                break;
            case JMP_CODE:{
                elem_t target = get_jmp_arg(&spu, &opType);
                break;
            }
            case JA_CODE:
            case JB_CODE:
            case JE_CODE:
            case JAE_CODE:
            case JBE_CODE:
            case JNE_CODE:{
                elem_t target = get_jmp_arg(&spu, &opType);

                elem_t A = 0, B = 0;
                pop_stack(spu.stk, &A);
                pop_stack(spu.stk, &B);
                bool condition = false;
                switch(opCode){
                    case JA_CODE: condition = (A > B); break;
                    case JB_CODE: condition = (A < B); break;
                    case JE_CODE: condition = (A == B); break;
                    case JAE_CODE: condition = (A >= B); break;
                    case JBE_CODE: condition = (A <= B); break;
                    case JNE_CODE: condition = (A != B); break;
                    default: break;
                }

                if(condition){
                    spu.cIp = target;
                }
                break;
            }
            default: break;
        }
        // print_stack(stack, spu, comandName);
    }



    free(spu.byteCode);
}
