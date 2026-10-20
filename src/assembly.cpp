#include "assembly.h"

#define DEBUG
int main() {
    FILE* input = fopen("input.txt", "r");
    assert(input && "Error opening file");

    off_t inputSize = get_file_size(input);

    AsmContext ctx = {
        .input = input,
        .byteCode = (uint8_t*)calloc(inputSize, sizeof(uint16_t)),
        .currentByte = 0,
        .labelTable = (label*)calloc(inputSize / 2, sizeof(label)),
        .currentNumber = 0,
        .pass = 1
    };

    // Первый проход (сбор меток)
    assembly(&ctx);

    ctx.currentByte = 0;
    rewind(ctx.input);
    ctx.pass = 2;

    // Второй проход (генерация байткода)
    assembly(&ctx);

    // Запись результата в файл
    write_to_file(ctx.byteCode, ctx.currentByte);

    fclose(ctx.input);
    free(ctx.byteCode);
    free(ctx.labelTable);

    return 0;
}

void skip_comment(FILE* input) {
    int ch = 0;
    while ((ch = fgetc(input)) != EOF && ch != '\n') { }
}

bool check_line(FILE* input) {
    assert(input);
    int ch = 0;
    while ((ch = fgetc(input)) != EOF && ch != '\n') {
        if (ch == '#') {
            skip_comment(input);
            return false;
        }

        if (!isspace(ch)) {
            return true;
        }
    }

    return false;
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

void parse_arguments(AsmContext* ctx, Instruction* cmd, size_t argCnt) {
    char arg_str[BUFFER_SIZE] = { };
    char reg_name[BUFFER_SIZE] = { };
    elem_t arg_dec = 0;

    if (fscanf(ctx->input, "%[^\n]", arg_str) != 1) {
        cmd->opType = NO_ARG;

        return;
    }

    int readCnt = sscanf(arg_str, " [ %[A-Za-z] + %d ]", reg_name, &arg_dec);
    if (readCnt == 2) {
        cmd->argValue1 = arg_dec;

        cmd->argValue2 = get_reg_arg(argCnt, reg_name);
        assert(cmd->argValue1 != 0 && "unrecognized argument");
        cmd->opType = RAM_REG_DEC_ARG;

    } else if (readCnt == 1) {
        cmd->argValue1 = get_reg_arg(argCnt, reg_name);
        assert(cmd->argValue1 != 0 && "unrecognized argument");

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
        cmd->argValue1 = get_label_code(reg_name, ctx);
        cmd->opType = DEC_ARG;

    } else {
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

void assembly(AsmContext* ctx) {
    assert(ctx && ctx->input && ctx->byteCode);

#ifdef DEBUG
    FILE* byteCodeTxt = fopen("byte_code.txt", "w");
    assert(byteCodeTxt && "Error opening file");
#endif

    char temp[BUFFER_SIZE] = { };
    size_t commandsCnt = sizeof(commands) / sizeof(processorOP);
    size_t argCnt = sizeof(argView) / sizeof(argOp);

    while (fscanf(ctx->input, "%" SPEC_BUFFER_SIZE "s", temp) != EOF) {
        if (temp[0] == '#') {
            skip_comment(ctx->input);
            continue;
        }

        Instruction cmd = { .opCode = NOT_A_CODE, .opType = NO_ARG, .argValue1 = 0, .argValue2 = 0 };
        cmd.opCode = get_opCode(temp, commandsCnt);

        if (temp[0] == ':') {
            cmd.opCode = LABEL_CODE;
            if (ctx->pass == 1) {
                ctx->labelTable[ctx->currentNumber].name = strdup(&temp[1]);
                ctx->labelTable[ctx->currentNumber].ip = ctx->currentByte;
                ctx->currentNumber++;
            }
            continue;
        }

        parse_arguments(ctx, &cmd, argCnt);

        if (check_line(ctx->input)) {
            fprintf(stderr, "Syntax error near: %s\n", temp);
            assert(0 && "input error");
        }

        if (cmd.opCode == NOT_A_CODE) {
            fprintf(stderr, "Syntax Error: unknown command: %s\n", temp);
            exit(EXIT_FAILURE);
        }

#ifdef DEBUG
        fprintf(byteCodeTxt, "\n%d ", cmd.opCode);
#endif

        fill_byte_code(ctx, &cmd);
    }

#ifdef DEBUG
    fclose(byteCodeTxt);
#endif
}

void write_to_file(uint8_t* byteCode, size_t currentByte) {
    assert(byteCode);

    int byteCodeFile = open("byte_code.bin", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    assert(byteCodeFile != ERROR_VALUE && "Error opening");

    write(byteCodeFile, byteCode, currentByte);

    close(byteCodeFile);
}
