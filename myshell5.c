#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "PUCITshell:- "
#define HISTORY_SIZE 10

typedef struct {
    pid_t pid;
    char command[MAX_LEN];
} Job;

int execute(char* arglist[], int background);
char** tokenize(char* cmdline);
char* read_cmd(char*, FILE*);
void handle_sigchld(int sig);
void add_to_history(const char* cmd);
char* get_history_command(int index);
void print_history();
void add_job(pid_t pid, const char* cmd);
void remove_job(pid_t pid);
void print_jobs();
void kill_job(int job_num);
void handle_builtin_commands(char** arglist, int* is_builtin, int* background);

Job jobs[HISTORY_SIZE];
int job_count = 0;
char history[HISTORY_SIZE][MAX_LEN];
int history_count = 0;

int main() {
    char *cmdline;
    char** arglist;
    char* prompt = PROMPT;
    
    signal(SIGCHLD, handle_sigchld);

    while ((cmdline = read_cmd(prompt, stdin)) != NULL) {
        int background = 0;
        int is_builtin = 0;

        // Handle command from history if it starts with '!'
        if (cmdline[0] == '!') {
            int index;
            if (cmdline[1] == '-') {
                index = history_count - 1;
            } else {
                index = atoi(cmdline + 1) - 1;
            }
            
            char* history_cmd = get_history_command(index);
            if (history_cmd) {
                printf("Repeating command: %s\n", history_cmd);
                strncpy(cmdline, history_cmd, MAX_LEN);
            } else {
                printf("No such command in history.\n");
                free(cmdline);
                continue;
            }
        }

        // Check if command is meant to run in background
        if (cmdline[strlen(cmdline) - 1] == '&') {
            background = 1;
            cmdline[strlen(cmdline) - 1] = '\0';
        }

        add_to_history(cmdline);

        if ((arglist = tokenize(cmdline)) != NULL) {
            handle_builtin_commands(arglist, &is_builtin, &background);

            if (!is_builtin) {
                execute(arglist, background);
            }

            for (int j = 0; j < MAXARGS + 1; j++)
                free(arglist[j]);
            free(arglist);
            free(cmdline);
        }
    }
    printf("\n");
    return 0;
}

void handle_builtin_commands(char** arglist, int* is_builtin, int* background) {
    *is_builtin = 1;

    if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) {
            printf("cd: expected argument\n");
        } else if (chdir(arglist[1]) != 0) {
            perror("cd");
        }
    } else if (strcmp(arglist[0], "exit") == 0) {
        exit(0);
    } else if (strcmp(arglist[0], "jobs") == 0) {
        print_jobs();
    } else if (strcmp(arglist[0], "kill") == 0) {
        if (arglist[1] == NULL) {
            printf("kill: expected job number\n");
        } else {
            int job_num = atoi(arglist[1]);
            kill_job(job_num);
        }
    } else if (strcmp(arglist[0], "help") == 0) {
        printf("Available commands:\n");
        printf("cd <directory> - change the working directory\n");
        printf("exit - terminate the shell\n");
        printf("jobs - list all background processes\n");
        printf("kill <job_number> - terminate a background job\n");
        printf("help - display this help message\n");
    } else {
        *is_builtin = 0;  // Mark as not a built-in command
    }
}

