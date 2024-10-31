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

int execute(char* arglist[], int background);
char** tokenize(char* cmdline);
char* read_cmd(char*, FILE*);
void handle_sigchld(int sig);

int main() {
    char *cmdline;
    char **arglist;
    char *prompt = PROMPT;

    // Set up signal handler for SIGCHLD
    signal(SIGCHLD, handle_sigchld);

    while ((cmdline = read_cmd(prompt, stdin)) != NULL) {
        int background = 0;

        // Check if command is meant to run in background
        if (cmdline[strlen(cmdline) - 1] == '&') {
            background = 1;
            cmdline[strlen(cmdline) - 1] = '\0'; // Remove '&' from cmdline
        }

        if ((arglist = tokenize(cmdline)) != NULL) {
            execute(arglist, background);

            for (int j = 0; j < MAXARGS + 1; j++)
                free(arglist[j]);
            free(arglist);
            free(cmdline);
        }
    }
    printf("\n");
    return 0;
}

int execute(char *arglist[], int background) {
    int status;
    pid_t cpid;
    int in_fd = -1, out_fd = -1;
    int pipe_pos = -1;

    // Check for I/O redirection or pipes in arguments
    for (int i = 0; arglist[i] != NULL; i++) {
        if (strcmp(arglist[i], "<") == 0) {
            in_fd = open(arglist[i + 1], O_RDONLY);
            arglist[i] = NULL; // Terminate argument list before '<'
        } else if (strcmp(arglist[i], ">") == 0) {
            out_fd = open(arglist[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            arglist[i] = NULL; // Terminate argument list before '>'
        } else if (strcmp(arglist[i], "|") == 0) {
            pipe_pos = i;
            arglist[i] = NULL; // Terminate first part of command at '|'
        }
    }

    // Handle piping
    if (pipe_pos != -1) {
        int pipefd[2];
        pipe(pipefd);

        // First child for the command before the pipe
        cpid = fork();
        if (cpid == 0) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            execvp(arglist[0], arglist);
            perror("Command not found...");
            exit(1);
        }

        // Second child for the command after the pipe
        cpid = fork();
        if (cpid == 0) {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);
            execvp(arglist[pipe_pos + 1], &arglist[pipe_pos + 1]);
            perror("Command not found...");
            exit(1);
        }

        // Parent closes pipes and waits for both children
        close(pipefd[0]);
        close(pipefd[1]);
        wait(NULL);
        wait(NULL);

    } else { // No pipe, handle background process and I/O redirection
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

        // Parent handles background process or waits for the child process
        if (!background) {
            waitpid(cpid, &status, 0); // Wait for the child process
        } else {
            printf("[%d] %d\n", 1, cpid); // Print background process info
        }
    }

    return 0;
}

char** tokenize(char *cmdline) {
    char **arglist = (char**) malloc(sizeof(char*) * (MAXARGS + 1));
    for (int j = 0; j < MAXARGS + 1; j++) {
        arglist[j] = (char*) malloc(sizeof(char) * ARGLEN);
        bzero(arglist[j], ARGLEN);
    }

    if (cmdline[0] == '\0') // Handle empty input
        return NULL;

    int argnum = 0; // Slots used
    char *cp = cmdline; // Position in string
    char *start;
    int len;

    while (*cp != '\0') {
        while (*cp == ' ' || *cp == '\t') // Skip leading spaces
            cp++;
        start = cp; // Start of the word
        len = 1;

        // Find the end of the word
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t'))
            len++;
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }
    arglist[argnum] = NULL;
    return arglist;
}

char* read_cmd(char *prompt, FILE *fp) {
    printf("%s", prompt);
    int c;
    int pos = 0;
    char *cmdline = (char*) malloc(sizeof(char) * MAX_LEN);
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
    // Wait for all child processes to prevent zombies
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
