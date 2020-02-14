//----------------------------------------------------------------------------------------------------------------------------------------------//
//  ECSE 427
//  ASSIGNMENT 2 - Part 1
//  ULUC ALI TAPAN
//  260556540
//----------------------------------------------------------------------------------------------------------------------------------------------//
//This program has 2 modes:
// 1- No input arguments (default)
// 2- ./a.out <NUMBER_OF_WRITES> <NUMBER_OF_READS>
//The default settings are 10 writer threads and 500 reader threads. The manual inputs can only support up to 1000 threads.

#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

static sem_t rw_mutex; //Common to both reader and writer threads						
static sem_t mutex;	//Ensures mutual exclusion when variable read_count is updated
static int read_count = 0; //Keeps track of how many threads are currently reading the object

static int target_variable = 0;

//For timing purposes//
float findMaximumValue(float sample[], size_t size);
float findMinimumValue(float sample[], size_t size);
float findAverageValue(float sample[], size_t size);

static int reader_index = 0; //Index variable for reader
static int writer_index = 0; //Index variable for writer

//For default run
static float reader_time[500];
static float writer_time[10];

//For argv run
static float reader_time_argv[1000];
static float writer_time_argv[1000];

//Flag for the program to know which mode the program is on
static int isDefault = 1;

//readerFunc: The reader thread that is created by the main function. It follows the pseudo-code from the
//assignment speficiations. Prints out the current target_variable and returns null. Uses semaphores to wait
//and signal other threads.
//--------------------------------------------------------------------------------------------------------//
static void *readerFunc(void * args){

	//Initiazilize variables
	int loops = *((int *) args);
	int local_variable;
	struct timeval time_value; 
	time_t time = 0;
	time_t time_In = 0;
	time_t time_Out = 0;
	int r = rand();

	for(int i = 0; i < loops; i++)
	{
		//Start the timing before sem_wait opereation
		gettimeofday(&time_value, NULL);
		time_In = time_value.tv_sec*1000000 + time_value.tv_usec;

		if(sem_wait(&mutex) == -1)
		{
			exit(2);
		}

		//Get the timing after sem_wait operation
		gettimeofday(&time_value, NULL);
		time_Out = time_value.tv_sec*1000000 + time_value.tv_usec;
   		time = time + (time_Out - time_In);

   		//Re-Initiazilize
    	time_Out = 0;                             
   		time_In = 0;

		read_count++;

		if(read_count == 1)
		{
			//Start the timing before sem_wait opereation
			gettimeofday(&time_value, NULL);
			time_In = time_value.tv_sec*1000000 + time_value.tv_usec;

			if(sem_wait(&rw_mutex) == -1)
			{
				exit(2);
			}

			//Get the timing after sem_wait operation
			gettimeofday(&time_value, NULL);
			time_Out = time_value.tv_sec*1000000 + time_value.tv_usec;
   			time = time + (time_Out - time_In);

   			//Re-Initiazilize
    		time_Out = 0;                             
   			time_In = 0;

		}

		if(sem_post(&mutex) == -1)
		{
			exit(2);
		}

		//Perform Read
		printf("Target variable = %d\n", target_variable);

		//Start the timing before sem_wait opereation
		gettimeofday(&time_value, NULL);
		time_In = time_value.tv_sec*1000000 + time_value.tv_usec;

		if(sem_wait(&mutex) == -1)
		{
      		exit(2);
		}

		//Get the timing after sem_wait operation
		gettimeofday(&time_value, NULL);
		time_Out = time_value.tv_sec*1000000 + time_value.tv_usec;
   		time = time + (time_Out - time_In);

   		//Re-Initiazilize
    	time_Out = 0;                             
   		time_In = 0;

		read_count--;

		if(read_count == 0)
		{
			if(sem_post(&rw_mutex) == -1)
      		{
        		exit(2);
      		}
		}

		if(sem_post(&mutex) == -1)
		{
			exit(2);
		}

		usleep((float)(r%100));
	}

	if(isDefault == 1)
	{
		reader_time[reader_index] = time;
	}
	else
	{
		reader_time_argv[reader_index] = time;
	}
	reader_index++;
	return NULL;
}

