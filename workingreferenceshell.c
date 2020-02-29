#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef READ
#define READ 0
#endif

#ifndef WRITE
#define WRITE 1
#endif

void clearArgIndexContainer (int argLocation[]);

int main() {
    /* variables for command parsing and storage*/
    char n, *parser, buf[80], *argv[20];
    int m, status, inword, continu;

    /* variables and flags for redirection (note: C does not have type bool; using integer value 0 or 1) */
    char *in_path, *out_path;
    int inputRedirectFlag, outputRedirectFlag;

    /* variables for piping */
    int count, pipes;
    pid_t pid;

    /* left and right pipes */
    int l_pipe[2], r_pipe[2];

    /* container for recording argument locations in argv[] */
    int argLocation[20] = { 0 };

    while (1) {

        /* reset parsing and piping variable values */
        m = inword = continu = count = pipes = pid = 0;

        /* begin parsing at beginning of buffer */
        parser = buf;

        /* reset redirection flags */
        inputRedirectFlag = outputRedirectFlag = 0;

        /* print shell prompt */
        printf("\nshhh> ");

        /* parse commands */
        while ((n = getchar()) != '\n' || continu)
        {
            if (n == ' ') {
                if (inword)
                {
                    inword = 0;
                    *parser++ = 0;
                }
            }
            else if (n == '\n')
                continu = 0;
            else if (n == '\\' && !inword)
                continu = 1;
            else {
                if (!inword)
                {
                    inword = 1;
                    argv[m++] = parser;
                    *parser++ = n;
                }
                else
                    *parser++ = n;
            }
        } /* end of command parsing */

        /* append terminating character to end of parser buffer and argv buffer */
        *parser++ = 0;
        argv[m] = 0;

        /* user wishes to terminate program */
        if (strcmp(argv[0], "exit") == 0)
            exit(0);

        /* manage redirection */
        while (argv[count] != 0) {
            if (strcmp(argv[count], "|") == 0) {
                argv[count] = 0;
                argLocation[pipes + 1] = count + 1;
                ++pipes;
            }
            else if (strcmp(argv[count], "<") == 0) {
                in_path = strdup(argv[count + 1]);
                argv[count] = 0;
                inputRedirectFlag = 1;
            }
            else if (strcmp(argv[count], ">") == 0) {
                out_path = strdup(argv[count + 1]);
                argv[count] = 0;
                outputRedirectFlag = 1;
            }
            else {
                argLocation[count] = count;
            }

            ++count;
        } /* end of redirection management */

        /* execute commands [<= in for-loop; n pipes require n+1 processes] */
        for (int index = 0; index <= pipes; ++index) {
            if (pipes > 0 && index != pipes) { /* if user has entered multiple commands with '|' */
                pipe(r_pipe); /* no pipe(l_pipe); r_pipe becomes next child's l_pipe */
            }

            /* switch-statement for command execution */
            switch (pid = fork()) {
                case -1: perror("fork failed"); /* fork() error */
                         break;

                case 0: /* child process manages redirection and executes */
                       if ((index == 0) && (inputRedirectFlag == 1)) {
                           int inputFileDescriptor = open(in_path, O_RDONLY , 0400);
                           if (inputFileDescriptor == -1) {
                               perror("input file failed to open\n");
                               return(EXIT_FAILURE);
                           }
                           close(READ);
                           dup(inputFileDescriptor);
                           close(inputFileDescriptor);
                       } /* end of input redirection management */

                       if ((index == pipes) && (outputRedirectFlag == 1)) {
                           //printf("DEBUG: here we should be about to create our output file\n");
                           int outputFileDescriptor = creat(out_path, 0700);
                           if (outputFileDescriptor < 0) {
                               perror("output file failed to open\n");
                               return(EXIT_FAILURE);
                           }
                           close(WRITE);
                           dup(outputFileDescriptor);
                           close(outputFileDescriptor);
                       } /* end of output redirection management */

                        /* manage pipes if (a) first child process, (b) in-between child process, or (c) final child process */
                        if (pipes > 0) {
                            if (index == 0){ /* first child process */
                                close(WRITE);
                                dup(r_pipe[WRITE]);
                                close(r_pipe[WRITE]);
                                close(r_pipe[READ]);
                            }
                            else if (index < pipes) { /* in-between child process */
                                close(READ);
                                dup(l_pipe[READ]);
                                close(l_pipe[READ]);
                                close(l_pipe[WRITE]);
                                close(WRITE);
                                dup(r_pipe[WRITE]);
                                close(r_pipe[READ]);
                                close(r_pipe[WRITE]);
                            }
                            else { /* final child process */
                                close(READ);
                                dup(l_pipe[READ]);
                                close(l_pipe[READ]);
                                close(l_pipe[WRITE]);
                            }
                        }

                       /* execute command */
                       execvp(argv[argLocation[index]], &argv[argLocation[index]]);

                       /* if execvp() fails */
                       perror("execution of command failed\n");

                       break;

                default: /* parent process manages the pipes for child process(es) */
                        if (index > 0) {
                            close(l_pipe[READ]);
                            close(l_pipe[WRITE]);
                        }
                        l_pipe[READ] = r_pipe[READ];
                        l_pipe[WRITE] = r_pipe[WRITE];

                        /* parent waits for child process to complete */
                        wait(&status);

                        break;
            } /* end of switch-statement for command execution */
        } /* end of loop for all pipes */

        // clear all executed commands
        for (int i = 0; i < 20; ++i) {
            argv[i] = 0;
        }
    }
}

void clearArgIndexContainer (int argLocation[]){
    // clear argument container
    for (int i = 0; i < 20; ++i) {
        argLocation[i] = 0;
    }
}