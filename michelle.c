#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/types.h>



typedef struct Command {
  char *name;				  //string for the command
	char **args;		    //array of pointers to strings with command args
	int argC;

  int inFromFile;     //replace stdin FILE stream  with inputFile
  char *inputFile;    //containts filename to draw input from

  int redOutRep;			//redirect output and replace file if existing (>)
	int redOutApp;			//redirect output and append file if existing (>>)
	char *outputFile;	  //contains filename if it is to be redirected
  
	int isPipeline;			//self explanatory

  int runInBackground;

} Command;


//Function Prototypes
void parseCommand(char *tokens[], int numTokens);
Command *initCommand();
void displayCommand(Command *thisCommand);
void myExit(); //HAD TO CHANGE NAME SINCE unistd.h HAS ITS OWN exit METHOD
void cd(char **args, int argc);
void clr();
void dir(char *directory);
void environment(char *varName); //HAD TO CHANGE NAME SINCE unistd.h HAS ITS OWN environ METHOD
void echo(char **args, int argc);
void help();
void myPause();   //HAD TO CHANGE NAME SINCE unistd.h HAS ITS OWN PAUSE METHOD
void path();


int main(int argc, char *argv[], char *envp[]) {

  char *path = getenv("PATH");
  strncat(path, "/myshell", 8);

  setenv("shell", path, 1);


  int batchMode = 0;
  FILE *fp;

  if (argc == 2) { //has batch file
    batchMode = 1;
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
      char error_message[30] = "An error has occurred\n";
      write(STDERR_FILENO, error_message, strlen(error_message));   
      exit(1);
    } 
  } else if (argc > 2) {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
  }

  // for (int i = 0; envp[i] != NULL; i++){    
  //   printf("\n%s", envp[i]);
  // }


  size_t bufsize = 32;
  char *buffer = malloc(bufsize * sizeof(char));
  char *savePtr;


  //main driver loop
  while(1) {              

    if(batchMode) { //batch mode
      if(!feof(fp)) {
        fgets(buffer, 32, fp);
      }
    } else {  //normal mode
      char cwd[100];
      printf("%s-michelle> ", getcwd(cwd,100));  //get line from user
      getline(&buffer, &bufsize, stdin);

      if(!strcmp(buffer, "\n")){continue;}
    }


    //tokenize line into command
    char **strings = (char **)malloc(100 * sizeof(char *));
    char *commandString;
    int stringCount = 0;

    commandString = strtok_r(buffer, " \t\n", &savePtr);
    while (commandString != NULL) {
      strings[stringCount] = (char *)malloc(100 * sizeof(char));
      strcpy(strings[stringCount], commandString);
      //printf("\t added token: %s at index %d\n", commandString, stringCount);
      commandString = strtok_r(NULL, " \t\n", &savePtr);
      stringCount++;
    }

    parseCommand(strings, stringCount);

    // strings[stringCount] = NULL;     //TRY UNCOMMENTING THIS TO SEE IF ERROR
    for (int i = 0; i < stringCount; i++) {
      free(strings[i]);
    }
    free(strings);

    if(batchMode && feof(fp)) {break;}

  } //close driver loop
  if(batchMode){fclose(fp);}
} //close main


