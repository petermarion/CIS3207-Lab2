#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>     //suggested to add
// extern char *environ[];



char *PATH = "/bin";  //is this right??
/*
The shell environment should contain shell=<pathname>/myshell where <pathname>/myshell is the full path for the shell executable(not a hardwired path back to your directory, but the one from which it was executed)
*/

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
  int nextInPipe;

  int runInBackground;

} Command;


//Function Prototypes
void parseCommand(char *tokens[], int numTokens);
Command *initCommand();
void displayCommand(Command *thisCommand);
void myExit();
void cd(char **args, int argc);
void clr();
void dir(char *directory);
void environ(char *varName);
void echo(char **args, int argc);
void help();
void myPause();   //HAD TO CHANGE NAME SINCE unistd.h HAS ITS OWN PAUSE METHOD
void path();


int main(void) {


  // for(int i = 0; i < sizeof(environ); i++) {
  //   printf("Environment variables: %s\n", environ[i]);
  // }

  //init vars
  int sizeCommandsArray = 1;
  char **commands = (char **)malloc(sizeCommandsArray * sizeof(char *));  //pointers to any concurrent commands
  

  size_t bufsize = 32;
  char *buffer = malloc(bufsize * sizeof(char));
  char *commandString = malloc(bufsize * sizeof(char));
  char *savePtr;


  //main driver loop
  while(1) {              

    //get line from user
    printf("myshell> ");  
    getline(&buffer, &bufsize, stdin);

    if(!strcmp(buffer, "\n")){continue;}

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

  }
}


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
      thisCommand->args[thisCommand->argC + 1] = NULL;  //all args for this command are read in, so add NULL to end for execvp
      thisCommand->isPipeline = 1;
      initNewCommand = 1;

    } else if (!strcmp(thisToken, "&")) {
      thisCommand->args[thisCommand->argC + 1] = NULL;//all args for this command are read in, so add NULL to end for execvp
      thisCommand->runInBackground = 1;
      initNewCommand = 1;

    } else {
      // printf("new arg\n");
      fflush(stdout);
      thisCommand->args[thisCommand->argC] = (char *)malloc(100 * sizeof(char));
      strcpy(thisCommand->args[thisCommand->argC], thisToken);
      thisCommand->argC++;
    }

  }

  //Null terminate args array
  thisCommand->args[thisCommand->argC] = NULL;  

  //DISPLAY ALL COMMANDS
  // for(int i = 0; i < numCommands; i++) {
  //   printf("\nCommand %d: \n", i+1);
  //   displayCommand(commands[i]);
  // }

  
  for(int i = 0; i < numCommands; i++) {
    thisCommand = commands[i];

    //check for built in functions
    if (!strcmp(thisCommand->name, "exit")) {
      myExit();
      continue;
    } else if (!strcmp(thisCommand->name, "cd")) {
      cd(thisCommand->args, thisCommand->argC);
      continue;
    } else if (!strcmp(thisCommand->name, "clr")) {
      clr();
      continue;
    } else if (!strcmp(thisCommand->name, "dir")) {
      dir(thisCommand->args[1]);
      continue;
    } else if (!strcmp(thisCommand->name, "environ")) {
      environ(thisCommand->args[1]);
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
    }

    //if is pipeline
    //if run in background

    //use execvp(char* path, char* argv[]); to execute commands

  }






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
  thisCommand->nextInPipe = 0;
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

void cd(char **args, int argc) {    //UNTESTED
  if ((argc-1) > 2 || argc == 0) {      //argc-1 since count includes command name
    printf("error\n");
  } else {
    printf("IMPLEMENT DIRECTORY CHANGE\n");
    //chdir(args[0]);
  }
}

void path() {
  printf("Called path function\n");
}

void clr() {  //DONE
  printf("\033[H\033[2J");
}

void dir(char *directory) {
  printf("Called dir function\n");
}

void environ(char *varName) {
  printf("Called environ function\n");
  // if (!strcmp(varName, "PATH")) {
  //   printf("PATH: %s\n", PATH);
  // } else if (!strcmp(varName, xxx)) {
  //   printf("xxx: %s\n", xxx);
  // }
}

void echo(char **args, int argc) {  //DONE
  for(int i = 1; i < argc; i++) {   //start at 1 since index 0 contains echo
    printf("%s ", args[i]);
  }
  printf("\n");
}

void help() {
  printf("Implement help function\n");
}

void myPause() {  //DONE
  getchar();
}
