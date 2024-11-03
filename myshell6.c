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
#define MAX_VARS 100

// Structure for storing variables
struct var {
    char *str;  // "name=value" string
    int global; // Boolean: 1 for environment variable, 0 for local
};

struct var var_table[MAX_VARS]; // Array to store variables
int var_count = 0;

int execute(char* arglist[], int background);
char** tokenize(char* cmdline);
char* read_cmd(char*, FILE*);
void handle_sigchld(int sig);
void add_to_history(const char* cmd);
char* get_history_command(int index);
void print_history();

// New variable functions
void set_variable(const char *name, const char *value, int global);
char* get_variable(const char *name);
void list_variables();
void export_variable(const char *name);

char history[HISTORY_SIZE][MAX_LEN];
int history_count = 0;

int main() {
    char *cmdline;
    char** arglist;
    char* prompt = PROMPT;

    signal(SIGCHLD, handle_sigchld);

    while ((cmdline = read_cmd(prompt, stdin)) != NULL) {
        int background = 0;

        // Handle variable assignments (e.g., VAR=value)
        char *equal_sign = strchr(cmdline, '=');
        if (equal_sign != NULL) {
            *equal_sign = '\0';
            const char *name = cmdline;
            const char *value = equal_sign + 1;
            set_variable(name, value, 0); // Add as a local variable
            free(cmdline);
            continue;
        }

        // Check for special commands like "export" and "set"
        if (strncmp(cmdline, "export ", 7) == 0) {
            export_variable(cmdline + 7);
            free(cmdline);
            continue;
        }
        if (strcmp(cmdline, "set") == 0) {
            list_variables();
            free(cmdline);
            continue;
        }

        if (cmdline[strlen(cmdline) - 1] == '&') {
            background = 1;
            cmdline[strlen(cmdline) - 1] = '\0';
        }

        add_to_history(cmdline);

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

// Function to set or update a variable
void set_variable(const char *name, const char *value, int global) {
    for (int i = 0; i < var_count; i++) {
        if (strncmp(var_table[i].str, name, strlen(name)) == 0 && var_table[i].str[strlen(name)] == '=') {
            free(var_table[i].str);
            var_table[i].str = malloc(strlen(name) + strlen(value) + 2);
            sprintf(var_table[i].str, "%s=%s", name, value);
            var_table[i].global = global;
            if (global) putenv(var_table[i].str);
            return;
        }
    }
    if (var_count < MAX_VARS) {
        var_table[var_count].str = malloc(strlen(name) + strlen(value) + 2);
        sprintf(var_table[var_count].str, "%s=%s", name, value);
        var_table[var_count].global = global;
        if (global) putenv(var_table[var_count].str);
        var_count++;
    } else {
        printf("Error: Variable table full.\n");
    }
}

// Function to retrieve a variable's value
char* get_variable(const char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strncmp(var_table[i].str, name, strlen(name)) == 0 && var_table[i].str[strlen(name)] == '=') {
            return strchr(var_table[i].str, '=') + 1;
        }
    }
    return NULL;
}

// Function to list all variables
void list_variables() {
    for (int i = 0; i < var_count; i++) {
        printf("%s\n", var_table[i].str);
    }
}

// Function to export a local variable to the environment
void export_variable(const char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strncmp(var_table[i].str, name, strlen(name)) == 0 && var_table[i].str[strlen(name)] == '=') {
            var_table[i].global = 1;
            putenv(var_table[i].str);
            return;
        }
    }
    printf("Variable %s not found.\n", name);
}

// Remaining code for execute, tokenize, etc., remains unchanged
