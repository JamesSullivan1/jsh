%{ 
    #include "parser.h"
%}

%option reentrant
%option noyywrap

%x SINGLE_QUOTED
%x DOUBLE_QUOTED

%%

"|"     {return PIPE;}

"&"     {return BACKGROUND;}

"<"     {return REDIRECT_IN;}

">"     {return REDIRECT_OUT;}

[\t\n\r ]      {} 

[a-zA-Z0-9\.\-_]+       {return FILENAME;}

[^ \t\r\n|'"($\()\)]+   {return ARGUMENT;}

%%
