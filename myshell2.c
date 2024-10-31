#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "PUCITshell:- "

int execute(char* arglist[]);
char** tokenize(char* cmdline);
char* read_cmd(char*, FILE*);

int main(){
   char *cmdline;
   char** arglist;
   char* prompt = PROMPT;   
   while((cmdline = read_cmd(prompt, stdin)) != NULL){
      if((arglist = tokenize(cmdline)) != NULL){
         execute(arglist);
         for(int j=0; j < MAXARGS+1; j++)
            free(arglist[j]);
         free(arglist);
         free(cmdline);
      }
   }
   printf("\n");
   return 0;
}

int execute(char* arglist[]){
   int status;
   int cpid;
   int in_fd = -1, out_fd = -1;
   int pipe_pos = -1;

   // Check for I/O redirection or pipes in arguments
   for(int i = 0; arglist[i] != NULL; i++){
      if(strcmp(arglist[i], "<") == 0){
         in_fd = open(arglist[i+1], O_RDONLY);
         arglist[i] = NULL;  // Terminate argument list before '<'
      }
      else if(strcmp(arglist[i], ">") == 0){
         out_fd = open(arglist[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
         arglist[i] = NULL;  // Terminate argument list before '>'
      }
      else if(strcmp(arglist[i], "|") == 0){
         pipe_pos = i;
         arglist[i] = NULL;  // Terminate first part of command at '|'
      }
   }

   // Handle piping
   if(pipe_pos != -1){
      int pipefd[2];
      pipe(pipefd);
      cpid = fork();
      if(cpid == 0){
         close(pipefd[0]);
         dup2(pipefd[1], STDOUT_FILENO);
         close(pipefd[1]);
         execvp(arglist[0], arglist);
         perror("Command not found...");
         exit(1);
      }
      close(pipefd[1]);
      if(fork() == 0){
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
      if(cpid == 0){
         if(in_fd != -1){
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
         }
         if(out_fd != -1){
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
         }
         execvp(arglist[0], arglist);
         perror("Command not found...");
         exit(1);
      }
      if(in_fd != -1) close(in_fd);
      if(out_fd != -1) close(out_fd);
      waitpid(cpid, &status, 0);
      printf("child exited with status %d\n", status >> 8);
   }
   return 0;
}

char** tokenize(char* cmdline){
   char** arglist = (char**)malloc(sizeof(char*) * (MAXARGS + 1));
   for(int j = 0; j < MAXARGS + 1; j++){
      arglist[j] = (char*)malloc(sizeof(char) * ARGLEN);
      bzero(arglist[j], ARGLEN);
   }
   if(cmdline[0] == '\0')
      return NULL;
   int argnum = 0;
   char* cp = cmdline;
   char* start;
   int len;
   while(*cp != '\0'){
      while(*cp == ' ' || *cp == '\t')
         cp++;
      start = cp;
      len = 1;
      while(*++cp != '\0' && !(*cp == ' ' || *cp == '\t'))
         len++;
      strncpy(arglist[argnum], start, len);
      arglist[argnum][len] = '\0';
      argnum++;
   }
   arglist[argnum] = NULL;
   return arglist;
}      

char* read_cmd(char* prompt, FILE* fp){
   printf("%s", prompt);
   int c;
   int pos = 0;
   char* cmdline = (char*)malloc(sizeof(char) * MAX_LEN);
   while((c = getc(fp)) != EOF){
      if(c == '\n')
         break;
      cmdline[pos++] = c;
   }
   if(c == EOF && pos == 0)
      return NULL;
   cmdline[pos] = '\0';
   return cmdline;
}

