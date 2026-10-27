#include "assembly.h"

#define DEBUG
int main(int argc, char* argv[]) {
    char* file_name = "input.txt";
    if (argc == 2) {
        file_name = argv[1];
    }

    int fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    struct stat fileInfo;
    if (fstat(fd, &fileInfo) < 0) {
        perror("Error getting file status");
        close(fd);
        return EXIT_FAILURE;
    }

    off_t inputSize = fileInfo.st_size;

    // 2. Чтение всего файла в буфер
    char* source_buffer = (char*)calloc(inputSize + 1, sizeof(char));
    if (read(fd, source_buffer, inputSize) < 0) {
        perror("Error reading file");
        free(source_buffer);
        close(fd);
        return EXIT_FAILURE;
    }
    close(fd);

    AsmContext ctx = {
        .source_code = source_buffer,
        .source_size = (size_t)inputSize,
        .current_pos = 0,
        .current_line = 1,
        .byteCode = (uint8_t*)calloc(inputSize, sizeof(uint16_t)),
        .currentByte = 0,
        .labelTable = (label*)calloc(inputSize / 2, sizeof(label)),
        .currentNumber = 0,
        .pass = 1,
        .error_count = 0,
    };

    // First pass (collecting labels)
    assembly(&ctx);

    if (ctx.error_count > 0) {
        fprintf(stderr, "Error count: %d", ctx.error_count);
        free(ctx.source_code);
        free(ctx.byteCode);
        free(ctx.labelTable);

        return EXIT_FAILURE;
    }

    // Подготовка ко второму проходу
    ctx.currentByte = 0;
    ctx.current_pos = 0;
    ctx.current_line = 1;
    ctx.pass = 2;

    // Second pass (bytecode generation)
    assembly(&ctx);

    write_to_file(ctx.byteCode, ctx.currentByte);

    free(ctx.source_code);
    free(ctx.byteCode);
    free(ctx.labelTable);

    return 0;
}

void skip_comment(AsmContext* ctx) {
    while (ctx->current_pos < ctx->source_size) {
        char ch = ctx->source_code[ctx->current_pos];

        if (ch == '\n') {
            ctx->current_line++;
            ctx->current_pos++;
        } else if (isspace(ch)) {
            ctx->current_pos++;
        } else if (ch == '#') {
            while (ctx->current_pos < ctx->source_size && ctx->source_code[ctx->current_pos] != '\n') {
                ctx->current_pos++;
            }
        } else {
            break;
        }
    }
}

bool get_next_token(AsmContext* ctx, char* token_out, size_t max_len) {
    skip_comment(ctx);

    // EOF
    if (ctx->current_pos >= ctx->source_size) {
        return false;
    }

    size_t i = 0;
    while (ctx->current_pos < ctx->source_size && i < max_len - 1) {
        char ch = ctx->source_code[ctx->current_pos];
        if (isspace(ch) || ch == '#')
            break;

        token_out[i++] = ch;
        ctx->current_pos++;
    }
    token_out[i] = '\0';
    return true;
}

void get_arg_string(AsmContext* ctx, char* args_out, size_t max_len) {
    size_t i = 0;

    // Пропускаем пробелы перед аргументом
    while (ctx->current_pos < ctx->source_size) {
        char ch = ctx->source_code[ctx->current_pos];
        if (ch == '\n' || (!isspace(ch)))
            break;
        ctx->current_pos++;
    }

    while (ctx->current_pos < ctx->source_size && i < max_len - 1) {
        char ch = ctx->source_code[ctx->current_pos];
        if (ch == '\n' || ch == '#' || isspace(ch))
            break;

        args_out[i++] = ch;
        ctx->current_pos++;
    }
    args_out[i] = '\0';
}

off_t get_file_size(FILE* file) {
    assert(file);
    int fd = fileno(file);
    assert(fd != ERROR_VALUE && "Error file descriptor");

    struct stat fileInfo = { };
    assert(fstat(fd, &fileInfo) != ERROR_VALUE && "Error fstat");

    return fileInfo.st_size;
}

command_code get_opCode(char* temp, size_t commandsCnt) {
    for (size_t i = 0; i < commandsCnt; i++) {
        if (strcmp(temp, commands[i].name) == 0) {
            return commands[i].opCode;
        }
    }

    return NOT_A_CODE;
}

