#ifndef _job_h
#define _job_h

#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

/* Process structure - An individual process of computation */
typedef struct process process;
struct process {
    process *next;                  // Next process on the pipe
    pid_t pid;                      // Process ID
    char completed;                 // True if process is complete
    char stopped;                   // True if process is stopped
    int status;                     // Status flags
    int argc;                       // Number of arguments
    char **argv;                    // Arguments for execution
};


/* Job structure - A pipeline of processes */
typedef struct job job;
struct job {
    job *next;                      // Next active job
    char *command;                  // Command Line
    process *first_process;         // Pointer to process list for job
    pid_t pgid;                     // Process group ID
    char notified;                  // True if user told about a stopped job
    struct termios tmodes;          // Terminal modes
    int stdin, stdout, stderr;      // IO channels
};

job *new_job();

process *new_process();

#endif
