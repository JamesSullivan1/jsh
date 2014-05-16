#ifndef _command_h
#define _command_h

typedef struct command command;

struct command {
    int argc;
    char **argv;
    command *pipe_to;
    

};

#endif
