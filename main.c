#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "dbg.h"
#include "parser.h"
#include "scanner.yy.h"
#include "job.h"

#define MAX_HISTORY 1 << 8
#define MAX_SIZE 1 << 16
#define MAX_ARGUMENTS 1 << 8
#define MAX_COMMANDS 1 << 8

void *ParseAlloc(void* (*allocProc)(size_t));
void Parse(void* parser, int token, const char* in, int* val);
void ParseFree(void* parser, void(*freeProc)(void*));
char *getcwd(char *buf, size_t size);

extern job *first_job;
job *current_job;

int debug = 1;

void print_lexCode(int debug, int lexCode)
{
if(debug == 0) return;
    
    switch(lexCode) {
        case 1: 
            printf("BACKGROUND\n");
            break;
        case 2: 
            printf("PIPE\n");
            break;
        case 3: 
            printf("FILENAME\n");
            break;
        case 4: 
            printf("ARGUMENT\n");
            break;
        case 5: 
            printf("REDIRECT_IN\n");
            break;
        case 6: 
            printf("REDIRECT_OUT\n");
            break;
    }
}

/* Print the prompt with the cwd to the command line */
void print_prompt()
{
    char cwd[1024];
    if( getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s$ ", cwd);
    }
}

/* Safely copies n characters from src to dst, ensuring that dst is null-terminated. */
char *sstrcpy(char *dst, const char *src, size_t n)
{
    char *ret = dst;
    while(n-- > 0) {
        if((*dst++ = *src++) == '\0') return ret;
    }
    *dst++ = '\0';
    return ret;
}

/* Parse the command line.
 *  Returns 0 on success and -1 on a syntax error.
 */
