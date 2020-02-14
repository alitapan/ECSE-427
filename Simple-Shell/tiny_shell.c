//-------------------------------------------------------------------------------------------------------------------------//
//  ECSE 427
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
#include <termios.h>
#include <errno.h>
#include <sys/stat.h>

//Global variables//
char *history[101];
int historyIndex = 0;
int signalFlag = 0;
char *argvCopy[100];
//struct rlimit lim;

void intHandler(int sig);
void tstpHandler(int sig);

//history_funct: Tracks the user inputs and stores them in the global variable history[].
//mode = 0 --> store input
//mode = 1 --> load history
//-------------------------------------------------------------------------------------------------------------------------//
int history_funct(char *buffer[], int numberOfInputs, int mode)
{
	char store[101];
	strcpy(store, " ");

	if(mode == 0)
	{
		if(numberOfInputs >= 0)
		{
			for(int i = 0; i<numberOfInputs; i++) //Merge all the arguments of the input into a single string for easier storage.
			{	
				strcat(store, " ");
				strcat(store, buffer[i]);
			}
		}
		//Stores the history
		if(historyIndex < 100)
		{
			history[historyIndex] = strdup(store);
			historyIndex++;
		}
		//Wraps the history if the entires exceed 100
		else
		{
			for(int j = 1; j < historyIndex; j++)
			{
				history[j - 1] = strdup(history[j]);
			}
			history[historyIndex-1] = strdup(store);
		}
	}
	//Prints the history
	else if(mode == 1)
	{ 
		for(int k = 0; historyIndex > k; k++)
		{
			printf("  %u", k+1);
			printf("    %s\n", history[k]);
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
		if(feof(stdin))
	{
		exit(0);
	}
	strtok(buffer, "\n");

	//Exit statement for the input
	if(strcasecmp(buffer, "exit") == 0) {
        exit(0);
    }

    //If the user presses CTRL-C, for more information see the intHandler() function below.
    if(signalFlag == 1)
    {
    	if(strcasecmp(buffer, "y") == 0 || strcasecmp(buffer, "Y") == 0)
    	{
    		exit(0);
    	}
    	else
    	{
    		signalFlag = 0;
    	}
    }
	return buffer;
}

//fork_system: Standard fork() implementation. The child executes the arguments while the parent waits 
//for the child to complete the process.
//-------------------------------------------------------------------------------------------------------------------------//
int fork_system(char ** args)
{
	//Fork call
    pid_t pid;
	pid = fork();

	if(pid<0)
	{ 
		//Fork error case
		fprintf(stderr, "Error: Main Fork Failed!\n");
		exit(EXIT_FAILURE);
		return 0;
	}
	else if(pid == 0)
	{ 
		//Child Process
		 if (execvp(args[0], args) == -1)
          	 _exit(EXIT_FAILURE);
         else
         	exit(EXIT_SUCCESS);
	}
	else
	{ 
		//Parent Process
		waitpid(pid,NULL,0);
			
	}
	return 1;
}

//pipe_system: Named pipe implementation with a double fork().
//-------------------------------------------------------------------------------------------------------------------------//

int pipe_system(char **args, int numberOfInputs, int numberOfinputsUntilPipe, char *argv[])
{ 
	//FIFO initialization 
	char *fifoPath = argv[1];
	mkfifo(fifoPath, 0777); //Just in case if the user did not create the fifo

	//Initialize variables
	int fd;
	pid_t pid1, pid2;

	int counter = 0;
	int args1_counter = 0;
	int args2_counter = 0;
	int flag = 0;

	//Seperating the inputs of the pipe into 2 arrays which will be passed into the execvp of the fork
	char *args1[sizeof(args)];
	char *args2[sizeof(args)];

	//Populate the arrays with the user inputs with this while loop
	while(counter < numberOfInputs)
	{
		if(strcasecmp(args[counter], "|") == 0)
		{
			
			flag = 1;
			counter ++;
		}
		else 
		{
			if(flag == 0)
			{
				args1[counter] = strdup(args[counter]);
				args1_counter++;
				counter++;
			}
			else
			{
				args2[args2_counter] = strdup(args[counter]);
				counter++;
				args2_counter++;
			}
		}
	}

	//The last arguement in the string has to be NULL
	args1[args1_counter] = NULL;
	args2[args2_counter] = NULL;

	pid1 = fork();

	if(pid1 < 0)
	{ 
		//Fork error case 1
		fprintf(stderr, "Error: Pipe Fork Failed!\n");
		exit(EXIT_FAILURE);
		return 0;
	}
	else if(pid1 == 0)
	{ 
		//Child Process 1
		fclose(stdout);
		fd = open(fifoPath, O_WRONLY);
		dup2(fd, STDOUT_FILENO);
		if (execvp(args1[0], args1) == -1)
          	_exit(EXIT_FAILURE);
        close(fd);
	}
	else
	{ 
		//Parent Process 1
		pid2 = fork();

		if(pid2 < 0)
		{ 
			//Fork error case 2
			fprintf(stderr, "Error: Pipe Fork Failed!\n");
			exit(EXIT_FAILURE);
			return 0;
		}
		else if(pid2 == 0)
		{
			//Child Process 2
			fd = open(fifoPath, O_RDONLY);
			dup2(fd, STDIN_FILENO);
			waitpid(pid1,NULL,0);
			if (execvp(args2[0], args2) == -1)
          		_exit(EXIT_FAILURE);
        	close(fd);
		}
		else
		{
			//Parent Process 2
			waitpid(pid2, NULL, 0);
		}

	}
	return 1;
}

//my_system: Takes the user input, tokenizes the string and stores the string into an array of strings called "args[]". 
//This array of strings is passed into the fork_system for execution. If the user commands for a pipe, the
//array of strings are passed into pipe_system for execution. This function also deals with the internal commands chdir, history and limit.
//-------------------------------------------------------------------------------------------------------------------------//
int my_system(char *line, char *argv[]) 
{
	struct rlimit old_lim, new_lim;
	getrlimit(RLIMIT_DATA, &old_lim);
	//Store the input into history array
	//history_funct(line, 0);
	//Tokenize the input line and store them into an array of strings: args[], which is then passed in to the fork to be executed.
	char *token;
	char *args[sizeof(line)+2];
	char *rest = strdup(line); 
	int systemCounter = 0;
	int i = 0; //Represents the number of strings seperated in the input
	while((token = strtok_r(rest, " ", &rest)))
	{
		args[i] = strdup(token);
		i++;
	}

	history_funct(args, i, 0); //Store the tokenized input into history

	//Check for internal commands:
	//INTERNAL COMMAND: chdir AND cd
	if(strcasecmp(args[0], "chdir") == 0 && i < 2) //If the user enters the internal command "chdir" and does not provide an argument
	{
        chdir(getenv("HOME"));
   		return 1;
    }
    else if(strcasecmp(args[0], "chdir") == 0 && i > 1)
    {
    	args[0] = args[1];
    	chdir(args[0]);

    	return 1;
    }

    if(strcasecmp(args[0], "cd") == 0 && i < 2) //If the user enters the internal command "cd" and does not provide an argument
	{
		chdir(getenv("HOME"));
   		return 1;
    }
    else if(strcasecmp(args[0], "cd") == 0 && i > 1)
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
    	history_funct(args, -1, 1);
    	return 1;
    }

    //INTERNAL COMMAND: limit
    if(strcasecmp(args[0], "limit") == 0 && i < 2)
    {	
    	getrlimit(RLIMIT_DATA, &old_lim);
    	printf("The current data soft limit is: %lu\n", old_lim.rlim_cur);
    	printf("The current data hard limit is: %lu\n", old_lim.rlim_max);
    	printf("To set a resource limit please type: limit <amount_of_mem>\n");
    	return 1;
    }
    else if(strcasecmp(args[0], "limit") == 0 && i < 3  && i > 1)
    {
    	if(atoi(args[1]) != 0)
    	{
    		getrlimit(RLIMIT_DATA, &old_lim); //Get limit before setting
    		new_lim.rlim_cur = atoi(args[1]);
    		new_lim.rlim_max = old_lim.rlim_max; //Hard limit stays the same
    		//int limit_input = atoi(args[1]);
    		//lim.rlim_cur = limit_input;
    		if(setrlimit(RLIMIT_DATA, &new_lim) == -1)
    		{
    			printf("Error: Could not set limit!\n");
    		} 
  			//printf("Limits set! To verify type: limit\n");
  			int fd[2];
  			if (pipe(fd) == -1)
  			{
  				printf("Error: Pipe() error for limit\n");
  			}
    	}
    	else
    	{
    		printf("Error: Invalid limit command! Please use an integer for setting resource limit!\n");
    	}

    	return 1;
    }
    else if(strcasecmp(args[0], "limit") == 0 && i > 2)
    {
    	printf("Error: Invalid limit command! Please use the following format for limit: limit <amount_of_mem>\n");
    }

    //Multiple touch commands were causing an issue in macOS, implemented a built in function to fix it
   if(strcasecmp(args[0], "touch") == 0 && i > 2)
   {
   		
   		int touchCounter = 0;
   		char *copy[3];
   		copy[0] = strdup(args[0]);

   		while(touchCounter<i-1)
   		{
   			copy[1] = strdup(args[touchCounter+1]);
   			fork_system(copy);
   			touchCounter++;
   		}
   		return 1;
   }

   //Detect if the user input is a pipe command
   while(systemCounter < i)
   {
	   if(strcasecmp(args[systemCounter], "|") == 0)
	   {
	   		if(argv[1] != NULL)
		   	{
		    	if(i < 3)
		    	{
		    		printf("Error: Invalid pipe command!\n");
		    		printf("Please use the following format for using pipes: <command_1> | <command_2>\n");
		    		return 1;
		    	}
		    	else
		    	{	
		    		pipe_system(args, i, systemCounter, argv);
		    		return 1;
		    	}
		    }
		    else 
		    {
		    	printf("Error: No fifo_path provided!");
		    	return 1;
		    }
		    
		}
		systemCounter++;
}
	
	return fork_system(args);
}

//intHandler: Prompts the user to exit the program using CTRL+C. When the user press CTRL+C the signalFlag variable is set to 1.
//This signalFlag variable is checked in get_a_line() function where it lets the user to exit the program by entering "y".
//-------------------------------------------------------------------------------------------------------------------------//
void intHandler(int sig)
{

	//printf("\nInterrupt caught:%i \n", sig);
	signalFlag = 1;
	printf("\nWould you like to exit the shell? [y/n]\n");
	//Could implement a fork() to prompt yes or no but its easier to implement the user prompt in the get_a_line()//
}

//tstpHandler: Ignores the CTRL+Z.
//-------------------------------------------------------------------------------------------------------------------------//
void tstpHandler(int sig)
{
	//printf("\nInterrupt caught:%i\n", sig);
	//printf("\nCTRL-Z is ignored in Assignment 1! Press ENTER key to continue.\n");
}

//The main function of the shell.
//-------------------------------------------------------------------------------------------------------------------------//
int main(int argc, char *argv[])
{

	signal(SIGINT, intHandler);
	signal(SIGTSTP, tstpHandler);
	while(1) 
	{
		//printf("> ");

		char *line = get_a_line();
		if((sizeof(line)) > 1)
			my_system(line, argv);

	}
	return 0;
}
//-------------------------------------------------------------------------------------------------------------------------//
