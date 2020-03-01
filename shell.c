/*
TODOS:
Built in shell commands (cd, env, amp, etc.) Should be very easy
Argument parsing & handling
  - Function that determines what to run that is itself run in every process?
  - Parsing through and setting indicators to true / false and labeling tokens
*/

/*
FOR FUNSIES:
Make this thing really powerful. This could be a cool project. Maybe even make a Windows CP Emulator from it.
Increase efficiency 10x. There's a better parsing algorithm out there
*/


/*
Brady Kruse
bak225
Sources:
1) Vector Implementation: https://gist.github.com/EmilHernvall/953968/0fef1b1f826a8c3d8cfb74b2915f17d2944ec1d0
2) Redirect Skeleton Code: http://www.cs.loyola.edu/~jglenn/702/S2005/Examples/dup2.html
3) Mr. Ritter's "Babyshell"
4) Help from http://www.cs.loyola.edu/~jglenn/702/S2005/Examples/dup2.html, https://www.geeksforgeeks.org/c-program-demonstrate-fork-and-pipe/, 
   http://www.microhowto.info/howto/capture_the_output_of_a_child_process_in_c.html, http://heapspray.net/post/redirect-stdout-of-child-to-parent-process-in-c/
5) Circular Buffer Adapted From: http://www.equestionanswers.com/c/c-circular-buffer.php
*/

#include <stdio.h> //All necessary libraries
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "vector.h"


char history[30][256]; //History variables, created here so the signal handler can use them.
int run_count = 0;
int loop = 0;

void handle_sig(int sig) //Signal handler, handles CTRL + C & SIGUSR1
{
  if(sig == SIGINT)
  {
    printf("\nCAUGHT CTRL + C\n");
    exit(0);
  }

  if(sig == SIGUSR1)
  {
    printf("\nCAUGHT SIGUSR1\n");
    {
      for(int i = run_count; i < 30*loop; i++)
      {
      printf("History %d: %s\n", i, history[i]);
      }

      for(int i = 0; i < run_count; i++)
      {
        printf("History %d: %s\n", i+(30*loop), history[i]);
      }
    }
    exit(0);
  }
}