int parse(char *cmd_line)
{
    char *token[MAX_COMMANDS];
    int foreground = 1;

    // Set up the lexer
    yyscan_t lexer;
    yylex_init(&lexer);
    YY_BUFFER_STATE bufferState = yy_scan_string(cmd_line, lexer);

    // Set up the parser
    void *parser = ParseAlloc(malloc);

    int validParse;
    int lexCode;
    int i,j;
    int proc_marker, num_processes;
    int infile_marker, outfile_marker;
    char *proc_argv[MAX_ARGUMENTS][MAX_COMMANDS];
    int proc_argc[MAX_COMMANDS];
    i = 0;
    j = 0;
    infile_marker = -1;
    outfile_marker = -1;
    proc_marker = 0;
    num_processes = 0;
    do {
        lexCode = yylex(lexer);
        validParse = 1;
        Parse(parser, lexCode, NULL, &validParse);

        if(!validParse || lexCode == -1) goto error;

        if(lexCode == BACKGROUND) foreground = 0;
        else if(lexCode == REDIRECT_IN) { 
            infile_marker = i;
        }
        else if(lexCode == REDIRECT_OUT) {
            outfile_marker = i;
        }
        else if(lexCode == 0 || lexCode == PIPE) {
            if(i == 0) break;
            // This is a pipe or endline, so accumulate the tokens up to this point
            //  as the arguments to a single process.
            int pos = 0;
            for(j = proc_marker; j < i; j++) {
                // Clone in each token into the processes' argv
                // As long as the token is not associated with a file redirect
                if(j != infile_marker && j != outfile_marker) {
                    int len = strlen(token[j]);
                    proc_argv[num_processes][pos] = malloc(sizeof(char) * (len + 1));
                    sstrcpy(proc_argv[num_processes][pos++], token[j], len); 
                }
            }
            proc_argc[num_processes] = i - proc_marker;
            if(infile_marker > 0) proc_argc[num_processes]--;
            if(outfile_marker > 0) proc_argc[num_processes]--;
            // Increment the process count
            num_processes++;
            // The next process' arguments will start at token i
            proc_marker = i;
        // Any other tokens that are not already designated as IO 
        } else {
            // Clone the token
            int len = strlen(yyget_text(lexer));
            token[i] = calloc(sizeof(char) * (len + 1), sizeof(char));
            sstrcpy(token[i++], yyget_text(lexer), len);
        }

    } while(lexCode > 0);

    char *infile_name;
    char *outfile_name;

    if(infile_marker > 0) {
        int len = strlen(token[infile_marker]);
        infile_name = (char*)malloc(sizeof(char) * (len+1));
        sstrcpy(infile_name, token[infile_marker], len);
    }

    if(outfile_marker > 0) {
        int len = strlen(token[outfile_marker]);
        outfile_name = (char*)malloc(sizeof(char) * (len+1));
        sstrcpy(outfile_name, token[outfile_marker], len);
    }

    if(num_processes == 0) goto error;

    int k,l;
    // Wipe the token buffer
    for(k = 0; k < i; k++) {
        l = 0;
        while(token[k][l] != '\0') token[k][l++] = '\0';
        if(token[k]) free(token[k]);
    }

    if(!current_job) current_job = new_job();

    // Initialize a job with the processes
    if(current_job->first_process == NULL) current_job->first_process = new_process();
    process *p = current_job->first_process;

    // Initialize all processes
    for(k = 0; k < num_processes; k++) {
        p->argc = proc_argc[k];
        p->argv = (char**)malloc(sizeof(char *) * MAX_ARGUMENTS);
        for(j = 0; j < proc_argc[k]; j++) {
            // Clone in each token
            int len = strlen(proc_argv[k][j]);
            p->argv[j] = calloc(sizeof(char) * (len + 1), sizeof(char));
            sstrcpy(p->argv[j], proc_argv[k][j], len);
            // Deallocate the memory associated with the old buffer
            while(proc_argv[k][j][l] != '\0') proc_argv[k][j][l++] = '\0';
            if(proc_argv[k][j]) free(proc_argv[k][j]);
        }

        if(k < num_processes - 1) {
            p->next = new_process();
            p = p->next;
        }
    }

    // Set IO as needed
    if(infile_marker > 0) { 
        current_job->stdin = open(infile_name, O_RDONLY);
        free(infile_name);
        infile_marker = -1;
    }
    if(outfile_marker > 0) {
        current_job->stdout = open(outfile_name, O_WRONLY | O_CREAT, 
                        S_IWUSR | S_IRUSR | S_IRGRP | S_IWGRP);
        printf("Got fd %d for file %s\n", current_job->stdout, outfile_name);
        free(outfile_name);
        outfile_marker = -1;
    }

    current_job->command = current_job->first_process->argv[0];
    launch_job(current_job, foreground);

    current_job = current_job->next;

    yy_delete_buffer(bufferState, lexer);
    yylex_destroy(lexer);
    ParseFree(parser, free);
    return 0;

error:
    // Wipe the token buffer
    for(k = 0; k < i; k++) {
        l = 0;
        while(token[k][l] != '\0') token[k][l++] = '\0';
        if(token[k]) free(token[k]);
    }
    // Wipe each argument buffer
    for(k = 0; k < num_processes; k++) {
        int r;
        for(r = 0; r < proc_argc[k]; r++) {
            l = 0;
            while(proc_argv[k][r][l] != '\0') proc_argv[k][r][l++] = '\0';
            if(proc_argv[k][r]) free(proc_argv[k][r]);
        } 
    }

    yy_delete_buffer(bufferState, lexer);
    yylex_destroy(lexer);
    ParseFree(parser, free);
    return -1;
}

int no_exit = 1;

void int_handler(int dummy) {
    no_exit = 0;
}

int main(int argc, char *argv[])
{
    init_shell();
    first_job = new_job();
    current_job = first_job;
    size_t nbytes = MAX_SIZE;
    char *commandLine;


    signal(SIGINT, int_handler);

    print_prompt();
    while( no_exit == 1 && getline(&commandLine, &nbytes, stdin)) {
        parse(commandLine);        
        print_prompt();
    }

    return 0;
}
