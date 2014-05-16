/*
  Grammar definitions for the jsh parser
   
   start       -> commandList 
   commandList -> command PIPE commandList
               -> command BACKGROUND PIPE commandList
               -> command 
               -> command BACKGROUND
   command     -> FILENAME argumentList
               -> FILENAME
               -> COMMAND_SUBSTITUTION_START commandList 
                    COMMAND_SUBSTITUTION_END
   argumentList-> argument argumentList
               -> argument
   argument    -> ARGUMENT
               -> FILENAME
               -> COMMAND_SUBSTITUTION_START commandList
                    COMMAND_SUBSTITUTION_END
*/

%include
{
#include <assert.h>
#include <stdio.h>
}

%token_type {const char*}

%extra_argument {int *valid}

%syntax_error
{
printf("Syntax Error\n");
*valid = 0;
}

start ::= commandList .
commandList ::= command_start PIPE commandList .
{
}
commandList ::= command_start .
{
}
commandList ::= .
{
}

command_start ::= command .
{
}
command_start ::= command BACKGROUND .
{
}

command ::= FILENAME argumentList .
{
}
command ::= FILENAME .
{
}
command ::= COMMAND_SUBSTITUTION_START commandList COMMAND_SUBSTITUTION_END . 
{
}

argumentList ::= argument argumentList .
{
}
argumentList ::= argument .
{
}

argument ::= ARGUMENT .
{
}
argument ::= FILENAME .
{
}
argument ::= COMMAND_SUBSTITUTION_START commandList COMMAND_SUBSTITUTION_END .
{
}
