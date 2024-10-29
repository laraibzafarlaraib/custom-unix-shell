#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXARGS 100
#define MAXLINE 1024

// Function prototypes
int execute(char *arglist[]);
void parse_command(char *arglist[], char *cmd1[], char *cmd2[], char **infile, char **outfile);

// Main function to run the shell
int main() {
    char line[MAXLINE];
    char *arglist[MAXARGS + 1];

    printf("PUCITshell@/home/arif/:- ");
    while (fgets(line, MAXLINE, stdin) != NULL) {
        // Parse the input line into arguments
        int i = 0;
        arglist[i] = strtok(line, " \n");
        while (arglist[i] != NULL) {
            i++;
            arglist[i] = strtok(NULL, " \n");
        }

        // Check if the command is "exit" or "quit" to terminate the shell
        if (arglist[0] != NULL && (strcmp(arglist[0], "exit") == 0 || strcmp(arglist[0], "quit") == 0)) {
            printf("Exiting shell...\n");
            break;
        }

        if (arglist[0] != NULL) {
            // Execute the command
            execute(arglist);
        }

        printf("PUCITshell@/home/arif/:- ");
    }
    return 0;
}

// Execute function to handle piping and redirection
int execute(char *arglist[]) {
    int pipefd[2];
    int status;
    pid_t pid1, pid2;
    char *cmd1[MAXARGS + 1], *cmd2[MAXARGS + 1];
    char *infile = NULL, *outfile = NULL;

    // Parse the command for piping and redirection
    parse_command(arglist, cmd1, cmd2, &infile, &outfile);

    // Create a pipe if there's a second command (cmd2)
    if (cmd2[0] != NULL && pipe(pipefd) == -1) {
        perror("pipe failed");
        return -1;
    }

    if ((pid1 = fork()) == -1) {
        perror("fork failed");
        return -1;
    }

    if (pid1 == 0) {
        // First child process for cmd1
        if (infile) {
            int fd_in = open(infile, O_RDONLY);
            if (fd_in == -1) {
                perror("open infile failed");
                exit(1);
            }
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
        if (cmd2[0] != NULL) {
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
        } else if (outfile) {
            int fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out == -1) {
                perror("open outfile failed");
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        if (cmd2[0] != NULL) close(pipefd[1]);
        execvp(cmd1[0], cmd1);
        perror("execvp failed");
        exit(1);
    }

    if (cmd2[0] != NULL && (pid2 = fork()) == -1) {
        perror("fork failed");
        return -1;
    }

    if (cmd2[0] != NULL && pid2 == 0) {
        // Second child process for cmd2
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        if (outfile) {
            int fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out == -1) {
                perror("open outfile failed");
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        execvp(cmd2[0], cmd2);
        perror("execvp failed");
        exit(1);
    }

    // Parent process
    if (cmd2[0] != NULL) {
        close(pipefd[0]);
        close(pipefd[1]);
    }
    waitpid(pid1, &status, 0);
    if (cmd2[0] != NULL) waitpid(pid2, &status, 0);
    return 0;
}

// Parse function to split commands, and find input/output redirection files
void parse_command(char *arglist[], char *cmd1[], char *cmd2[], char **infile, char **outfile) {
    int i = 0, cmd1_end = 0;
    *infile = NULL;
    *outfile = NULL;

    // Parse cmd1, pipe, and redirection
    while (arglist[i] != NULL) {
        if (strcmp(arglist[i], "<") == 0) {
            *infile = arglist[++i];
        } else if (strcmp(arglist[i], ">") == 0) {
            *outfile = arglist[++i];
        } else if (strcmp(arglist[i], "|") == 0) {
            i++;
            int cmd2_end = 0;
            while (arglist[i] != NULL) {
                cmd2[cmd2_end++] = arglist[i++];
            }
            cmd2[cmd2_end] = NULL;
            break;
        } else {
            cmd1[cmd1_end++] = arglist[i];
        }
        i++;
    }
    cmd1[cmd1_end] = NULL;
    if (cmd2[0] == NULL) {
        cmd2[0] = NULL;
    }
}

