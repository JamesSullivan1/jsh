/*
  Grammar definitions for the jsh parser
   
   start       -> jobSetIn
               -> jobSetIn BACKGROUND
               -> 
   jobSetIn    -> jobSetOut REDIRECT_IN FILENAME
               -> jobSetOut
   jobSetOut   -> commandList REDIRECT_OUT FILENAME
               -> commandList
   commandList -> command PIPE commandList
               -> command
   command     -> FILENAME argumentList
               -> FILENAME
   argumentList-> argument argumentList
               -> argument
   argument    -> ARGUMENT
               -> FILENAME
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
start ::= jobSetIn .
{
}
start ::= jobSetIn BACKGROUND .
{
}

jobSetIn ::= jobSetOut REDIRECT_IN FILENAME .
{
}
jobSetIn ::= jobSetOut .
{
}

jobSetOut ::= commandList REDIRECT_OUT FILENAME .
{
}          
jobSetOut ::= commandList .
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
