#include "spu.h"
#include "colors.h"
#include "commands.h"

int main(int argc, char* argv[]) {
    char* file_name = "byte_code.bin";
    if (argc == 2) {
        file_name = argv[1];
    }

    const int byte_code_fd = get_file_dp(file_name);

    off_t byte_code_size = 0;
    if (!validate_header(byte_code_fd, &byte_code_size)) {
        close(byte_code_fd);
        return 1;
    }

    FILE* spuDump = 0;
#ifdef DEBUG
    spuDump = fopen("spu_dump.txt", "w");
    assert(spuDump);
#endif

    spu_t spu = { };

    spu.byteCode = (uint8_t*)calloc(byte_code_size, sizeof(uint8_t));
    read(byte_code_fd, spu.byteCode, byte_code_size);

    spu.stk = STACK_INIT(10, "stack");
    spu.call_stack = STACK_INIT(10, "call stack");
    spu.cIp = 0;

    while (spu.cIp < byte_code_size) {
        size_t command_ip = spu.cIp; // for dump
        command_code opCode = (command_code)(spu.byteCode[spu.cIp] & 31);
        uint8_t opType = spu.byteCode[spu.cIp++] & (7 << 5);

        switch (opCode) {
        case PUSH_CODE: {
            push(&spu, opType);

            break;
        }
        case POP_CODE: {
            pop(&spu, opType);

            break;
        }
        case OUT_CODE:
            out_value(spu.stk);

            break;
        case HLT_CODE:
            close(byte_code_fd);
            if (spuDump)
                fclose(spuDump);
            free(spu.byteCode);
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
        case IN_CODE:
            do_in(&spu, opType);

            break;
        case CALL_CODE:
            do_call(&spu);

            break;
        case RET_CODE:
            do_ret(&spu);

            break;
        case DISC_CODE:
            calc_disc(&spu);

            break;
        case PRAM_CODE:
            print_ram(&spu);

            break;
        case JMP_CODE: {
            elem_t target = get_jmp_arg(&spu);
            spu.cIp = target;

            break;
        }
        case JA_CODE:
        case JB_CODE:
        case JE_CODE:
        case JAE_CODE:
        case JBE_CODE:
        case JNE_CODE: {
            elem_t target = get_jmp_arg(&spu);

            elem_t A = 0, B = 0;
            pop_stack(spu.stk, &B);
            pop_stack(spu.stk, &A);

            bool condition = get_condition(opCode, A, B);

            if (condition) {
                spu.cIp = target;
            }
            break;
        }
        default:
            break;
        }

        spu_dump(&spu, command_ip, opCode, opType, spuDump, byte_code_size);
    }

    free(spu.byteCode);
}

bool get_condition(command_code opCode, elem_t A, elem_t B) {
    bool condition = false;
    switch (opCode) {
    case JA_CODE:
        condition = (A > B);
        break;
    case JB_CODE:
        condition = (A < B);
        break;
    case JE_CODE:
        condition = (A == B);
        break;
    case JAE_CODE:
        condition = (A >= B);
        break;
    case JBE_CODE:
        condition = (A <= B);
        break;
    case JNE_CODE:
        condition = (A != B);
        break;
    default:
        break;
    }

    return condition;
}

int get_file_dp(char* name) {
    int fd = open(name, O_RDONLY);
    assert(fd != ERROR_VALUE && "Error file descriptor");

    return fd;
}

off_t get_file_size(int fd) {
    struct stat fileInfo = { };
    assert(fstat(fd, &fileInfo) != ERROR_VALUE && "Error fstat");

    return fileInfo.st_size;
}

bool validate_header(int fd, off_t* expected_size) {
    SPU_Header header = { 0 };
    ssize_t bytes_read = read(fd, &header, sizeof(SPU_Header));

    if (bytes_read != sizeof(SPU_Header)) {
        fprintf(stderr, "Error: File is too small or missing header.\n");
        return false;
    }

    if (header.signature != SPU_SIGNATURE) {
        fprintf(stderr, "Error: Invalid signature. Expected 0x%X, got 0x%X\n", SPU_SIGNATURE, header.signature);
        return false;
    }

    if (header.version != SPU_VERSION) {
        fprintf(stderr, "Error: Unsupported version. Expected %d, got %d\n", SPU_VERSION, header.version);
        return false;
    }

    *expected_size = header.code_size;
    return true;
}

//============== DUMP ===========================

