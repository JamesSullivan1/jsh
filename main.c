#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "dbg.h"
#include "parser.h"
#include "scanner.yy.h"

#define MAX_SIZE 1 << 16

void *ParseAlloc(void* (*allocProc)(size_t));
void Parse(void* parser, int token, const char* in, int* val);
void ParseFree(void* parser, void(*freeProc)(void*));

char *getcwd(char *buf, size_t size);

int debug = 1;

void print_lexCode(int debug, int lexCode)
{
if(debug == 0) return;
    switch(lexCode) {
        case 1: 
            printf("PIPE\n");
            break;
        case 2: 
            printf("BACKGROUND\n");
            break;
        case 3: 
            printf("FILENAME\n");
            break;
        case 4: 
            printf("CMD_SUB_START\n");
            break;
        case 5: 
            printf("CMD_SUB_END\n");
            break;
        case 6: 
            printf("ARGUMENT\n");
            break;
    }
}

void print_prompt()
{
    char cwd[1024];
    if( getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s$ ", cwd);
    }
}

int parse(char *cmd_line)
{
    // Set up the lexer
    yyscan_t lexer;
    yylex_init(&lexer);
    YY_BUFFER_STATE bufferState = yy_scan_string(cmd_line, lexer);

    // Set up the parser
    void *parser = ParseAlloc(malloc);

    int validParse;
    int lexCode;
    do {
        lexCode = yylex(lexer);
        print_lexCode(debug, lexCode);
        validParse = 1;
        Parse(parser, lexCode, NULL, &validParse);
    } while(lexCode > 0 && validParse != 0);


    if(lexCode == -1 || validParse == 0) {
        return -1;
    }
    
    yy_delete_buffer(bufferState, lexer);
    yylex_destroy(lexer);
    ParseFree(parser, free);
    return 0;
}

int main(int argc, char *argv[])
{

    size_t nbytes = MAX_SIZE;
    char *commandLine;
    print_prompt();
    while( getline(&commandLine, &nbytes, stdin)) {
        parse(commandLine);
        print_prompt();
    }

    return 0;
}