int execute(char* arglist[], int background) {
    int status;
    int cpid;
    int in_fd = -1, out_fd = -1;
    int pipe_pos = -1;

    // Check for I/O redirection or pipes in arguments
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "<") == 0) {
            in_fd = open(arglist[i + 1], O_RDONLY);
            arglist[i] = NULL;
        } else if (strcmp(arglist[i], ">") == 0) {
            out_fd = open(arglist[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            arglist[i] = NULL;
        } else if (strcmp(arglist[i], "|") == 0) {
            pipe_pos = i;
            arglist[i] = NULL;
        }
    }

    // Handle piping
    if (pipe_pos != -1) {
        int pipefd[2];
        pipe(pipefd);
        cpid = fork();
        if (cpid == 0) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            execvp(arglist[0], arglist);
            perror("Command not found...");
            exit(1);
        }
        close(pipefd[1]);
        if (fork() == 0) {
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            execvp(arglist[pipe_pos + 1], &arglist[pipe_pos + 1]);
            perror("Command not found...");
            exit(1);
        }
        close(pipefd[0]);
        wait(NULL);
        wait(NULL);
    } else {
        // Fork and execute with redirection if no pipe
        cpid = fork();
        if (cpid == 0) {
            if (in_fd != -1) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (out_fd != -1) {
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }
            execvp(arglist[0], arglist);
            perror("Command not found...");
            exit(1);
        }
        if (in_fd != -1) close(in_fd);
        if (out_fd != -1) close(out_fd);
        if (!background) {
            waitpid(cpid, &status, 0);
            printf("child exited with status %d\n", status >> 8);
        } else {
            printf("[%d] %d\n", 1, cpid);
            add_job(cpid, arglist[0]);
        }
    }
    return 0;
}
void add_job(pid_t pid, const char* cmd) {
    if (job_count < HISTORY_SIZE) {
        jobs[job_count].pid = pid;
        strncpy(jobs[job_count].command, cmd, MAX_LEN);
        job_count++;
    } else {
        printf("Job list full. Cannot add more jobs.\n");
    }
}

void remove_job(pid_t pid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pid == pid) {
            for (int j = i; j < job_count - 1; j++) {
                jobs[j] = jobs[j + 1];
            }
            job_count--;
            break;
        }
    }
}

void print_jobs() {
    for (int i = 0; i < job_count; i++) {
        printf("[%d] %d %s\n", i + 1, jobs[i].pid, jobs[i].command);
    }
    if (job_count == 0) {
        printf("No background jobs.\n");
    }
}

void kill_job(int job_num) {
    if (job_num < 1 || job_num > job_count) {
        printf("kill: invalid job number\n");
        return;
    }
    pid_t pid = jobs[job_num - 1].pid;
    if (kill(pid, SIGKILL) == 0) {
        printf("Job [%d] (%d) terminated\n", job_num, pid);
        remove_job(pid);
    } else {
        perror("kill");
    }
}

char** tokenize(char* cmdline) {
    char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
    for (int j = 0; j < MAXARGS + 1; j++) {
        arglist[j] = (char*)malloc(sizeof(char) * ARGLEN);
        bzero(arglist[j], ARGLEN);
    }

    if (cmdline[0] == '\0')
        return NULL;

    int argnum = 0;
    char* cp = cmdline;
    char* start;
    int len;

    while (*cp != '\0') {
        while (*cp == ' ' || *cp == '\t')
            cp++;
        start = cp;
        len = 1;
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t'))
            len++;
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }
    arglist[argnum] = NULL;
    return arglist;
}

char* read_cmd(char* prompt, FILE* fp) {
    printf("%s", prompt);
    int c;
    int pos = 0;
    char* cmdline = (char*)malloc(sizeof(char) * MAX_LEN);

    while ((c = getc(fp)) != EOF) {
        if (c == '\n')
            break;
        cmdline[pos++] = c;
    }
    if (c == EOF && pos == 0)
        return NULL;

    cmdline[pos] = '\0';
    return cmdline;
}

void handle_sigchld(int sig) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        remove_job(pid);
    }
}

void add_to_history(const char* cmd) {
    if (history_count < HISTORY_SIZE) {
        strncpy(history[history_count], cmd, MAX_LEN);
        history_count++;
    } else {
        for (int i = 0; i < HISTORY_SIZE - 1; i++) {
            strncpy(history[i], history[i + 1], MAX_LEN);
        }
        strncpy(history[HISTORY_SIZE - 1], cmd, MAX_LEN);
    }
}

char* get_history_command(int index) {
    if (index >= 0 && index < history_count) {
        return history[index];
    }
    return NULL;
}

void print_history() {
    for (int i = 0; i < history_count; i++) {
        printf("%d %s\n", i + 1, history[i]);
    }
}