//writerFunc: The writer thread that is created by the main function. It follows the pseudo-code from the
//assignment specifications. Increments the target_variable by 10 and stores it back into it. Uses
//semaphores to wait and signal other threads.
//--------------------------------------------------------------------------------------------------------//
static void *writerFunc(void *args){

	int loops = *((int *) args);
	int local_variable;
	struct timeval time_value; 
	time_t time = 0;
	time_t time_In = 0;
	time_t time_Out = 0;
	int r = rand();

	for(int i = 0; i < loops; i++)
	{
		//Start the timing before sem_wait opereation
		gettimeofday(&time_value, NULL);
		time_In = time_value.tv_sec*1000000 + time_value.tv_usec;

		if(sem_wait(&rw_mutex) == -1)
		{
			exit(2);
		}

		//Get the timing after sem_wait operation
		gettimeofday(&time_value, NULL);
		time_Out = time_value.tv_sec*1000000 + time_value.tv_usec;
   		time = time + (time_Out - time_In);

   		//Re-Initiazilize
    	time_Out = 0;                             
   		time_In = 0;

		printf("Writing to target variable!\n");
		local_variable = target_variable;
		local_variable = local_variable + 10;
		target_variable = local_variable;

		if(sem_post(&rw_mutex) == -1)
		{
			exit(2);
		}

		usleep((float)(r%100));
	}
	if(isDefault == 1)
	{
		writer_time[writer_index] = time;
	}
	else
	{
		writer_time_argv[writer_index] = time;
	}
	writer_index++;
	return NULL;
}


//The main function of reader-writer assignment.
//--------------------------------------------------------------------------------------------------------//
int main(int argc, char *argv[])
{
	//Initiazilize Variables
	int writerInput;
	int readerInput;
	pthread_t readers[500];
	pthread_t writers[10];


	int s; //Semaphore
	int repeat_counter_writer = 30;
	int repeat_counter_reader = 60;
	float maximumReadTime, minimumReadTime, maximumWriteTime, minimumWriteTime, averageWriteTime, averageReadTime;

	//Initialize semaphores
	if(sem_init(&mutex,0,1) == -1 || sem_init(&rw_mutex,0,1) == -1)
  	{
    	printf("Error init semaphore\n");
    	exit(1);
  	}

 	//Create readers and writers as default (no input arguments)
  	if(argc == 1)
  	{
  		//Create the Wrtiers Thread
	  	for(int i = 0; i < 10; i++)
	  	{
	  		printf("Creating a Writer Thread\n");
	  		s = pthread_create(&writers[i], NULL, &writerFunc, &repeat_counter_writer);
	  	}

	  	//Create the Readers Threads
	  	for(int j = 0; j < 500; j++)
	  	{
	  		printf("Creating a Reader Thread\n");
	  		s = pthread_create(&readers[j], NULL, &readerFunc, &repeat_counter_reader);
	  	}
		
	  	//Detach the Writer Threads
	  	for(int k = 0; k < 10; k++)
	  	{
	  		s = pthread_join(writers[k], NULL);
	  		if (s != 0)
	  		{
	  			printf("Error, creating threads\n");
	    		exit(1);
	  		}
	  	}

	  	//Detach the Reader Threads
	  	for(int l = 0; l < 500; l++)
	  	{
	  		s = pthread_join(readers[l], NULL);
	  		if (s != 0)
	  		{
	  			printf("Error, creating threads\n");
	    		exit(1);
	  		}
	  	}

		printf("No input arguments were provided. Program ran with 10 writer threads and 500 reader threads.\n");
		printf("Final value of the target_variable is: %d\n", target_variable);

  		maximumReadTime = findMaximumValue(reader_time, 500);
  		maximumWriteTime= findMaximumValue(writer_time, 10);
		printf("Maximum Reader Time = %f us\n", maximumReadTime);
		printf("Maximum Writer Time = %f us\n", maximumWriteTime);

  		minimumReadTime = findMinimumValue(reader_time, 500);
  		minimumWriteTime = findMinimumValue(writer_time, 10);
  		printf("Minimum Reader Time = %f us\n", minimumReadTime);
		printf("Minimum Writer Time = %f us\n", minimumWriteTime);

  		averageReadTime = findAverageValue(reader_time, 500);
  		averageWriteTime = findAverageValue(writer_time, 10);
  		printf("Average Reader Time = %f us\n", averageReadTime);
		printf("Average Writer Time = %f us\n", averageWriteTime);

	}
	else if(argc == 2)
	{
		printf("Invalid arguments. The input format shall be:\n ./a.out <NUMBER_OF_WRITES> <NUMBER_OF_READS>\n");
		exit(0);
	}
	//Create readers and writers with the given inputs
	else
	{
		writerInput = atoi(argv[1]);
		readerInput = atoi(argv[2]);
		isDefault = 0;
		pthread_t readersInput[readerInput];
		pthread_t writersInput[writerInput];

		if(writerInput == 0 || readerInput == 0)
		{
			printf("Invalid arguments. The input shall be with integers:\n ./a.out <NUMBER_OF_WRITES> <NUMBER_OF_READS>\n");
			exit(0);
		}
		else if(writerInput > 1000 || readerInput > 1000)
		{
			printf("The limits for this program is 1000 threads. If you would like to increase these limits you may\n edit the programs global variables.");
			exit(0);
		}

		//Create the Wrtiers Thread
		for(int i = 0; i < writerInput; i++)
	  	{
	  		printf("Creating a Writer Thread\n");
	  		s = pthread_create(&writersInput[i], NULL, &writerFunc, &repeat_counter_writer);
	  	}

	  	//Create the Readers Threads
	  	for(int j = 0; j < readerInput; j++)
	  	{
	  		printf("Creating a Reader Thread\n");
	  		s = pthread_create(&readersInput[j], NULL, &readerFunc, &repeat_counter_reader);
	  	}
		
	  	//Detach the Writer Threads
	  	for(int k = 0; k < writerInput; k++)
	  	{
	  		s = pthread_join(writersInput[k], NULL);
	  		if (s != 0)
	  		{
	  			printf("Error, creating threads\n");
	    		exit(1);
	  		}
	  	}

	  	//Detach the Reader Threads
	  	for(int l = 0; l < readerInput; l++)
	  	{
	  		s = pthread_join(readersInput[l], NULL);
	  		if (s != 0)
	  		{
	  			printf("Error, creating threads\n");
	    		exit(1);
	  		}
	  	}
	  	printf("Final value of the target_variable is: %d\n", target_variable);

  		maximumReadTime = findMaximumValue(reader_time_argv, readerInput);
  		maximumWriteTime= findMaximumValue(writer_time_argv, writerInput);
		printf("Maximum Reader Time = %f us\n", maximumReadTime);
		printf("Maximum Writer Time = %f us\n", maximumWriteTime);

  		minimumReadTime = findMinimumValue(reader_time_argv, readerInput);
  		minimumWriteTime = findMinimumValue(writer_time_argv, writerInput);
  		printf("Minimum Reader Time = %f us\n", minimumReadTime);
		printf("Minimum Writer Time = %f us\n", minimumWriteTime);

  		averageReadTime = findAverageValue(reader_time_argv, readerInput);
  		averageWriteTime = findAverageValue(writer_time_argv, writerInput);
  		printf("Average Reader Time = %f us\n", averageReadTime);
		printf("Average Writer Time = %f us\n", averageWriteTime);
	}

  	exit(0);
}


