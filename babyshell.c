#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* T. Ritter 2/19/2014 - Poor style but simple*/

char *prompt="myshell> ";

main()
{
  int pid;
  char line[256];

    fprintf(stderr,"%s",prompt);
    while ( scanf ("%s",line) != EOF )
    {
    switch(pid=fork())
    {
      case 0:  execlp(line,line, (char *) NULL);   /* child */
               fprintf(stderr,"ERROR %s no such program\n",line);
               exit(1);
               break;
      case -1: fprintf(stderr,"ERROR can't create child process!\n");
               break;
      default: wait((int *) NULL);   /* parent incorrect wait seen notes */
               break;
     }
    fprintf(stderr,"%s",prompt);
    }
    exit(0);
} /* end main */

