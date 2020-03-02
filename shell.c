/*
TODOS:
Built in shell commands (cd, env, amp, etc.) Should be very easy
Error handling
ERROR CHECK!!!!
*/

/*
Error Log:
Redirect not overwriting files, tweak permissions
General cleanup & consistency, esp variables, pci logic
Amp issues
*/

/*
FOR FUNSIES:
Make this thing really powerful. This could be a cool project. Maybe even make a Windows CP Emulator from it.
Increase efficiency 10x. There's a better parsing algorithm out there
Up arrow to display history
Make this thing less hard coded--FUNCTIONS
*/

/*
Brady Kruse
bak225
Sources:
1) Redirect Skeleton Code: http://www.cs.loyola.edu/~jglenn/702/S2005/Examples/dup2.html
2) Mr. Ritter's "Babyshell"
3) Help from http://www.cs.loyola.edu/~jglenn/702/S2005/Examples/dup2.html, https://www.geeksforgeeks.org/c-program-demonstrate-fork-and-pipe/, 
   http://www.microhowto.info/howto/capture_the_output_of_a_child_process_in_c.html, http://heapspray.net/post/redirect-stdout-of-child-to-parent-process-in-c/
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
    fprintf(stderr, "\nCAUGHT CTRL + C\n");
    exit(0);
  }

  if(sig == SIGUSR1)
  {
    fprintf(stderr, "\nCAUGHT SIGUSR1\n");
    {
      for(int i = run_count; i < 30*loop; i++)
      {
      fprintf(stderr, "History %d: %s\n", i, history[i]);
      }

      for(int i = 0; i < run_count; i++)
      {
        fprintf(stderr, "History %d: %s\n", i+(30*loop), history[i]);
      }
    }
    exit(0);
  }
}

int main()
{

  signal(SIGINT, handle_sig); //Setting up of signal handlers
  signal(SIGUSR1, handle_sig);

  char *prompt = "BRADv4> "; //Parsing variables
  char line[256];
  int p[2];
  int token_count = 0;
  char *argv_l[10];
  char *argv_r[10];
  char *redir_file;
  char *read_file;
  int p_true, append, redir, r_true, read_pipe, read_pipe_append, read_pipe_redir, read_redir, read_append, amp, norm, pci, pipe_redir, pipe_append, current_tok;

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
          if(!strncmp(token, "&", 256))
          {
            amp = 1;
          }
          else{
            strcpy(v_line[token_count+1], token);
          }
        }
        token_count++;
      }
      
      //Logic handler of commands
      pci = 0;
      current_tok = 0;
      while(current_tok != token_count) //Walk through every token
      {

        if(!strncmp(v_line[current_tok], "|", 256)) //Found a pipe
        {
          p_true = 1;

          int pipe_to = current_tok+1;
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
          pci = l;


          char argv_2[10][256];
          l = 0;
          for(int j = pci+1; j < token_count; j++) //Grab everything after the pipe
            {
              if(!strncmp(v_line[j], ">", 256)) //Check for redirection
                {
                  pipe_redir = 1;
                  redir_file = v_line[j+1];
                  pci = j;
                  current_tok = j+1;
                  break;
                }

              if(!strncmp(v_line[j], ">>", 256)) //Check for appending
                {
                  pipe_append = 1;
                  redir_file = v_line[j+1];
                  pci = j;
                  current_tok = j+1;
                  break;
                }

              strcpy(argv_2[l], v_line[j]);
              l++;
            }
          
          for(int m = 0; m < l; m++){argv_r[m] = argv_2[m];} //Assign right arguments
          argv_r[l] = NULL;
          current_tok += l;
        }

        else if(!strncmp(v_line[current_tok], ">", 256)) //Found a redirect
        {
          redir = 1;
          int redir_to = current_tok+1;
          int k = pci;
          int l = 0;
          char argv[10][256];
          for(int j = 0; j < redir_to - pci - 1; j++) //Grab everything before redirect
            {
              strcpy(argv[l], v_line[k]);
              k++;
              l++;
            }
          for(int m = 0; m < l; m++){argv_l[m] = argv[m];} //Assign left arguments
          argv_l[l] = NULL;
          pci = l;

          redir_file = v_line[redir_to];
          current_tok++;
        }

        else if(!strncmp(v_line[current_tok], ">>", 256)) //Found an append
        {
          append = 1;
          int append_to = current_tok+1;
          int k = pci;
          int l = 0;
          char argv[10][256];
          for(int j = 0; j < append_to - pci - 1; j++) //Grab everything before redirect
            {
              strcpy(argv[l], v_line[k]);
              k++;
              l++;
            }
          for(int m = 0; m < l; m++){argv_l[m] = argv[m];} //Assign left arguments
          argv_l[l] = NULL;
          pci = l;

          redir_file = v_line[append_to];
          current_tok++;
        }

        else if(!strncmp(v_line[current_tok], "<", 256)) //Found a read
        {
          r_true = 1;
          int read_from  = current_tok+1;
          int k = pci;
          int l = 0;
          char argv[10][256];
          for(int j = 0; j < read_from - pci - 1; j++) //Grab everything before read
            {
              strcpy(argv[l], v_line[k]);
              k++;
              l++;
            }
          for(int m = 0; m < l; m++){argv_l[m] = argv[m];} //Assign left arguments
          argv_l[l] = NULL;
          pci = l;

          read_file = v_line[read_from];
          current_tok++;
          fprintf(stderr, "%d", current_tok);

          l = 0;
          char argv_2[10][256];
          for(int j = pci+1; j < token_count; j++) //Figure out what comes after the read
          {
            if(!strncmp(v_line[j], ">", 256)){ //Find redirect
              read_redir = 1;
              redir_file = v_line[j+1];
              current_tok++;
              pci = j;
              break;
            }

            else if(!strncmp(v_line[j], ">>", 256)){ //Find append
              read_append = 1;
              redir_file = v_line[j+1];
              current_tok++;
              pci = j;
              break;
            }

            else if(!strncmp(v_line[j], "|", 256)){ //Find pipe and record pipe arguments
              read_pipe = 1;
              current_tok++;
              pci = j;
              k = pci+1;
            }

            if(read_pipe)
            {
              strcpy(argv[l], v_line[k]);
              k++;
              l++;
              current_tok++;
            }
          }

          for(int m = 0; m < l; m++){argv_l[m] = argv[m];}
          argv_l[l] = NULL;

        }

        else{
          current_tok++; //ERROR: This needs to be changed to execute normal commands
        }
        
        
        
      }

      if(p_true) //TEST: Piping
      { 
        p_true = 0;
        pipe(p); //One pipe, parent controls between the two

        switch(pid=fork()) //1st child
        {
          case 0: //Left process
            close(p[0]); //no need to read, just writes to its pipe.
            dup2(p[1], STDOUT_FILENO);
            close(p[1]);
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
            if(pipe_redir)
            {
              out = open(redir_file, O_WRONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
              dup2(out, STDOUT_FILENO);
              close(out);
            }

            if(pipe_append)
            {
              out = open(redir_file, O_APPEND | O_WRONLY, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
              dup2(out, STDOUT_FILENO);
              close(out);
            }
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

      else if(redir) //TEST: Redirection
      { 
        redir = 0;
        switch(pid = fork())
        {
          case 0:
             out = open(redir_file, O_WRONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
             dup2(out, STDOUT_FILENO);
             close(out);
             execvp(argv_l[0], argv_l);
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

      else if(append) //TEST: Append
      {
        append = 0;
        switch(pid = fork())
        {
          case 0:
             out = open(redir_file, O_APPEND | O_WRONLY, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
             dup2(out, STDOUT_FILENO);
             close(out);
             execvp(argv_l[0] , argv_l);
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

      else if(r_true) //TEST: Read
      {
        r_true = 0;

        if(read_pipe){ //Read + Pipe
          read_pipe = 0;
          pipe(p); //One pipe, parent controls between the two

          switch(pid=fork()) //1st child
          {
            case 0: //Left process
              in = open(read_file, O_RDONLY); //Read from file
              dup2(in, STDIN_FILENO);
              close(in);

              close(p[0]); //no need to read, just writes to its pipe.
              dup2(p[1], STDOUT_FILENO);
              close(p[1]);
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
              if(read_redir)
              {
                out = open(redir_file, O_WRONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                dup2(out, STDOUT_FILENO);
                close(out);
              }

              if(read_append)
              {
                out = open(redir_file, O_APPEND | O_WRONLY, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                dup2(out, STDOUT_FILENO);
                close(out);
              }
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

        else{
          if(read_redir)
          {
            switch(pid = fork())
            {
              case 0:
                in = open(read_file, O_RDONLY);
                out = open(redir_file, O_WRONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR); 
                dup2(in, STDIN_FILENO);
                dup2(out, STDOUT_FILENO);
                close(in);
                close(out);
                execvp(argv_l[0] , argv_l);
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

          else if(read_append)
          {
            switch(pid = fork())
            {
              case 0:
                in = open(read_file, O_RDONLY);
                out = open(redir_file, O_APPEND, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                dup2(in, STDIN_FILENO);
                dup2(out, STDOUT_FILENO);
                close(in);
                close(out);
                execvp(argv_l[0] , argv_l);
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

          else{
            switch(pid = fork())
            {
              r_true = 0;
              case 0:
                in = open(read_file, O_RDONLY);
                dup2(in, STDIN_FILENO);
                close(in);
                execvp(argv_l[0], argv_l);
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
        }
      }

      else
      {
        for(int i = 0; i < token_count; i++){argv_l[i] = v_line[i];} //Assign left arguments
          argv_l[token_count+1] = NULL;
        switch(pid = fork()) 
        {
          case 0:
            execvp(argv_l[0], argv_l);
            exit(1);
          
          case -1: fprintf(stderr, "ERROR CAN'T CREATE CHILD PROCESS\n");
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
      for(int i = 0; i < 10; i++){ //Clear out argument tags
        argv_l[i] = NULL;
        argv_r[i] = NULL;
      }
    }
  exit(0);
}


  
