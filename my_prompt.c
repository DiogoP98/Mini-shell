/////////////////////////////////////////////////////////
//                                                     //
//               Work I: Mini-Shell                    //
//                                                     //
// Compilation: gcc my_prompt.c -lreadline -o my_prompt//
// Execution: ./my_prompt                              //
//                                                     //
/////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

#define MAXARGS 100
#define PIPE_READ 0
#define PIPE_WRITE 1

typedef struct command {
    char *cmd;              // string with command
    int argc;               // number of arguments
    char *argv[MAXARGS+1];  // vector with command arguments
    struct command *next;   // pointer to next command
} COMMAND;

// global variables
char* inputfile = NULL;	    // name of file (in case of redirection of the standard input)
char* outputfile = NULL;    // name of file (in case of redirection of the standard output)
int background_exec = 0;    // indicates if a program is running concurrently with shell (0/1)
int numberpipes = 0;

// funcitons
COMMAND* parse(char *);
void print_parse(COMMAND *);
void execute_commands(COMMAND *);
void free_commlist(COMMAND *);
void execute_filtro(COMMAND *);

// include do código do parser da linha de comandos
#include "parser.c"

int main(int argc, char* argv[]) {
  char *linha;
  COMMAND *com;

  while (1) {
    if ((linha = readline("my_prompt$ ")) == NULL)
      exit(0);
    if (strlen(linha) != 0) {
      add_history(linha);
      com = parse(linha);
      if (com) {
        if (strcmp (com->cmd,"exit")==0)
            break;
      	print_parse(com);
      	execute_commands(com);
      	free_commlist(com);
      }
    }
    free(linha);
  }
}


void print_parse(COMMAND* commlist) {
  int n, i;

  printf("---------------------------------------------------------\n");
  printf("BG: %d IN: %s OUT: %s\n", background_exec, inputfile, outputfile);
  n = 1;
  while (commlist != NULL) {
    printf("#%d: cmd '%s' argc '%d' argv[] '", n, commlist->cmd, commlist->argc);
    i = 0;
    while (commlist->argv[i] != NULL) {
      printf("%s,", commlist->argv[i]);
      i++;
    }
    printf("%s'\n", commlist->argv[i]);
    commlist = commlist->next;
    numberpipes++;
    n++;
  }
  printf("---------------------------------------------------------\n");
}


void free_commlist(COMMAND *commlist){ //free allocated memory
  COMMAND *x;
  while (commlist!=NULL) {
    x=commlist->next;
    free(commlist);
    commlist = x;
  }
}


void execute_commands(COMMAND *commlist) {
  int fd[numberpipes][2];

  int i=0;
  numberpipes--;

  while (commlist != NULL) {
    pipe(fd[i]);

    pid_t pid = fork();

    if (pid<0){ 
      perror("Error: ");
      return;
    }

    if (pid==0) {

      if (strcmp (commlist->cmd,"filtro") == 0) //checks if it is a command implemented by user
        execute_filtro(commlist);

      if (strcmp (commlist->argv[commlist->argc-1],"&") == 0){
        commlist->argv[commlist->argc-1] = NULL;
        background_exec = 1;
      }


      if (inputfile!=NULL && i==0) {
        int in = open (inputfile, O_RDWR);

        if (in<0){
          perror("Error: ");
          return; 
        }

        dup2(in,STDIN_FILENO);
        close(in);
      }

      if (outputfile != NULL && i == numberpipes) {
        int out = open (outputfile, O_WRONLY | O_CREAT | O_TRUNC);

        if (out<0){
          perror("Error: "); 
          return;
        }

        dup2(out,STDOUT_FILENO);
        close(out);
      }

      if (i > 0) {
        dup2 (fd[i-1][0],STDIN_FILENO); //get input from previous pipe
        close (fd[i-1][0]);
      }

      if (i<numberpipes) {
        dup2 (fd[i][1], STDOUT_FILENO); //send output to next pipe
      }

      if (execvp(commlist->cmd,commlist->argv)<0){ //execute command
        perror ("Error: ");
        return;
      }

    }

    else {
      if (i<numberpipes)
        close (fd[i][1]);
      
      if (background_exec==0)
        wait(NULL);
    }

    i++;
    commlist = commlist->next;
  }
} 

void execute_filtro (COMMAND *commlist) {
  int fd[2];
  pid_t pid;
  pipe(fd);

  pid = fork();


  if (pid <0) {
    perror ("Error: ");
    return;
  }

  if (pid == 0){
    close (fd[1]);

    int out = open (commlist->argv[2], O_RDWR | O_CREAT, O_APPEND);

    if (out < 0) {
      perror ("Error: ");
      return;
    }

    dup2 (out,STDOUT_FILENO);
    close(out);

    dup2 (fd[0], STDIN_FILENO);

    if (execlp ("grep","grep",commlist->argv[3],NULL)<0) {
      perror ("Error: ");
      return;
    }

  }

  else {
    close (fd[0]);

    int in = open (commlist->argv[1], O_RDONLY);

    dup2 (in, STDIN_FILENO);
    close(in);

    dup2 (fd[1],STDOUT_FILENO);

    if (execlp ("cat","cat",commlist->argv[1],NULL)<0) {
      perror ("Error: ");
      return;
    }

    wait(NULL);

  }

}


