/*
TODOS:
Error handling
*/

/*
Error Log:
SEGFAULT / INF Loop on Enter?
*/

/*
FOR FUNSIES:
- Make this thing really powerful. This could be a cool project. Maybe even make a Windows CP Emulator from it.
- Increase efficiency 10x / general cleanup. There's a better parsing algorithm out there. Use functions instead of this shitty logic if-loop bullshit. I'll let it slide for now, but you're better than that.
- Up arrow to display history
- Wait to print stuff so the prompt doesn't get overrun
*/

/*
Brady Kruse
bak225
2/2/20 @ 2:36:00AM

Sources:
1) Redirect Skeleton Code: http://www.cs.loyola.edu/~jglenn/702/S2005/Examples/dup2.html
2) Mr. Ritter's "Babyshell"
3) General Help from http://www.cs.loyola.edu/~jglenn/702/S2005/Examples/dup2.html, https://www.geeksforgeeks.org/c-program-demonstrate-fork-and-pipe/, 
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
#include <limits.h>


char history[30][256]; //History variables, created here so the signal handler can use them.
int run_count = 0;
int loop = 0;

void handle_sig(int sig) //Signal handler, handles CTRL + C & SIGUSR1
{
  if(sig == SIGINT)
  {
    fprintf(stderr, "\nCAUGHT CTRL + C\n");
  }

  if(sig == SIGUSR1)
  {
    fprintf(stderr, "\nCAUGHT SIGUSR1\n");
    {
      for(int i = run_count; i < 30*loop; i++)
      {
        fprintf(stderr, "CMD %d: %s\n", i, history[i]);
      }

      for(int i = 0; i < run_count; i++)
      {
        fprintf(stderr, "CMD %d: %s\n", i+(30*loop), history[i]);
      }
    }
    exit(0);
  }
}

int main()
{

  signal(SIGINT, handle_sig); //Setting up of signal handlers
  signal(SIGUSR1, handle_sig);

  char *prompt = "B-RADv6> "; //Parsing variables
  char *line;
  size_t size = 256;
  int token_count = 0;
  int pci;
  int current_tok;

  char *argv_l[10]; //Execution Variables
  char *argv_r[10];
  char *redir_file;
  char *read_file;
  int amp;
  char home_path[PATH_MAX];
  getcwd(home_path, sizeof(home_path));
  pid_t kid_pids[100];
  int kid_count = 0;
  char kid_names[100][256];
  int* kid_status;

  pid_t pid; //Forking & Piping Variables
  int in, out;
  int p[2];

  int append, redir; //Standard variables
  int p_true, p_redir, p_append; //Pipe variables
  int r_true, r_pipe, r_redir, r_append; //Read variables

  fprintf(stderr,"%s",prompt);
  while (getline(&line, &size, stdin) != EOF ) //While-loop for shell
    { 
      if(!strcmp(line, "\n"))
      {
        goto end;
      }
      char v_line[10][256];
      strcpy(history[run_count], line); //History record, max up to 30, circular buffer
      run_count++;
      if(run_count == 30)
      {
        run_count = 0;
        loop++;
      }

      char * token = "";
      strcpy(v_line[0], strtok(line, " \n,")); //Scan each line, divvy into tokens
      while(token != NULL)
      { 
        token = strtok(NULL, " \n,");
        if(token != NULL)
        { 
          if(!strcmp(token, "&"))
          {
            amp = 1;
            token = NULL;
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
      while(current_tok < token_count) //Walk through every token
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
              argv_l[l] = v_line[k];
              k++;
              l++;
            } 
          argv_l[l] = NULL;
          pci = l;

          l = 0;
          for(int j = pci+1; j < token_count; j++) //Grab everything after the pipe //SOMEHOW, the right is receiving old arguments
            {
              if(!strncmp(v_line[j], ">", 256)) //Check for redirection
                {
                  p_redir = 1;
                  redir_file = v_line[j+1];
                  pci = j;
                  current_tok = j+1;
                  break;
                }

              if(!strncmp(v_line[j], ">>", 256)) //Check for appending
                {
                  p_append = 1;
                  redir_file = v_line[j+1];
                  pci = j;
                  current_tok = j+1;
                  break;
                }

              argv_r[l] = v_line[j];
              l++;
            }
          argv_r[l] = NULL;
          current_tok = token_count;
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
          for(int j = 0; j < read_from - pci - 1; j++) //Grab everything before read
            {
              argv_l[l] = v_line[k];
              k++;
              l++;
            }
          argv_l[l] = NULL;
          pci = l;

          read_file = v_line[read_from];
          current_tok = current_tok+2;

          l = 0;
          for(int j = pci+1; j < token_count; j++) //Grab everything after read
          {
            if(!strncmp(v_line[j], ">", 256)){ //Found redirect
              r_redir = 1;
              redir_file = v_line[j+1];
              current_tok = token_count;
              pci = j;
              break;
            }

            else if(!strncmp(v_line[j], ">>", 256)){ //Found append
              r_append = 1;
              redir_file = v_line[j+1];
              current_tok = token_count;
              pci = j;
              break;
            }

            else if(!strncmp(v_line[j], "|", 256)){ //Found pipe, continue scanning for redirect / append
              r_pipe = 1;
              current_tok++;
              pci = j;
              k = pci+1;
            }

            else//Record pipe arguments
            {
              if(r_pipe)
              {
                argv_r[l] = v_line[k];
                k++;
                l++;
                current_tok++;
              }
            }
          }
          argv_r[l] = NULL;

        }

        else{
          current_tok++;
        }
      }

      if(p_true) //Piping
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
            strcpy(kid_names[kid_count], argv_l[0]);
            kid_pids[kid_count++] = pid;
            break;
        }

        switch(pid = fork()) //Start second process
        {
          case 0: //Right process
            close(p[1]);
            dup2(p[0], STDIN_FILENO); //Reads from its pipe
            close(p[0]);

            if(p_redir) //Check for redir
            {
              p_redir = 0;
              out = open(redir_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
              dup2(out, STDOUT_FILENO);
              close(out);
            }

            if(p_append) //Check for append
            {
              p_append = 0;
              out = open(redir_file, O_APPEND | O_WRONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
              dup2(out, STDOUT_FILENO);
              close(out);
            }
            execvp(argv_r[0], argv_r);
            exit(1);

          case -1: fprintf(stderr, "ERROR can't create child process\n");
            break;

          default:
            strcpy(kid_names[kid_count], argv_r[0]); //Grab process info for jobs command
            kid_pids[kid_count++] = pid;
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

      else if(redir) //Redirection
      { 
        redir = 0;
        switch(pid = fork())
        {
          case 0:
             out = open(redir_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR); //Open output file
             dup2(out, STDOUT_FILENO);
             close(out);
             execvp(argv_l[0], argv_l);
             exit(1);
             break;

          case -1: fprintf(stderr,"ERROR can't create child process!\n");
            break;

          default:
            strcpy(kid_names[kid_count], argv_l[0]); //Grab process info for jobs command
            kid_pids[kid_count++] = pid;
            if(!amp)
              {
              wait(NULL);
              }
            break;
        }
      }

      else if(append) //Append
      {
        append = 0;
        switch(pid = fork())
        {
          case 0:
             out = open(redir_file, O_APPEND | O_WRONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR); //Same as redirect, different permissions
             dup2(out, STDOUT_FILENO);
             close(out);
             execvp(argv_l[0] , argv_l);
             exit(1);
             break;

          case -1: fprintf(stderr,"ERROR can't create child process!\n");
            break;

          default:
            strcpy(kid_names[kid_count], argv_l[0]); //Grab process info
            kid_pids[kid_count++] = pid;
            if(!amp)
              {
              wait(NULL);
              }
            break;
        }
      }

      else if(r_true) //Read
      {
        r_true = 0;

        if(r_pipe)
        { //Read + Pipe
          r_pipe = 0;
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
              strcpy(kid_names[kid_count], argv_l[0]);
              kid_pids[kid_count++] = pid;
              break;
          }

          switch(pid = fork()) //Start second process
          {
            case 0: //Right process
              close(p[1]);
              dup2(p[0], STDIN_FILENO); //Reads from its pipe
              close(p[0]);
              if(r_redir)
              {
                out = open(redir_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                dup2(out, STDOUT_FILENO);
                close(out);
              }

              if(r_append)
              {
                out = open(redir_file, O_APPEND | O_WRONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                dup2(out, STDOUT_FILENO);
                close(out);
              }
              execvp(argv_r[0], argv_r);
              exit(1);

            case -1: fprintf(stderr, "ERROR can't create child process\n");
              break;

            default:
            strcpy(kid_names[kid_count], argv_r[0]);
              kid_pids[kid_count++] = pid;
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

        else{ //No pipe
          if(r_redir) //Check for redirect, same as before (really need to store these as functions)
          {
            switch(pid = fork())
            {
              case 0:
                in = open(read_file, O_RDONLY);
                out = open(redir_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR); 
                dup2(in, STDIN_FILENO);
                dup2(out, STDOUT_FILENO);
                close(in);
                close(out);
                execvp(argv_l[0] , argv_l);
                break;
              
              case -1: fprintf(stderr,"ERROR can't create child process!\n");
                break;

              default:
                strcpy(kid_names[kid_count], argv_l[0]);
                kid_pids[kid_count++] = pid;
                if(!amp)
                  {
                  wait(NULL);
                  }
                break;
            }
          }

          else if(r_append) //Check for append, same as before
          {
            switch(pid = fork())
            {
              case 0:
                in = open(read_file, O_RDONLY);
                out = open(redir_file, O_APPEND | O_WRONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                dup2(in, STDIN_FILENO);
                dup2(out, STDOUT_FILENO);
                close(in);
                close(out);
                execvp(argv_l[0] , argv_l);
                break;
              
              case -1: fprintf(stderr,"ERROR can't create child process!\n");
                break;

              default:
                strcpy(kid_names[kid_count], argv_l[0]);
                kid_pids[kid_count++] = pid;
                if(!amp)
                  {
                  wait(NULL);
                  }
                break;
            }
          }

          else{ //Just a standard read
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
                strcpy(kid_names[kid_count], argv_l[0]);
                kid_pids[kid_count++] = pid;
                if(!amp)
                  {
                  wait(NULL);
                  }
                break;
            }
          }
        }
      }

      else //Shell Built-Ins & Other defaults
      {
        if(!strcmp(v_line[0], "history") || (!strcmp(v_line[0], "HISTORY"))) //History command
        {
          for(int i = run_count; i < 30*loop; i++)
          {
            fprintf(stderr, "CMD %d: %s\n", i, history[i]);
          }

          for(int i = 0; i < run_count; i++)
          {
            fprintf(stderr, "CMD %d: %s\n", i+(30*loop), history[i]);
          }
        }

        else if(!strcmp(v_line[0], "exit") || (!strcmp(v_line[0], "EXIT"))) //Exit Command
        {
          for(int i = run_count; i < 30*loop; i++)
          {
            fprintf(stderr, "CMD %d: %s\n", i, history[i]);
          }

          for(int i = 0; i < run_count; i++)
          {
            fprintf(stderr, "CMD %d: %s\n", i+(30*loop), history[i]);
          }
          exit(1);
        }

        else if(!strcmp(v_line[0], "jobs") || (!strcmp(v_line[0], "JOBS"))) //Jobs Command
        {
          for(int i = 0; i < kid_count; i++){
            if(!waitpid(kid_pids[i],kid_status, WNOHANG))
            { 
              fprintf(stderr, "%s: %d\n", kid_names[i], kid_pids[i]);
            }
          }
        }

        else if(!strcmp(v_line[0], "cd")) //CD command
        {
          if(token_count == 1)
          {
            chdir(home_path);
          }
          else{
            if(chdir(v_line[1])){fprintf(stderr, "NO SUCH DIRECTORY!");}
          }
        }


        else //Just a normal process
        {
          for(int i = 0; i < token_count; i++){argv_l[i] = v_line[i];} //Assign left arguments
          argv_l[token_count] = NULL;
          switch(pid = fork()) 
          {
            case 0:
              execvp(argv_l[0], argv_l);
              exit(1);
            
            case -1: fprintf(stderr, "ERROR CAN'T CREATE CHILD PROCESS\n");
              break;

            default:
              strcpy(kid_names[kid_count], argv_l[0]);
              kid_pids[kid_count++] = pid;
              if(!amp)
              {
                wait(NULL);
              }
              break;
          }
        }
      } 

      end:
      fprintf(stderr,"%s",prompt);
      amp = 0;
      token_count = 0;
      for(int i = 0; i < 10; i++){ //Clear out argument tags to use again.
        argv_l[i] = NULL;
        argv_r[i] = NULL;
        strcpy(v_line[i], "");
      }
    }
  exit(0);
}


  
