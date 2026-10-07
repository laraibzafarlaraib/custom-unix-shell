# Unix Shell Programming Project

A custom Unix shell implemented in C as a multi-stage operating systems assignment. The project begins with a minimal shell that can read commands, tokenize input, and execute programs, then progressively adds support for features such as redirection, piping, background jobs, command history, job control, and shell variables.

This repository contains six evolving implementations:

- `myshell1.c` – basic shell loop, command tokenization, and `fork()`/`execvp()` execution
- `myshell2.c` – I/O redirection and pipe support
- `myshell3.c` – background process handling with `SIGCHLD`
- `myshell4.c` – command history and `!` command replay
- `myshell5.c` – built-in commands and job management
- `myshell6.c` – environment/local variable support and export behavior

The final shell is the most complete version and is the best candidate for demonstration or extension.

## Project Goals

The objective of this assignment is to build a simplified Linux shell that demonstrates how command-line interpreters work internally. It covers the core ideas behind:

- reading user input from a prompt
- parsing commands into arguments
- creating child processes using `fork()`
- replacing child processes with user programs using `execvp()`
- handling process status and signals
- redirecting standard input/output
- connecting commands with pipes
- tracking shell history and jobs
- managing shell variables and environment state

## Repository Structure

```text
.
├── Assignment-01 (UNIX Shell).pdf
├── README.md
├── myshell1.c
├── myshell2.c
├── myshell3.c
├── myshell4.c
├── myshell5.c
├── myshell6.c
└── .gitignore (if present in your local clone)
```

## Features by Version

### Version 1 – Basic Shell

- displays a prompt
- reads commands from standard input
- splits commands into tokens using whitespace
- executes programs with `fork()` and `execvp()`
- waits for child process completion

### Version 2 – Redirection and Pipes

- supports input redirection: `command < input.txt`
- supports output redirection: `command > output.txt`
- supports command piping: `ls | grep file`

### Version 3 – Background Jobs

- supports background execution with `&`
- prevents zombie processes using `SIGCHLD`
- runs programs asynchronously when requested

### Version 4 – Command History

- stores previously entered commands
- re-runs commands using `!n` and `!-1`
- improves interactive shell usability

### Version 5 – Job Control and Built-ins

- built-in commands:
  - `cd`
  - `exit`
  - `jobs`
  - `kill`
  - `help`
- tracks running background jobs
- can terminate jobs by ID

### Version 6 – Shell Variables

- supports local variables like `NAME=value`
- supports exporting variables with `export NAME`
- lists variables using `set`
- manages variables in a custom shell table

## Building the Shell

Compile the final version:

```bash
gcc myshell6.c -o myshell
```

Or compile any earlier version:

```bash
gcc myshell5.c -o myshell
```

## Running the Program

```bash
./myshell
```

You will see a prompt such as:

```text
PUCITshell:- 
```

Then enter shell commands such as:

```bash
ls
pwd
whoami
date
ls -l /tmp
cat file.txt
grep hello file.txt
ls | wc -l
sleep 5 &
!1
set
NAME=value
export NAME
```

## Example Commands

### Basic execution

```bash
ls
cat README.md
```

### Input and output redirection

```bash
cat < README.md > output.txt
```

### Pipes

```bash
ls -l | grep .c
```

### Background processing

```bash
sleep 10 &
```

### History replay

```bash
!1
!-1
```

### Built-ins in later versions

```bash
cd /tmp
jobs
kill 1
help
exit
```

### Variables in the final version

```bash
MYVAR=hello
set
export MYVAR
```

## Notes

This project is primarily educational and demonstrates how a real shell works at a simplified level. It does not aim to fully replicate bash or zsh, but it covers the fundamental concepts required for understanding Unix process management and shell behavior.

Some areas are intentionally minimal and may not handle every edge case of a production-grade shell, but they provide an excellent foundation for learning operating system concepts.

## Learning Outcomes

By working through the versions in this repository, you will gain practical experience with:

- process creation and waiting
- program execution via `exec` family calls
- inter-process communication with pipes
- file redirection
- signal handling
- job tracking
- command parsing and shell design

## License

This project is provided for academic learning purposes. Please check the assignment instructions or course policy for any usage restrictions.

## Acknowledgement

This assignment is based on the study of Unix process control and shell implementation concepts in an operating systems course environment.
