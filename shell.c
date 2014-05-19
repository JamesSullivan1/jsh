#include <sys/types.h>
#include <stdlib.h>
#include <wait.h>
#include <error.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <termios.h>
#include "job.h"

/* Shell attributes */
pid_t shell_pgid;
struct termios shell_tmodes;
int shell_terminal;
int shell_is_interactive;
job *first_job = NULL;

/* Frees the job j if it is in the job list */
void free_job(job *j)
{
    /* Terminate all processes */
    if(kill(- j->pgid, SIGTERM) > 0) {
        perror("kill (SIGTERM)");
    }

    free(j);
}

void free_process(process *p)
{

    // Kill the process
    if(kill(- p->pid, SIGTERM) > 0) {
        perror("kill (SIGTERM)");
    }

    free(p);
}
/* Find active job with the given pgid */
job *find_job(pid_t pgid)
{
    job *j;
    // Continues until j->next is null
    for(j = first_job; j; j = j->next) {
        if(j->pgid == pgid) return j;
    }
    return NULL;
}

/* Returns 1 iff all processes in the given job are stopped */
int job_is_stopped(job *j)
{
    process *p;
    //  Continues untill p->next is null
    for(p = j->first_process; p; p = p->next) {
        if(!p->completed && !p->stopped) return 0;
    }
    return 1;
}

/* Returns 1 iff all processes in the given job are completed */
int job_is_completed(job *j)
{
    process *p;
    // Continues until p->next is null
    for(p = j->first_process; p; p=p->next) {
        if(!p->completed) return 0;
    }
    return 1;
}

/* Send a signal to a job */
void job_send_signal(job *j, int signal)
{
    if(kill(- j->pgid, signal) > 0) {
        fprintf(stderr, "kill (%d)", signal);
    }
}

/* Sets the status of the process pid returned by waitpid.
 *  Return 0 on success.
 */
int mark_process_status(pid_t pid, int status)
{
    job *j;
    process *p;

    if(pid > 0) {
        /* Update record for the process */
        for(j = first_job; j; j = j->next) {
            for(p = j->first_process; p; p = p->next) {
                if(p->pid == pid) {
                    p->status = status;
                    if(WIFSTOPPED(status)) p->stopped = 1;
                    else {
                        p->completed = 1;
                        if(WIFSIGNALED(status)) {
                            fprintf(stderr, "%d: Terminated by signal %d\n",
                                        (int)pid, WTERMSIG(p->status));
                        }
                    }
                    return 0;
                }
            }
	    }
        fprintf(stderr, "No child process %d.\n",pid);
        return -1;
    } else if (pid == 0 || errno == ECHILD) {
        /* No processes ready to report */
        return -1;
    } else {
        perror("Unexpected error");
        return -1;
    }
}
/* Check for processes that have status info available. */
void update_status()
{
    int status;
    pid_t pid;

    do pid = waitpid(WAIT_ANY, &status, WUNTRACED|WNOHANG);
    while(!mark_process_status(pid, status));
}

/* Check for processes that have status info available,
 *  blocking until all processes in the given job have reported.
 */
void wait_for_job(job *j)
{
    int status;
    pid_t pid;

    do pid = waitpid(WAIT_ANY, &status, WUNTRACED);
    while(!mark_process_status(pid,status) 
            && !job_is_stopped(j)
            && !job_is_completed(j));
}

/* Format information about the job for display to user */
void format_job_info(job *j, const char *status)
{
    fprintf(stderr, "%ld (%s): %s\n", (long)j->pgid, status, j->command);
}

/* Puts the job j in the foreground.
 *  If cont != 0, restore the terminal mode and send the process group a 
 *  SIGCONT signal to wake it up before we block.
 */
void put_job_in_foreground(job *j, int cont)
{
    /* Put job in foreground */
    tcsetpgrp(shell_terminal, j->pgid);

    /* Send the job a continue signal if needed */
    if(cont) {
        tcsetattr(shell_terminal, TCSADRAIN, &j->tmodes);
        if(kill(- j->pgid, SIGCONT) > 0) {
            perror("kill (SIGCONT)");
        }
    }

    /* Wait for the job to report */
    wait_for_job(j);

    /* Put the shell into the foreground */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* Restore the shell terminal modes */
    tcgetattr(shell_terminal, &j->tmodes);
    tcsetattr(shell_terminal, TCSADRAIN, &shell_tmodes);
}

/* Puts the job j in the background.
 *  If cont != 0, send the process group a SIGCONT to wake it up.
 */
void put_job_in_background(job *j, int cont)
{
    /* Send the SIGCONT signal if needed */
    if(cont) {
        if(kill(- j->pgid, SIGCONT) > 0) {
            perror("kill (SIGCONT)");
        }
    }
}


/* Notify user about stopped or terminated jobs.
 *  Delete terminated jobs from active joblist.
 */