elem_t get_reg_arg(size_t argCnt, char* argText) {
    for (size_t i = 0; i < argCnt; i++) {
        if (strcmp(argText, argView[i].name) == 0) {
            return (elem_t)argView[i].argCode;
        }
    }

    return 0;
}

elem_t get_label_code(char* str, AsmContext* ctx) {
    for (size_t i = 0; i < ctx->currentNumber; i++) {
        if (strcmp(str, ctx->labelTable[i].name) == 0) {
            return ctx->labelTable[i].ip;
        }
    }

    return -1;
}

void parse_arguments(AsmContext* ctx, Instruction* cmd, size_t argCnt, const char* arg_str) {
    char reg_name[BUFFER_SIZE] = { };
    elem_t arg_dec = 0;

    if (arg_str[0] == '\0') {
        cmd->opType = NO_ARG;
        return;
    }

    int readCnt = sscanf(arg_str, " [ %[A-Za-z] + %d ]", reg_name, &arg_dec);
    if (readCnt == 2) {
        cmd->argValue1 = arg_dec;
        cmd->argValue2 = get_reg_arg(argCnt, reg_name);
        cmd->opType = RAM_REG_DEC_ARG;

    } else if (readCnt == 1) {
        cmd->argValue1 = get_reg_arg(argCnt, reg_name);
        cmd->opType = RAM_REG_ARG;

    } else if (sscanf(arg_str, " [ %d ]", &arg_dec) == 1) {
        cmd->argValue1 = arg_dec;
        cmd->opType = RAM_DEC_ARG;

    } else if (sscanf(arg_str, " %[A-Za-z]", reg_name) == 1) {
        cmd->argValue1 = get_reg_arg(argCnt, reg_name);
        cmd->opType = REG_ARG;

    } else if (sscanf(arg_str, "%d", &arg_dec) == 1) {
        cmd->argValue1 = arg_dec;
        cmd->opType = DEC_ARG;

    } else if (sscanf(arg_str, " :%s", reg_name) == 1) {
        elem_t ip = get_label_code(reg_name, ctx);

        if (ctx->pass == 2 && ip == -1) {
            fprintf(stderr, "[Error] Line %zu: Undefined label ':%s'\n", ctx->current_line, reg_name);
            ctx->error_count++;
        }

        cmd->argValue1 = ip;
        cmd->opType = DEC_ARG;
    } else {

        fprintf(stderr, "[Error] Line %zu: Unrecognized argument format: '%s'\n", ctx->current_line, arg_str);
        ctx->error_count++;
        cmd->opType = NO_ARG;
    }
}

void fill_byte_code(AsmContext* ctx, const Instruction* cmd) {
    switch (cmd->opType) {
    case NO_ARG:
        ctx->byteCode[ctx->currentByte++] = cmd->opCode;

        break;
    case DEC_ARG:
        ctx->byteCode[ctx->currentByte++] = cmd->opCode | (1 << 5);

        memcpy(ctx->byteCode + ctx->currentByte, &cmd->argValue1, sizeof(elem_t));
        ctx->currentByte += sizeof(elem_t);

        break;
    case REG_ARG:
        ctx->byteCode[ctx->currentByte++] = cmd->opCode | (1 << 6);

        memcpy(ctx->byteCode + ctx->currentByte, &cmd->argValue1, sizeof(elem_t));
        ctx->currentByte += sizeof(elem_t);

        break;
    case RAM_DEC_ARG:
        ctx->byteCode[ctx->currentByte++] = cmd->opCode | (1 << 7) | (1 << 5);

        memcpy(ctx->byteCode + ctx->currentByte, &cmd->argValue1, sizeof(elem_t));
        ctx->currentByte += sizeof(elem_t);

        break;
    case RAM_REG_ARG:
        ctx->byteCode[ctx->currentByte++] = cmd->opCode | (3 << 6);

        memcpy(ctx->byteCode + ctx->currentByte, &cmd->argValue1, sizeof(elem_t));
        ctx->currentByte += sizeof(elem_t);

        break;
    case RAM_REG_DEC_ARG:
        ctx->byteCode[ctx->currentByte++] = cmd->opCode | (7 << 5);

        memcpy(ctx->byteCode + ctx->currentByte, &cmd->argValue1, sizeof(elem_t));
        ctx->currentByte += sizeof(elem_t);

        memcpy(ctx->byteCode + ctx->currentByte, &cmd->argValue2, sizeof(elem_t));
        ctx->currentByte += sizeof(elem_t);

        break;
    }
}

