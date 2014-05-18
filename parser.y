/*
  Grammar definitions for the jsh parser
   
   start       -> commandList
               -> commandList BACKGROUND
               -> 
   commandList -> command PIPE commandList
               -> command
   command     -> FILENAME argumentList
               -> FILENAME
   argumentList-> argument argumentList
               -> argument
   argument    -> ARGUMENT
               -> FILENAME
               -> REDIRECT_IN FILENAME
               -> REDIRECT_OUT FILENAME
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

start ::= .
{
}
start ::= commandList .
{
}
start ::= commandList BACKGROUND .
{
}

commandList ::= command PIPE commandList .
{
}
commandList ::= command .
{
}

command ::= FILENAME argumentList .
{
}
command ::= FILENAME .
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
argument ::= REDIRECT_IN FILENAME .
{
}
argument ::= REDIRECT_OUT FILENAME .
{
}