void do_job_notification()
{
    job *j, *jlast, *jnext;
    process *p;

    /* Update status info for child processes */
    update_status();

    jlast = NULL;
    for(j = first_job; j; j = jnext) {
        jnext = j->next;
        /* If all process are complete, inform the user the job is complete
         *  and remove it from the active list 
         */
        if(job_is_completed(j)) {
            format_job_info(j, "completed");
            if(jlast) jlast->next = jnext;
            else first_job = jnext;
            free_job(j);
        }
        /* Notify the user about stopped jobs and mark them so we don't repeat this */
        else if(job_is_stopped(j) && !j->notified) {
            j->notified = 1;
            format_job_info(j, "stopped");
            jlast = j;
        } else {
            /* Job is running so don't report */
            jlast = j;
        }

    }
}

/* Notify user about all running jobs. */
void list_active_jobs()
{
    job *j, *jlast, *jnext;
    process *p;

    /* Update status info for child processes */
    update_status();

    jlast = NULL;
    for(j = first_job; j; j = jnext) {
        jnext = j->next;
        if(!job_is_completed(j) && !job_is_stopped(j)) {
            format_job_info(j, "active");
            jlast = j;
        }
    }
}

/* Mark a stopped job as being running again. Does not actually resume exec */
void mark_job_as_running(job *j)
{
    process *p;
    for(p = j->first_process; p; p = p->next) p->stopped = 0;
    j->notified = 0;
}

/* Continue the job with a SIGCONT*/
void continue_job(job *j, int foreground)
{
    mark_job_as_running(j);
    if(foreground) put_job_in_foreground(j, 1);
    else put_job_in_background(j, 1);
}

void init_shell()
{
    /* Ensure that shell is interactive */
    shell_terminal = STDIN_FILENO;
    shell_is_interactive = isatty(shell_terminal);

    if(shell_is_interactive) {
        /* Wait until shell is in foreground */
        while(tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp())) {
            kill(- shell_pgid, SIGTTIN);
        }

        /* Ignore interactive and job-control signals 
         *  Note that by inheritance all child processes will have this property
         *  so should have signal handling explicitly set after forking
         */
        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGCHLD, SIG_IGN);

        /* Put shell into its own proc group */
        shell_pgid = getpid();
        if(setpgid(shell_pgid, shell_pgid) < 0) {
            perror("Failed to put shell into its own process group.");
            exit(1);
        }

        /* Take terminal control */
        tcsetpgrp(shell_terminal, shell_pgid);

        /* Save the default terminal attributes */
        tcgetattr(shell_terminal, &shell_tmodes);
    }
}

/* Launch a process */
void launch_process(process *p, pid_t pgid, int infile, 
                    int outfile, int errfile, int foreground)
{
    pid_t pid;
    /* Don't do anything with pids and signals if noninteractive */
    if(shell_is_interactive) {
        /* Put process into the process group and give the process group the terminal
         *  if possible.
         * Due to race condition possibility both the shell and its child processes must do this
         */
        pid = getpid();
        if(pgid == 0) pgid = pid;
        setpgid(pid, pgid);
        if(foreground) tcsetpgrp(shell_terminal, pgid);

        /* Reset handling for signals to default */
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

    }

    /* Set the IO channels for the process */
    if(infile != STDIN_FILENO) {
        dup2(infile, STDIN_FILENO);
        close(infile);
    }
    if(outfile != STDOUT_FILENO) {
        dup2(outfile, STDOUT_FILENO);
        close(outfile);
    }
    if(errfile != STDERR_FILENO) {
        dup2(errfile, STDERR_FILENO);
        close(errfile);
    }

    /* Execute and exit */
    execvp(p->argv[0], p->argv);
    perror("execvp");
    exit(1);
}

void launch_job(job *j, int foreground)
{
    process *p;
    pid_t pid;
    int mypipe[2], infile, outfile;

    infile = j->stdin;
    for(p = j->first_process; p; p = p->next) {
        /* If needed, set up pipes */
        if(p->next) {
            if(pipe(mypipe) < 0) {
                perror("pipe");
                exit(1);
            }
            outfile = mypipe[1];
        } else outfile = j->stdout;

        /* Fork the child process */
        pid = fork();
        if(pid == 0) { 
            /* Child process */
            launch_process(p, j->pgid, infile, outfile, j->stderr, foreground);
        } else if(pid < 0) {
            /* Fork failed */
            perror("fork");
            exit(1);
        } else {
            /* Parent process */
            p->pid = pid;
            if(shell_is_interactive) {
                if(!j->pgid) j->pgid = pid;
                setpgid(pid, j->pgid);
            }
        }

        /* Cleanup after pipes */
        if(infile != j->stdin) close(infile);
        if(outfile != j->stdout) close(outfile);
        infile = mypipe[0];
    }

    format_job_info(j, "launched");

    if(!shell_is_interactive) wait_for_job(j);
    
    if(foreground) put_job_in_foreground(j, 0);
    else put_job_in_background(j, 0);
}