void print_stack(stack_t* stk, FILE* file) {
    for (size_t i = 0; i < stk->size; i++) {
        fprintf(file, "  *[%3zu] - ", i);
        fprintf(file, "%lg, [0x%04zX] \n", (float)stk->data[i] / accuracy, stk->data[i]);
    }
}

const char* print_command_type(uint8_t opType) {
    if (opType == 0)
        return "NO_ARG";
    if (opType == (1 << 5))
        return "DEC_ARG";
    if (opType == (1 << 6))
        return "REG_ARG";
    if (opType == ((1 << 7) | (1 << 5)))
        return "RAM_DEC_ARG";
    if (opType == ((1 << 7) | (1 << 6)))
        return "RAM_REG_ARG";
    if (opType == ((1 << 7) | (1 << 6) | (1 << 5)))
        return "RAM_REG_DEC_ARG";
    return "ERROR";
}

const char* get_arg_name(registerCode argCode) {
    for (size_t i = 0; i < REG_CNT; i++) {
        if (argView[i].argCode == argCode) {
            return argView[i].name;
        }
    }

    return "ERROR";
}

const char* get_command_name(command_code opCode) {
    size_t com_len = sizeof(commands) / sizeof(processorOP);

    for (size_t i = 0; i < com_len; i++) {
        if (commands[i].opCode == opCode) {
            return commands[i].name;
        }
    }

    return " ";
}

bool is_jump_command(command_code opCode) {
    return opCode == JA_CODE || opCode == JAE_CODE || opCode == JB_CODE || opCode == JBE_CODE || opCode == JE_CODE || opCode == JNE_CODE || opCode == JMP_CODE;
}

void print_byteCode(uint8_t* byteCode, size_t cIp, FILE* file, off_t byte_code_size) {
    fprintf(file, "=== BYTECODE DUMP (cIp: 0x%04zX / %zu) ===\n", cIp, cIp);

    size_t i = 0;
    while (i < (size_t)byte_code_size) {
        if (cIp == i) {
            fprintf(file, " -> [0x%04zX] ", i);
        } else {
            fprintf(file, "    [0x%04zX] ", i);
        }

        uint8_t current_byte = byteCode[i];
        command_code cmd = (command_code)(current_byte & 31);
        uint8_t opType = current_byte & (7 << 5);

        i++;

        fprintf(file, "%-6s (0x%02X)", get_command_name(cmd), current_byte);

        elem_t dec_val = 0;
        elem_t reg_val = 0;

        bool has_dec = opType & (1 << 5);
        bool has_reg = opType & (1 << 6);
        bool has_ram = opType & (1 << 7);

        if (has_dec) {
            memcpy(&dec_val, &byteCode[i], sizeof(elem_t));
            i += sizeof(elem_t);
        }
        if (has_reg) {
            memcpy(&reg_val, &byteCode[i], sizeof(elem_t));
            i += sizeof(elem_t);
        }

        if (has_dec || has_reg || has_ram) {
            fprintf(file, " arg = ");
            if (has_ram)
                fprintf(file, "[ ");

            if (has_reg) {
                fprintf(file, "%s", get_arg_name((registerCode)reg_val));
                if (has_dec)
                    fprintf(file, " + ");
            }
            if (has_dec) {
                if (cmd == CALL_CODE || is_jump_command(cmd)) {
                    fprintf(file, "[0x%04zX]", dec_val);
                } else {
                    fprintf(file, "%d", dec_val);
                }
            }

            if (has_ram)
                fprintf(file, " ]");
        }

        fprintf(file, "\n");
    }

    fprintf(file, "============================================\n");
}

void dump_register(FILE* file, spu_t* spu) {
    for (size_t i = 0; i < REG_CNT; i++) {
        fprintf(file, "%s = %d\n", argView[i].name, spu->reg[i] / accuracy);
    }
    fprintf(file, "\n");
}

#ifdef DEBUG
void spu_dump(spu_t* spu, size_t command_ip, command_code opCode, uint8_t opType, FILE* file, off_t byte_code_size) {
    if (!file)
        return;

    fprintf(file, "\n");
    fprintf(file, "Command = %s, Type = %s\n", get_command_name(opCode), print_command_type(opType));

    print_byteCode(spu->byteCode, command_ip, file, byte_code_size);

    dump_register(file, spu);

    fprintf(file, "--- STACK ---\n");
    print_stack(spu->stk, file);

    fprintf(file, "--- CALL STACK ---\n");
    print_stack(spu->call_stack, file);

    fprintf(file, LINE);
}
#else
inline void spu_dump(spu_t* spu, size_t command_ip, command_code opCode, uint8_t opType, FILE* file, off_t byte_code_size) {
}
#endif
