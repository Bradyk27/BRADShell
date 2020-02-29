/*
TODOS:
Array of commands that is printed out on interrupt
SIGUSR1 Interrupt Handling
Built in shell commands
Argument parsing & handling
  - Function that determines what to run that is itself run in every process?
*/

/*
FOR FUNSIES:
Make this thing really powerful. This could be a cool project. Maybe even make a Windows CP Emulator from it.
*/


/*
Brady Kruse
bak225
Sources:
1) Vector Implementation: https://gist.github.com/EmilHernvall/953968/0fef1b1f826a8c3d8cfb74b2915f17d2944ec1d0
2) Redirect Skeleton Code: http://www.cs.loyola.edu/~jglenn/702/S2005/Examples/dup2.html
3) Mr. Ritter's "Babyshell"
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>
#include <stdbool.h>
#include "vector.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>


void vector_init(vector *v)
{
	v->data = NULL;
	v->size = 0;
	v->count = 0;
}

int vector_count(vector *v)
{
	return v->count;
}

void vector_add(vector *v, void *e)
{
	if (v->size == 0) {
		v->size = 10;
		v->data = malloc(sizeof(void*) * v->size);
		memset(v->data, '\0', sizeof(void) * v->size);
	}

	// condition to increase v->data:
	// last slot exhausted
	if (v->size == v->count) {
		v->size *= 2;
		v->data = realloc(v->data, sizeof(void*) * v->size);
	}

	v->data[v->count] = e;
	v->count++;
}

void vector_set(vector *v, int index, void *e)
{
	if (index >= v->count) {
		return;
	}

	v->data[index] = e;
}

void *vector_get(vector *v, int index)
{
	if (index >= v->count) {
		return NULL;
	}

	return v->data[index];
}

void vector_delete(vector *v, int index)
{
	if (index >= v->count) {
		return;
	}

	v->data[index] = NULL;

	int i, j;
	void **newarr = (void**)malloc(sizeof(void*) * v->count * 2);
	for (i = 0, j = 0; i < v->count; i++) {
		if (v->data[i] != NULL) {
			newarr[j] = v->data[i];
			j++;
		}		
	}

	free(v->data);

	v->data = newarr;
	v->count--;
}

void vector_free(vector *v)
{
	free(v->data);
}

vector history;

void handle_sig(int sig) //FIX: SIGUSR1, PRINTING OF VALUES ON EXIT
{
  if(sig == 2)
  {
    printf("\nCAUGHT CTRL + C\n");
    for (int i = 0; i < vector_count(&history); i++)
      {
        printf("%s\n", vector_get(&history, i));
      }
    exit(0);
  }

  if(sig == SIGUSR1)
  {
    printf("\nCAUGHT SIGUSR1\n");
    for (int i = 0; i < vector_count(&history); i++)
      {
        printf("%s\n", vector_get(&history, i));
      }
    exit(0);
  }
}

int main()
{

  signal(SIGINT, handle_sig);

  pid_t pid;
  char *prompt = "BRADv3> ";
  char line[256];
  int p[2];
  int bytes;

  vector_init(&history); //FIX: Rewrites all values...for some odd reason.
  vector v_line;
  vector_init(&v_line);

  int in, out;

  fprintf(stderr,"%s",prompt);
  while ( scanf ("%[^\n]%*c",line) != EOF )
    {
      vector_add(&history, line);

      vector_free(&v_line);
      vector_init(&v_line);
      vector_add(&v_line, strtok(line, " ,."));
      char * token = "";
      while(token != NULL)
      {
        token = strtok(NULL, " ,.");
        vector_add(&v_line, token);
      }

      //For testing purposes
      char *test_ls_arg[] = {"ls", NULL};
      char *test_wc_arg[] = {"wc", "-l", NULL};
      char *filename = "testfile";
      
      if(pipe(p) < 0)
      { 
        exit(1);
      }

      if(0) //WORKING pipe code
      {
        pipe(p); //One pipe, parent controls between the two

        switch(pid=fork()) //1st child
        {
          case 0: //ls process
            close(p[0]); //no nead to read, just writes to his pipe.
            dup2(p[1], STDOUT_FILENO);
            close(p[1]);
            fprintf(stderr, "ABOUT TO RUN LS!\n");
            execvp("ls" , test_ls_arg);
            exit(1);

          case -1: fprintf(stderr, "ERROR CAN'T CREATE CHILD PROCESS\n");
            break;

          default:
            break;
        }

        switch(pid = fork()) //Start second process
        {
          case 0: //wc -l
            close(p[1]);
            dup2(p[0], STDIN_FILENO); //Reads from his pipe
            close(p[0]);
            fprintf(stderr, "ABOUT TO RUN WC -L!\n");
            execvp("wc", test_wc_arg);
            exit(1);

          case -1: fprintf(stderr, "ERROR can't create child process\n");
            break;

          default:
            break;
        }
        
        close(p[0]);
        close(p[1]);
        wait(NULL);
        wait(NULL);
      }


      if(0) //WORKING redirect code
      {
        switch(pid = fork())
        {
          case 0:
             out = open("test", O_WRONLY | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR); //check permissions
             dup2(out, STDOUT_FILENO);
             close(out);
             execvp("ls", test_ls_arg);
             exit(1);
             break;

          case -1: fprintf(stderr,"ERROR can't create child process!\n");
            break;

          default:
            wait(NULL);
            break;
        }
      }

      if(0) //WORKING read code
      {
        switch(pid = fork())
        {
          case 0:
             in = open("test", O_RDONLY); //Check permissions
             dup2(in, STDIN_FILENO);
             close(in);
             execvp("wc", test_wc_arg);
             exit(1);
             break;

          case -1: fprintf(stderr,"ERROR can't create child process!\n");
            break;

          default:
            wait(NULL);
            break;
        }
      }

      if(0) //WORKING append code
      {
        switch(pid = fork())
        {
          case 0:
             out = open("test", O_APPEND | O_WRONLY, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR); //Check permisions
             dup2(out, STDOUT_FILENO);
             close(out);
             execvp("ls" , test_ls_arg);
             exit(1);
             break;

          case -1: fprintf(stderr,"ERROR can't create child process!\n");
            break;

          default:
            wait(NULL);
            break;
        }
      }

      if(0) //WORKING redirect AND read code
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
            execvp("wc" , test_wc_arg);
            break;
          
          case -1: fprintf(stderr,"ERROR can't create child process!\n");
            break;

          default:
            wait(NULL);
            break;
        }
      }

      if(0) //WORKING system commands (built-in)
      {
        system("env");
      }

      if(1) //SKELETON history command
      {
       for (int i = 0; i < vector_count(&history); i++)
        {
		      printf("%s\n", vector_get(&history, i));
	      } 
      }


      fprintf(stderr,"%s",prompt); 
    }
  exit(0);
}


  