//--------------------------------------------------------------------------------------------------------//
//************************************** TIMING ANALYSIS FUNCTIONS ***************************************//
//--------------------------------------------------------------------------------------------------------//

//findMinimumValue: Finds the minimum value in the array of floats with the given size. The argument "sample"
//is the array that contains all the timing and the argument "size" is the size of the sample, which in this
//case it is the number of times the thread is created.
//--------------------------------------------------------------------------------------------------------//
float findMinimumValue(float sample[], size_t size)
{
	float minimum = sample[0];	
  	for(int i = 1; i < size; i++)
  	{
    	if(minimum > sample[i])
    	{
      		minimum = sample[i];
    	}
  	}

  	return minimum;
}

//findMaximumValue: Finds the maximum value in the array of floats with the given size. The argument "sample"
//is the array that contains all the timing and the argument "size" is the size of the sample, which in this
//case it is the number of times the thread is created.
//--------------------------------------------------------------------------------------------------------//
float findMaximumValue(float sample[], size_t size)
{
  	float maximum = sample[0];
  	for(int i = 1; i < size; i++)
  	{
  		if(maximum < sample[i])
    	{
      		maximum = sample[i];
   		}
  	}

  	return maximum;
}

//findAverageValue: Finds the average value in the arra of floats and given size. The argument "sample"
//is the array that contains all the timing and the argument "size" is the size of the sample, which in this
//case it is the number of times the thread is created.
//--------------------------------------------------------------------------------------------------------//
float findAverageValue(float sample[], size_t size) 
{
	float sum = 0;
  	float average = 0;
  	for(int i = 0; i < size; i++)
  	{
  	  sum = sum + sample[i];
  	}
  	average = sum/((float) size);

  	return average;
}
//--------------------------------------------------------------------------------------------------------//
