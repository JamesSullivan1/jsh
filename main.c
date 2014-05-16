#include <stdio.h>
#include <stdlib.h>
#include "dbg.h"
#include "parser.h"
#include "scanner.yy.h"

#define MAX_SIZE 1 << 16

void *ParseAlloc(void* (*allocProc)(size_t));
void Parse(void* parser, int token, const char* in, int* val);
void ParseFree(void* parser, void(*freeProc)(void*));

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
        printf("%d\n", lexCode);
        validParse = 1;
        Parse(parser, lexCode, NULL, &validParse);
    } while(lexCode > 0 && validParse != 0);

    check(lexCode != -1, "The scanner encountered an error.");
    check(validParse != 0, "The scanner encountered an error.");

    
    yy_delete_buffer(bufferState, lexer);
    yylex_destroy(lexer);
    ParseFree(parser, free);
    return 0;

error:
    yy_delete_buffer(bufferState, lexer);
    yylex_destroy(lexer);
    ParseFree(parser, free);
    return -1;
    
}

int main(int argc, char *argv[])
{
    // First test a simple parser
    int result = 1;
    void* parser = ParseAlloc(malloc);
    Parse(parser, FILENAME, "cat", &result);
    printf("%d\n", result);
    result = 1;
    Parse(parser, PIPE, 0, &result);
    printf("%d\n", result);
    result = 1;
    Parse(parser, FILENAME, "ls", &result);
    printf("%d\n", result);
    result = 1;
    Parse(parser, 0, 0, &result); // Signal end of tokens
    printf("%d\n", result);
    result = 1;
    
    ParseFree(parser, free);

    size_t nbytes = MAX_SIZE;
    char *commandLine;
    while( getline(&commandLine, &nbytes, stdin)) {
        parse(commandLine);
    }

    return 0;
}
