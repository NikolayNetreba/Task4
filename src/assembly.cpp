#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>

#include "config.h"

#define ERROR_VALUE -1
#define BUFFER_SIZE 5
#define SPEC_BUFFER_SIZE "4"
#define PUSH_CODE 1
#define PUSH_TEXT_ARG 111
#define POP_CODE 2
#define POP_TEXT_ARG 112

typedef struct{
    const char* name;
    uint8_t opCode;
} processorOP;

//TODO Сделать байт отвечающий за тип данных.
processorOP commands[] = {
    {"PUSH", 1},
    {"POP", 2},
    {"DIV", 3},
    {"MUL", 4},
    {"SUB", 5},
    {"SQRT", 6},
    {"ADD", 7},
    {"HLT", 8},
    {"OUT", 9}
};

processorOP arg[] = {
    {"AX", 200},
    {"BX", 201},
    {"CX", 202},
};

bool check_line(FILE* input){
    int ch = 0;
    while((ch = fgetc(input)) != EOF && ch != '\n'){
        if(!isspace(ch)){
            return true;
        }
    }

    return false;
}

void get_text_arg(elem_t* argValue, size_t argCnt, FILE* input, uint8_t* opCode, uint8_t newCode){
    char argText[5] = {};
    if(fscanf(input, "%4s", argText) == 1){
        for(size_t i = 0; i < argCnt; i++){
            if(strcmp(argText, arg[i].name) == 0){
                *argValue = arg[i].opCode;
            }
        }

        *opCode = newCode;

        if(*argValue == 0){
            assert(0 && "unrecognized argument"); //TODO
        }
    }
}

off_t get_file_size(FILE* file){
    int fd = fileno(file);
    assert(fd != -1 && "Error file descriptor");

    struct stat fileInfo = {};
    assert(fstat(fd, &fileInfo) != -1 && "Error fstat");

    return fileInfo.st_size;
}

void assembly(FILE* input, uint8_t* byteCode, size_t* currentByte){
    FILE* byteCodeTxt = fopen("byte_code.txt", "w");
    assert(byteCodeTxt && "Error opening file");

    char temp[BUFFER_SIZE] = {};
    size_t commansdCnt = sizeof(commands) / sizeof(processorOP);
    size_t argCnt = sizeof(arg) / sizeof(processorOP);

    while(fscanf(input, "%" SPEC_BUFFER_SIZE "s", temp) != EOF){
        uint8_t opCode = 0;
        elem_t argValue = 0;

        for(size_t i = 0; i < commansdCnt; i++){
            if(strcmp(temp, commands[i].name) == 0){
                opCode = commands[i].opCode;
            }
        }
        if(opCode == PUSH_CODE){
            if(fscanf(input, "%d", &argValue) == 0){
                get_text_arg(&argValue, argCnt, input, &opCode, PUSH_TEXT_ARG);
            }
        } else if(opCode == POP_CODE){
            char nextCh = fgetc(input);
            while(nextCh == ' ' || nextCh == '\t'){
                nextCh = fgetc(input);
            }

            if(nextCh != '\n' && nextCh != EOF){
                ungetc(nextCh, input);

                get_text_arg(&argValue, argCnt, input, &opCode, POP_TEXT_ARG);
            }

        }

        if(check_line(input)){
            assert(0 && "input error");
        }

        byteCode[(*currentByte)++] = opCode;
        fprintf(byteCodeTxt, "%d ", opCode);
        if(opCode == PUSH_CODE || opCode == PUSH_TEXT_ARG || opCode == POP_TEXT_ARG){
            memcpy(&byteCode[*currentByte], &argValue, sizeof(elem_t));
            *currentByte += sizeof(elem_t);
            fprintf(byteCodeTxt, "%d", argValue);
        }
        fprintf(byteCodeTxt, "\n");
    }

    fclose(byteCodeTxt);
}

int main(){
    // open file to read
    FILE* input = fopen("input.txt", "r");
    assert(input && "Error opening file");

    uint8_t* byteCode = (uint8_t*) calloc(get_file_size(input), sizeof(uint8_t)); //можно написать свой вектор
    size_t currentByte = 0;

    assembly(input, byteCode, &currentByte);

    //open file to write
    int byteCodeFile = open("byte_code.bin", O_WRONLY | O_CREAT | O_TRUNC | O_BINARY);
    assert(byteCodeFile != -1 && "Error opening");

    write(byteCodeFile, byteCode, currentByte);

    close(byteCodeFile);
    fclose(input);
}