bool is_jump_command(command_code opCode) {
    return opCode == JA_CODE || opCode == JAE_CODE || opCode == JB_CODE || opCode == JBE_CODE || opCode == JE_CODE || opCode == JNE_CODE || opCode == JMP_CODE;
}

void validate_instruction(AsmContext* ctx, const Instruction* cmd, const char* cmd_name) {
    // no arg
    if (cmd->opCode == HLT_CODE || cmd->opCode == ADD_CODE || cmd->opCode == RET_CODE || cmd->opCode == OUT_CODE || cmd->opCode == SUB_CODE || cmd->opCode == MUL_CODE || cmd->opCode == DIV_CODE || cmd->opCode == SQRT_CODE || cmd->opCode == LABEL_CODE || cmd->opCode == RET_CODE || cmd->opCode == DISC_CODE || cmd->opCode == PRAM_CODE) {
        if (cmd->opType != NO_ARG) {
            fprintf(stderr, "[Error] Line %zu: Command '%s' shouldn't take any arguments\n", ctx->current_line, cmd_name);
            ctx->error_count++;
        }
    }

    // must have one arg
    if (cmd->opCode == PUSH_CODE || cmd->opCode == POP_CODE || is_jump_command(cmd->opCode)) {
        if (cmd->opType == NO_ARG) {
            fprintf(stderr, "[Error] Line %zu: Command '%s' expects an argument\n", ctx->current_line, cmd_name);
            ctx->error_count++;
        }
    }

    if ((cmd->opCode == POP_CODE || cmd->opCode == IN_CODE) && cmd->opType == DEC_ARG) {
        fprintf(stderr, "[Error] Line %zu: Cannot '%s' into a constant value\n", ctx->current_line, cmd_name);
        ctx->error_count++;
    }
}

void assembly(AsmContext* ctx) {
    assert(ctx && ctx->source_code && ctx->byteCode);

    char temp[BUFFER_SIZE] = { };
    char arg_str[BUFFER_SIZE] = { };

    size_t commandsCnt = sizeof(commands) / sizeof(processorOP);
    size_t argCnt = sizeof(argView) / sizeof(argOp);

    while (get_next_token(ctx, temp, BUFFER_SIZE)) {
        Instruction cmd = { .opCode = NOT_A_CODE, .opType = NO_ARG, .argValue1 = 0, .argValue2 = 0 };
        cmd.opCode = get_opCode(temp, commandsCnt);

        if (temp[0] == ':') {
            cmd.opCode = LABEL_CODE;
            if (ctx->pass == 1) {
                if (get_label_code(&temp[1], ctx) != -1) {
                    fprintf(stderr, "[Error] Line %zu: Duplicate label '%s'\n", ctx->current_line, temp);
                    ctx->error_count++;
                } else {
                    ctx->labelTable[ctx->currentNumber].name = strdup(&temp[1]);
                    ctx->labelTable[ctx->currentNumber].ip = ctx->currentByte;
                    ctx->currentNumber++;
                }
            }
            continue;
        }

        if (cmd.opCode == NOT_A_CODE) {
            fprintf(stderr, "[Error] Line %zu: Unknown instruction '%s'\n", ctx->current_line, temp);
            ctx->error_count++;

            get_arg_string(ctx, arg_str, BUFFER_SIZE);
            continue;
        }

        get_arg_string(ctx, arg_str, BUFFER_SIZE);

        parse_arguments(ctx, &cmd, argCnt, arg_str);

        if (ctx->pass == 1) {
            validate_instruction(ctx, &cmd, temp);
        }

        if (ctx->error_count == 0) {
            fill_byte_code(ctx, &cmd);
        }
    }
}

void write_to_file(uint8_t* byteCode, size_t currentByte) {
    assert(byteCode);

    SPU_Header header = { };
    header.signature = SPU_SIGNATURE;
    header.version = SPU_VERSION;
    header.code_size = currentByte;

    int byteCodeFile = open("byte_code.bin", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(byteCodeFile != ERROR_VALUE && "Error opening");

    write(byteCodeFile, &header, sizeof(SPU_Header));
    write(byteCodeFile, byteCode, currentByte);

    close(byteCodeFile);
}
