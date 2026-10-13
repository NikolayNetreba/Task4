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
#include "assembly.h"

#define DEBUG

int main(){
    // open file to read
    FILE* input = fopen("input.txt", "r");
    assert(input && "Error opening file");

    off_t inputSize = get_file_size(input);

    uint8_t* byteCode = (uint8_t*) calloc(inputSize, sizeof(uint16_t)); //TODO можно написать свой вектор
    size_t currentByte = 0;

    label* labelTable = (label*) calloc(inputSize / 2, sizeof(label));
    size_t currentNumber = 0;

    // filling labelTable
    assembly(input, byteCode, labelTable, &currentByte, &currentNumber, 1);

    currentByte = 0;
    rewind(input);

    //finaly generating bytecode
    assembly(input, byteCode, labelTable, &currentByte, &currentNumber, 2);

    //open file to write
    write_to_file(byteCode, currentByte);

    fclose(input);
    free(byteCode);
    free(labelTable);

    return 0;
}

bool check_line(FILE* input){
    assert(input);

    int ch = 0;
    while((ch = fgetc(input)) != EOF && ch != '\n'){
        // printf("%c : ", ch);
        if(!isspace(ch)){
            return true;
        }
    }

    return false;
}

void get_text_arg(elem_t* argValue, size_t argCnt, FILE* input){
    assert(argValue);
    assert(input);

    char argText[5] = {};
    if(fscanf(input, "%4s", argText) == 1){
        for(size_t i = 0; i < argCnt; i++){
            if(strcmp(argText, arg[i].name) == 0){
                *argValue = arg[i].argCode;
            }
        }

        if(*argValue == 0){
            assert(0 && "unrecognized argument"); //TODO
        }
    }
}

off_t get_file_size(FILE* file){
    assert(file);

    int fd = fileno(file);
    assert(fd != ERROR_VALUE && "Error file descriptor");

    struct stat fileInfo = {};
    assert(fstat(fd, &fileInfo) != ERROR_VALUE && "Error fstat");

    return fileInfo.st_size;
}

command_code get_opCode(char* temp, size_t commandsCnt){
    for(size_t i = 0; i < commandsCnt; i++){
        if(strcmp(temp, commands[i].name) == 0){
            return commands[i].opCode;
        }
    }

    return NOT_A_CODE;
}

void assembly(FILE* input, uint8_t* byteCode, label* labelTable, size_t* currentByte, size_t* currentNumber, const int number){
    assert(input);
    assert(byteCode);
    assert(currentByte);

    #ifdef DEBUG
    FILE* byteCodeTxt = fopen("byte_code.txt", "w");
    assert(byteCodeTxt && "Error opening file");
    #endif

    char temp[BUFFER_SIZE] = {};
    size_t commandsCnt = sizeof(commands) / sizeof(processorOP);
    size_t argCnt = sizeof(arg) / sizeof(processorOP);

    while(fscanf(input, "%" SPEC_BUFFER_SIZE "s", temp) != EOF){
        command_code opCode = get_opCode(temp, commandsCnt);
        command_type opType = NO_ARG;
        elem_t argValue = 0;

        if(temp[0] == ':'){
            opCode = LABEL_CODE;

            if(number == 1){
                labelTable[*currentNumber].name = strdup(temp);
                labelTable[(*currentNumber)++].ip = *currentByte;
            }
        }

        if(opCode == PUSH_CODE){
            if(fscanf(input, "%d", &argValue) == 0){
                get_text_arg(&argValue, argCnt, input); // if argument is text
                opType = PUSH_REG_ARG;
            } else {
                opType = PUSH_DEC_ARG;
            }
        } else if(opCode == POP_CODE){
            char nextCh = fgetc(input);
            while(nextCh == ' ' || nextCh == '\t'){
                nextCh = fgetc(input);
            }

            if(nextCh != EOF){
                ungetc(nextCh, input);
            }
            if(nextCh != '\n' && nextCh != EOF){ // if it is arg
                get_text_arg(&argValue, argCnt, input);
                opType = POP_REG_ARG;
            } else {
                opType = NO_ARG;
            }

        } else if(opCode == JA_CODE || opCode == JAE_CODE ||
                  opCode == JB_CODE || opCode == JBE_CODE ||
                  opCode == JE_CODE || opCode == JNE_CODE || opCode == JMP_CODE) {
            opType = J_POINT_ARG;

            char argLabel[BUFFER_SIZE] = {};
            if(fscanf(input, "%" SPEC_BUFFER_SIZE "s", argLabel) == 1){
                for(int i = 0; i < *currentNumber; i++){
                    if(strcmp(argLabel, labelTable[i].name) == 0){
                        argValue = labelTable[i].ip;
                        break;
                    }
                }
            }
        }

        if(check_line(input)){
            fprintf(stderr, "%s\n", temp);
            assert(0 && "input error");
        }

        if(opCode == NOT_A_CODE){
            fprintf(stderr, "Syntax Error: unknown command: %s", temp);
            exit(EXIT_FAILURE);
        }

        if(opCode != LABEL_CODE){
            byteCode[(*currentByte)++] = opCode;
            #ifdef DEBUG
            fprintf(byteCodeTxt, "\n%d ", opCode);
            #endif
        }


        if(opCode == PUSH_CODE || opCode == POP_CODE || opType == J_POINT_ARG){
            byteCode[(*currentByte)++] = opType;

            if(opType != NO_ARG){
                memcpy(&byteCode[*currentByte], &argValue, sizeof(elem_t));
                *currentByte += sizeof(elem_t);
            }

            #ifdef DEBUG
            fprintf(byteCodeTxt, "%d", argValue);
            #endif
        }
    }

    #ifdef DEBUG
    fclose(byteCodeTxt);
    #endif
}

void write_to_file(uint8_t* byteCode, size_t currentByte){
    assert(byteCode);

    int byteCodeFile = open("byte_code.bin", O_WRONLY | O_CREAT | O_TRUNC | O_BINARY);
    assert(byteCodeFile != ERROR_VALUE && "Error opening");

    write(byteCodeFile, byteCode, currentByte);

    close(byteCodeFile);
}

