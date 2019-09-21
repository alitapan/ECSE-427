//-------------------------------------------------------------------------------------------------------------------------//
//	ECSE 427
//  ASSIGNMENT 1
//  ULUC ALI TAPAN
//  260556540
//-------------------------------------------------------------------------------------------------------------------------//

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

//Global variables//
int counter = 0;
char *history[101];
int historyIndex = 0;
struct rlimit lim;

void intHandler(int sig);
void tstpHandler(int sig);

//history_funct: Tracks the user inputs and stores them in the global variables.
//mode = 0 --> store input
//mode = 1 --> load history
//-------------------------------------------------------------------------------------------------------------------------//
int history_funct(char *buffer, int mode)
{
	if(strcasecmp(buffer, "\n") == 0){
		return 0;
	}
	if(mode == 0)
	{
		//Store the user input into history array
		if(historyIndex<100)
		{
			history[historyIndex] = buffer; 
			historyIndex++;
		}
		//Wrap the history array if there are more than 100 elements
		else
		{
			for(int k=0; k < historyIndex-1; k++)
			{
				history[k] = history[k+1];
			}
			history[100] = buffer;
		}
	}
	else if(mode == 1)
	{
		for(int j = 0; j < historyIndex; j++)
    	{
    		printf("   %u", (j+1)); 
    		printf("  %s\n", history[j]);

    	}
	}
	return 0;

}

//get_a_line: Traces the user input and stores it into a string variable called "buffer".
//-------------------------------------------------------------------------------------------------------------------------//
char* get_a_line()
{ 

	//Initialize variables
	char *buffer = NULL;
	size_t n = 0;
	//Get the input
	getline(&buffer, &n, stdin);
	strtok(buffer, "\n");
	//Exit statement for the input
	if(strcasecmp(buffer, "exit") == 0) {
        exit(0);
    }

	return buffer;
}

//my_system: Takes the user input, stores it into history, tokenizes the string and stores the string into an array of 
//strings called "args[]". This array of strings is passed into the execvp() library call within the child process of the fork(). 
//This function also deals with the internal commands chdir, history and limit during the the main process.
//-------------------------------------------------------------------------------------------------------------------------//
int my_system(char *line) 
{
	//Store the input into history array
	history_funct(line, 0);
	//Tokenize the input line and store them into an array of strings, args, which is then passed on to execvp() function.
	char *token;
	char *rest = line; 
	char *args[sizeof(line)];
	int i = 0;

	while((token = strtok_r(rest, " ", &rest))){
		args[i] = token;
		i++;
	}

	//Check for internal commands:

	//INTERNAL COMMAND: chdir
	if(strcasecmp(args[0], "chdir") == 0 && i < 2) //If the user enters the internal command "chdir" and does not provide an argument
	{
        printf("Error: Please specify the directory you wish to switch!\n");

   		return 1;
    }
    else if(strcasecmp(args[0], "chdir") == 0 && i > 1)
    {
    	args[0] = args[1];
    	chdir(args[0]);

    	return 1;
    }

    //INTERNAL COMMAND: history
    if(strcasecmp(args[0], "history") == 0 && i > 1)
    {
    	printf("Error: Please do not provide multiple arguments with the history command!\n");
    	return 1;
    }
    else if(strcasecmp(args[0], "history") == 0 && i < 2)
    {
    	history_funct(line, 1);
    	return 1;
    }

    //INTERNAL COMMAND: limit
    if(strcasecmp(args[0], "limit") == 0 && i < 2)
    {	

    	getrlimit(RLIMIT_DATA, &lim);
    	printf("The current data soft limit is: %llu\n", lim.rlim_cur);
    	printf("The current data hard limit is: %llu\n", lim.rlim_max);
    	printf("To set a limit please type: limit <amount_of_mem>\n");
    	return 1;
    }
    else if(strcasecmp(args[0], "limit") == 0 && i < 3  && i > 1)
    {
    	if(atoi(args[1]) != 0)
    	{
    		int limit_input = atoi(args[1]);
    		lim.rlim_cur = limit_input;
    		setrlimit(RLIMIT_DATA, &lim);
  			printf("Limits set! To verify type: limit\n");
    	}
    	else
    	{
    		printf("Error: Invalid limit command! Please use an integer for setting limit!\n");
    	}

    	return 1;
    }
    else if(strcasecmp(args[0], "limit") == 0 && i > 2)
    {
    	printf("Error: Invalid limit command! please use the following format for limit: limit <amount_of_mem>\n");
    }


	pid_t pid;


    //Fork call
	pid = fork();

	if(pid<0)
	{ 
		//Fork error case
		fprintf(stderr, "Error: Fork Failed!");
		exit(EXIT_FAILURE);
		return 0;
	}
	else if(pid == 0)
	{ 
		//Child Process
		execvp(args[0], args);
    	exit(EXIT_SUCCESS);
      
		/*execvp(args[0], args);
		exit(EXIT_SUCCESS);	*/
	}
	else
	{ 
		//Parent Process
		wait(NULL);	
	}
		
	return 1;
}

//intHandler: Lets the user to exit the program using CTRL+C.
//-------------------------------------------------------------------------------------------------------------------------//
void intHandler(int sig)
{

	printf("\nInterrupt caught:%i\n", sig);
	printf("Would you like to exit the shell?\n");
	printf("Type exit to confirm otherwise press ENTER key to conintue.\n");
}

//tstpHandler: Ignores the CTRL+Z.
//-------------------------------------------------------------------------------------------------------------------------//
void tstpHandler(int sig)
{
	printf("\nInterrupt caught:%i\n", sig);
	printf("CTRL-Z is ignored in Assignment 1! Press ENTER key to continue.\n");
	printf("> ");
}

//The main function of the shell.
//-------------------------------------------------------------------------------------------------------------------------//
int main(int argc, char *argv[])
{

	
	while(1) 
	{
		printf("> ");
		signal(SIGINT, intHandler);
		signal(SIGTSTP, tstpHandler);
		char *line = get_a_line();
		if(sizeof(line) > 1)
			my_system(line);
	}
	return 0;
}

