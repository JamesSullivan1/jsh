#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include "job.h"

job *new_job()
{
    job *j = malloc(sizeof(job));
    j->next = NULL;
    j->command = NULL;
    j->first_process = NULL;
    j->pgid = 0;
    j->notified = 0;
    j->stdin = STDIN_FILENO;
    j->stdout = STDOUT_FILENO;
    j->stderr = STDERR_FILENO;
    return j;
}

process *new_process()
{
    process *p = malloc(sizeof(process));
    p->next = NULL;
    p->pid = 0;
    p->completed = 0;
    p->stopped = 0;
    p->status = 0;
    return p;
}

void print_process(process *p)
{
    printf("Process (%d):\nargc: %d\n", p->pid, p->argc);
    int i = 0;
    for(i = 0; i < p->argc; i++) {
        printf("argv[%d]: %s\n", i, p->argv[i]);
    }
}
