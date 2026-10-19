#include "spu.h"
#include "calc.h"
#include "colors.h"

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

void print_stack(stack_t* stk, FILE* file){
    for(size_t i = 0; i < stk->size; i++){
        fprintf(file, "  *[%3zu] - ", i);
        fprintf(file, "%lg\n", (float)stk->data[i] / accuracy);
    }
}

const char* print_command_name(command_code opCode){
    size_t com_len = sizeof(commands) / sizeof(processorOP);

    for(size_t i = 0; i < com_len; i++){
        if(commands[i].opCode == opCode){
            return commands[i].name;
        }
    }

    return " ";
}

const char* print_command_type(command_type opType){
    switch(opType){
        case NO_ARG:
            return "NO_ARG";
        case PUSH_DEC_ARG:
            return "PUSH_DEC";
        case PUSH_REG_ARG:
            return "PUSH_REG";
        case POP_REG_ARG:
            return "POP_REG";
        case J_POINT_ARG:
            return "J_POINT";
        default:
            return "ERROR";
    }
}

const char* get_arg_name(argumentCode arg){
    switch(arg){
        case AX_CODE:
            return "AX";
        case BX_CODE:
            return "BX";
        case CX_CODE:
            return "CX";
        default:
            return "ERROR";
    }
}
void print_byteCode(uint8_t* byteCode, size_t cIp, FILE* file, off_t byte_code_size, command_type opType){
    fprintf(file, "=== BYTECODE DUMP (cIp: 0x%04X / %d) ===\n", cIp, cIp);

    size_t i = 0;
    while(i < byte_code_size){
        if(cIp == i){
            fprintf(file, " -> [0x%04X] ", i);
        } else {
            fprintf(file, "    [0x%04X] ", i);
        }

        command_code cmd = (command_code) byteCode[i];
        const char* cmd_name = print_command_name(cmd);
        i += 2;

        fprintf(file, "%-10s (code: 0x%02X)", cmd_name, cmd);

        elem_t arg = 0;
        const char* argTxt = {};
        switch(opType){
            case NO_ARG:
                break;
            case PUSH_DEC_ARG:
            case J_POINT_ARG:
            case PUSH_REG_ARG:
                memcpy(&arg, &byteCode[i], sizeof(elem_t));
                i += sizeof(elem_t);
                if(opType == PUSH_DEC_ARG || opType == J_POINT_ARG){
                    fprintf(file, " arg = %d", arg);
                } else {
                    argTxt = get_arg_name((argumentCode) arg);
                    fprintf(file, " arg = %s", argTxt);
                }
                break;
        }
        fprintf(file, "\n");
    }

    fprintf(file, "============================================\n");
}

#ifdef DEBUG
void spu_dump(spu_t* spu, size_t command_ip, command_code opCode, command_type opType, FILE* file, off_t byte_code_size){
    fprintf(file, "\n");
    fprintf(file, "Command = %s, Type = %s\n", print_command_name(opCode), print_command_type(opType));

    print_byteCode(spu->byteCode, command_ip, file, byte_code_size, opType);

    fprintf(file, "AX = %d\nBX = %d\nCX = %d\n\n", spu->AX, spu->BX, spu->CX);

    print_stack(spu->stk, file);

    fprintf(file, LINE);
}
#else
void spu_dump(spu_t* spu, size_t command_ip, command_code opCode, command_type opType, FILE* file, off_t byte_code_size);
#endif

int main(){
    const int byte_code_fd = get_file_dp("byte_code.bin");
    off_t     byte_code_size = get_file_size(byte_code_fd);

    FILE* input = fopen("input.txt", "r");
    assert(input);

    #ifdef DEBUG
    FILE* spuDump = fopen("spu_dump.txt", "w");
    assert(spuDump);
    #endif

    spu_t spu = {.AX = 0, .BX = 0, .CX = 0};

    spu.byteCode = (uint8_t*) calloc(byte_code_size, sizeof(uint8_t));
    read(byte_code_fd, spu.byteCode, byte_code_size);

    spu.stk = STACK_INIT(10, "stack");
    spu.cIp = 0;

    while(spu.cIp < byte_code_size){
        size_t command_ip = spu.cIp;
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
                opType = (command_type) spu.byteCode[spu.cIp++];
                push(&spu, opType);
                break;
            }
            case POP_CODE:{
                opType = (command_type) spu.byteCode[spu.cIp++];
                pop(&spu, opType);
                break;
            }
            case OUT_CODE:
                out_value(spu.stk);
                break;
            case HLT_CODE:
                return 0;
            case ADD_CODE:
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
                spu.cIp = target;
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

        spu_dump(&spu, command_ip, opCode, opType, spuDump, byte_code_size);
    }

    free(spu.byteCode);
}