int main()
{

  signal(SIGINT, handle_sig); //Setting up of signal handlers
  signal(SIGUSR1, handle_sig);

  char *prompt = "BRADv3> "; //Parsing variables
  char line[256];
  int p[2];
  int token_count = 0;
  char *argv_l[10];
  char *argv_r[10];
  int p_true, append, redir, read, read_pipe, read_redir, amp, norm, pci, pipe_to;

  pid_t pid; //Forking & Piping Variables
  int in, out;


  fprintf(stderr,"%s",prompt);
  while ( scanf ("%[^\n]%*c",line) != EOF )
    { 
      char v_line[10][256];
      strcpy(history[run_count], line); //History record, max up to 30, circular buffer
      run_count++;
      if(run_count == 30)
      {
        run_count = 0;
        loop++;
      }


      char * token = "";
      strcpy(v_line[0], strtok(line, " ,."));
      while(token != NULL)
      { 
        token = strtok(NULL, " ,.");
        if(token != NULL)
        {
          strcpy(v_line[token_count+1], token);
        }
        token_count++;
      }
      
      //Logic handler of commands
      pci = 0;
      for(int i = 0; i < token_count; i++) //Walk through every token
      {

        if(!strncmp(v_line[i], "|", 256)) //Found a pipe
        {
          p_true = 1;
          pipe_to = i+1;

          int k = pci;
          int l = 0;
          char argv[10][256];
          for(int j = 0; j < pipe_to - pci - 1; j++) //Grab everything before the pipe
            {
              strcpy(argv[l], v_line[k]);
              k++;
              l++;
            }

          for(int m = 0; m < l; m++){argv_l[m] = argv[m];} //Assign left arguments
          argv_l[l] = NULL;
          pci = i;

          char argv_2[10][256];
          l = 0;
          fprintf(stderr, "%d", token_count);
          for(int j = pci+1; j < token_count; j++) //Grab everything after the pipe
            {
              strcpy(argv_2[l], v_line[j]);
              fprintf(stderr, "%s\n", argv_2[l]);
              strcpy(v_line[j], "");
              l++;
            }
          
          for(int m = 0; m < l; m++){argv_r[m] = argv_2[m]; fprintf(stderr, "Token: %s\n", argv_r[m]);} //Assign right arguments
          argv_r[l] = NULL;
        }

        
      } //This needs to be moved down to include everything. Walk through every if every time.

      
      /*
      Loop Pseudo Code
      for(word in token)
        previous_command_index = 0
        if word == |, pipe = true, pipe_to = token[word+2], argv = array[previous_command_index, i], arg = argv[0], pvi = i;
        if word == >>, app = true, file_app_to = token[word+2], ""
          scan argv for pipe
          if found, pipe_write = true, app = false
        if word == >, redirect = true, file_redirect_to = token[word+2], ""
          scan arv for pipe
          if found, pipe_write = true, app = false
        if word = <
          if word > in word:
            argv_read = everything before word and last command
            argv_pass = everything between <> (run this first)
            output_file = last element of vector
            for each in argv_pass:
              scan for pipes, if found, run read & write w/ a pipe
              else: run read & write w/out pipe
          else:
           arv_pass = everything after word and last command
           scan argv_pass for pipes
           if found, read_pipe = 1 (write this into pipe code to write to arv_read

        if array_last_elem = &:
          amp = 1
      */
      
      if(norm) //DEBUG: Standard Process Creation
      {
        switch(pid = fork()) 
        {
          case 0:
            //execvp("ls", test_ls_arg);
            exit(1);
          
          case -1: fprintf(stderr, "ERROR CAN'T CREATE CHILD PROCESS\n");
            break;

          default:
            break;
        }
      }

      if(1) //DEBUG: Piping
      {
        pipe(p); //One pipe, parent controls between the two

        switch(pid=fork()) //1st child
        {
          case 0: //Left process
            close(p[0]); //no need to read, just writes to its pipe.
            dup2(p[1], STDOUT_FILENO);
            close(p[1]);
            fprintf(stderr, "ABOUT TO RUN %s\n", argv_l[0]);
            execvp(argv_l[0], argv_l);
            exit(1);

          case -1: fprintf(stderr, "ERROR CAN'T CREATE CHILD PROCESS\n");
            break;

          default:
            break;
        }

        switch(pid = fork()) //Start second process
        {
          case 0: //Right process
            close(p[1]);
            dup2(p[0], STDIN_FILENO); //Reads from its pipe
            close(p[0]);
            execvp(argv_r[0], argv_r);
            exit(1);

          case -1: fprintf(stderr, "ERROR can't create child process\n");
            break;

          default:
            break;
        }
        
        close(p[0]); //Parent handles pipes
        close(p[1]);
        if(!amp) //Active command vs. run in background
        {
          wait(NULL);
          wait(NULL);
        }
      }

      if(0) //DEBUG: Redirection
      {
        switch(pid = fork())
        {
          case 0:
             out = open("test", O_WRONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
             dup2(out, STDOUT_FILENO);
             close(out);
             //execvp("ls", test_ls_arg);
             exit(1);
             break;

          case -1: fprintf(stderr,"ERROR can't create child process!\n");
            break;

          default:
            if(!amp)
              {
              wait(NULL);
              }
            break;
        }
      }

      if(0) //DEBUG: Read
      {
        switch(pid = fork())
        {
          case 0:
             in = open("test", O_RDONLY);
             dup2(in, STDIN_FILENO);
             close(in);
             //execvp("wc", test_wc_arg);
             exit(1);
             break;

          case -1: fprintf(stderr,"ERROR can't create child process!\n");
            break;

          default:
            if(!amp)
              {
              wait(NULL);
              }
            break;
        }
      }

      if(0) //DEBUG: Append
      {
        switch(pid = fork())
        {
          case 0:
             out = open("test", O_APPEND | O_WRONLY, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
             dup2(out, STDOUT_FILENO);
             close(out);
             //execvp("ls" , test_ls_arg);
             exit(1);
             break;

          case -1: fprintf(stderr,"ERROR can't create child process!\n");
            break;

          default:
            if(!amp)
              {
              wait(NULL);
              }
            break;
        }
      }

      if(0) //DEBUG: Read & Redirect
      {
        switch(pid = fork())
        {
          case 0:
            in = open("test", O_RDONLY);
            out = open("test2", O_APPEND | O_WRONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
            dup2(in, STDIN_FILENO);
            dup2(out, STDOUT_FILENO);
            close(in);
            close(out);
            //execvp("wc" , test_wc_arg);
            break;
          
          case -1: fprintf(stderr,"ERROR can't create child process!\n");
            break;

          default:
            if(!amp)
              {
              wait(NULL);
              }
            break;
        }
      }

      fprintf(stderr,"%s",prompt); 
      token_count = 0;
    }
  exit(0);
}


  