//PARSER FUNCTION
void parseCommand(char **tokens, int numTokens){
  
  Command **commands = malloc(sizeof(Command*));
  int sizeCommandsArray = 1;

  Command *thisCommand;
  int numCommands = 0;
  int initNewCommand = 1; //used to check if needing new command struct after & or |

  char *thisToken;

  for (int i = 0; i < numTokens; i++) {
    thisToken = tokens[i];

    if(initNewCommand) { // initialize new command and add to commands array

      thisCommand = initCommand();
      thisCommand->name = thisToken;

      if (numCommands >= sizeCommandsArray) { //realloc commands array
        sizeCommandsArray *= 2;
        commands = (Command **)realloc(commands, sizeCommandsArray * sizeof(Command));
      }

      commands[numCommands] = thisCommand;
      numCommands++;

      initNewCommand = 0;
      //continue;
    }

    
    if (!strcmp(thisToken, ">")) {
		  thisCommand->redOutRep = 1;
      strcpy(thisCommand->outputFile, tokens[++i]);

    } else if (!strcmp(thisToken, ">>")) {
      thisCommand->redOutApp = 1;
      strcpy(thisCommand->outputFile, tokens[++i]);
      
    } else if (!strcmp(thisToken, "<")) {
      thisCommand->inFromFile = 1;
      strcpy(thisCommand->inputFile, tokens[++i]);
      
    } else if (!strcmp(thisToken, "|")) {
      thisCommand->args[thisCommand->argC] = NULL;  //all args for this command are read in, so add NULL to end array for execvp
      thisCommand->isPipeline = 1;
      initNewCommand = 1;

    } else if (!strcmp(thisToken, "&")) {
      thisCommand->args[thisCommand->argC] = NULL;//all args for this command are read in, so add NULL to end array for execvp
      thisCommand->runInBackground = 1;
      initNewCommand = 1;

    } else {
      thisCommand->args[thisCommand->argC] = (char *)malloc(100 * sizeof(char));
      strcpy(thisCommand->args[thisCommand->argC], thisToken);
      thisCommand->argC++;
    }

  }

  //Null terminate args array for last command
  thisCommand->args[thisCommand->argC] = NULL;  

  // DISPLAY ALL COMMANDS
  // for(int i = 0; i < numCommands; i++) {
  //   printf("\nCommand %d: \n", i+1);
  //   displayCommand(commands[i]);
  // }

  
  for(int i = 0; i < numCommands; i++) {
    thisCommand = commands[i];

    //check for built in functions
    if (!strcmp(thisCommand->name, "exit")) {
      for (int i = 0; i < numCommands; i++) {
        free(commands[i]);
      }
      free(commands);
      myExit();

    } else if (!strcmp(thisCommand->name, "cd")) {
      cd(thisCommand->args, thisCommand->argC);
      continue;

    } else if (!strcmp(thisCommand->name, "clr")) {
      clr();
      continue;

    } else if (!strcmp(thisCommand->name, "dir")) {
      if (thisCommand->argC == 1) { //if no arguments, print current directory
        dir(".");
      } else {    //else print all directories mentioned
        for (int i = 1; i < thisCommand->argC; i++) {
          printf("\nDirectory <%s>\n", thisCommand->args[i]);
          dir(thisCommand->args[i]);
        }
      }
      continue;

    } else if (!strcmp(thisCommand->name, "environ")) {
      if (thisCommand->argC == 2) {
        environment(thisCommand->args[1]);
      } else {
        environment("noneSpecified"); //when no environ variables are specified, the function prints the "more important ones"
      }
      continue;

    } else if (!strcmp(thisCommand->name, "echo")) {
      echo(thisCommand->args, thisCommand->argC);
      continue;

    } else if (!strcmp(thisCommand->name, "help")) {
      help();
      continue;

    } else if (!strcmp(thisCommand->name, "pause")) {
      myPause();
      continue;
      
    } else if (!strcmp(thisCommand->name, "path")) {
      path();
      continue;

    } else {    //IF NOT BUILT IN COMMAND

      if (thisCommand->isPipeline) { //pipeline
      /* 
       * PIPE LOGIC:
       *  Main process is shell
       *  Pipeline requires 2 processes to exec on, which means 2 children
       *  Main process forks, that child1 becomes the Parent of the pipe
       *  Parent(child1) of the pipe forks, child2 executes first command and handles input of pipe, Parent(child1) executes second command and handles output of pipe
       */
        pid_t pipe_pid;
        int pipe_status;

        if((pipe_pid = fork()) == -1) {   //Create child to host pipeline
          perror("fork");
          exit(1);
        } else if (pipe_pid == 0){        //if child, create pipe
          pid_t child_pid;
          
          int fd[2];
          pipe(fd);

          if((child_pid = fork()) == -1) {  //create child for first command, parent will be second command
            perror("fork");
            exit(1);
          } else if (child_pid == 0) {  //child
            dup2(fd[1], 1); //writes to the pipe
            close(fd[0]);
            close(fd[1]);
            
            if(thisCommand->inFromFile) { //child opens input
              freopen(thisCommand->inputFile, "r", stdin);
            }
          
            if(execvp(thisCommand->name, thisCommand->args) < 0) {       //execute, if returns -1 then failed, report error
              char error_message[30] = "An error has occurred\n";
              write(STDERR_FILENO, error_message, strlen(error_message));
              exit(1);
            }
          } else {  //pipe parent to be second command (recall this parent process is also child of shell process)
            thisCommand = commands[++i]; //load next command
            dup2(fd[0], 0); //reads from the pipe
            close(fd[0]);
            close(fd[1]);

            if(thisCommand->redOutRep) {  //parent opens output
              freopen(thisCommand->outputFile, "w+", stdout);
            } else if(thisCommand->redOutApp) {
              freopen(thisCommand->outputFile, "a", stdout);
            }

            if(execvp(thisCommand->name, thisCommand->args) < 0) {     //execute, if returns -1 then failed, report error
              char error_message[30] = "An error has occurred\n";
              write(STDERR_FILENO, error_message, strlen(error_message));
              exit(1);
            }
          }
          
        } else {  //big parent waits for pipe to complete
          waitpid(pipe_pid, &pipe_status, 0);
        }

      } else {  //not pipeline
        pid_t child_pid;
        int status; 

        if((child_pid = fork()) == -1) {
          perror("fork");
          exit(1);
        }
        if(child_pid == 0) {  //child
          if(thisCommand->inFromFile) { //opens input and output files, if any are needed, and sets stdin/stdout accordingly
            freopen(thisCommand->inputFile, "r", stdin);
          }
          if(thisCommand->redOutRep) {
            freopen(thisCommand->outputFile, "w+", stdout);
          }
          if(thisCommand->redOutApp) {
            freopen(thisCommand->outputFile, "a", stdout);
          }
          if(execvp(thisCommand->name, thisCommand->args) < 0) {   //execute, if returns -1 then failed, report error
            char error_message[30] = "An error has occurred\n";
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
          }
        } else {  //parent
          if(!thisCommand->runInBackground){ //only wait if runInBackground is true
            waitpid(child_pid, &status, 0);
          }
        }

      } //close pipeline / not pipeline execution
    } //close not built in command 
  } //close for loop
  return;
}






Command *initCommand(){
  Command *thisCommand = (Command *)malloc(sizeof(Command));

  thisCommand->name = malloc(100*sizeof(char));
  thisCommand->argC = 0;
  thisCommand->args = (char **)malloc(100 * sizeof(char *));
  thisCommand->inFromFile = 0;
  thisCommand->inputFile = (char *)malloc(100 * sizeof(char));
  thisCommand->redOutRep = 0;
	thisCommand->redOutApp = 0;
  thisCommand->outputFile = (char *)malloc(100 * sizeof(char));
	thisCommand->isPipeline = 0;
  thisCommand->runInBackground = 0;

  return thisCommand;
}

void displayCommand(Command *thisCommand) {
  printf("\tName:              | \"%s\"\n",         thisCommand->name);
  printf("\tArgument Count:    | %d\n",             thisCommand->argC);
  for(int i = 0; i < thisCommand->argC; i++) {
    printf("\t\tArgument %d:    | \"%s\"\n", i+1,   thisCommand->args[i]);
  }
  printf("\tTakes Input:       | %d\n",             thisCommand->inFromFile);
  printf("\tInput File Name:   | \"%s\"\n",         thisCommand->inputFile);
  printf("\tRedOutRep:         | %d\n",             thisCommand->redOutRep);
  printf("\tRedOutApp:         | %d\n",             thisCommand->redOutApp);
  printf("\tOutput File Name:  | \"%s\"\n",         thisCommand->outputFile);
	printf("\tIs Pipeline:       | %d\n",             thisCommand->isPipeline);
  printf("\tRuns In Background:| %d\n",             thisCommand->runInBackground);
}



//BUILT-IN FUNCTIONS

void myExit() {   //DONE
  exit(0);
}

void cd(char **args, int argc) {    //DONE
  if ((argc-1) > 1) {      //argc-1 since count includes command name
    //printf("error\n");
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
  } else if (argc == 1){  //if user doesn't specify a directory, print current directory
    dir(".");
  } else {
    chdir(args[1]);
  }
}

void path() { //DONE
  printf("%s\n", getenv("PATH"));
}

void clr() {  //DONE
  printf("\033[H\033[2J");
}

void dir(char *directory) { //DONE
  struct dirent *fileName;
  DIR *dp;

  dp = opendir(directory);
  if (dp == NULL) {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
  }
  while((fileName = readdir(dp))) {
    printf("%s\n", fileName->d_name);
  }
  closedir(dp);
}


void environment(char *varName) { //DONE
  if (!strcmp(varName, "noneSpecified")) {  //if no varName specified
    printf("USER:%s\n", getenv("USER"));
    printf("PATH:%s\n", getenv("PATH"));
    printf("HOSTNAME:%s\n", getenv("HOSTNAME"));
    printf("HOME:%s\n", getenv("HOME"));
  } else {  //if varName is specified, only show that variable
    printf("%s:%s\n", varName, getenv(varName));
  }
}

void echo(char **args, int argc) {  //DONE
  for(int i = 1; i < argc; i++) {   //start at 1 since index 0 contains echo
    printf("%s ", args[i]);
  }
  printf("\n");
}

void help() { //DONE
  char *catArgs[2] = {"cat", "readme"};
  int retval = fork();
  int status;
  if (retval == 0) {
    execvp(catArgs[0], catArgs);
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
    exit(1);
  } else {
    int child_pid = retval;
    waitpid(child_pid, &status, 0);
  }
}

void myPause() {  //DONE
  getchar();
}
