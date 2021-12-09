/*This following code is credited to Professor Xiao Qin's aubatch_sample.c program*/
/* 
 * COMP7500
 * Project 3: AUbatch - A Batch Scheduling System
 *
 * Jacob Calvin
 * Department of Computer Science and Software Engineering
 * Auburn University
 * March 9, 2021. Version 1.1
 *
 * This source code demonstrates the development of 
 * a batch-job scheduler using pthread.
 *
 * Compilation Instruction: 
 * gcc aubatch.c -o aubatch -lpthread
 *
 * How to run aubatch?
 * 1. Compile the code from the example above
 * 2. Run the program: ./aubatch
 * 3. There will be instructions on how to use the program fully 
 */


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include "help.h"
#include "randomize.h"

typedef unsigned int u_int; 

#define JOBQ_BUF_SIZE 10 /* The size of the Job queue */
#define MAX_JOB_LEN  512 /* The longest commandline length */

int NUM_OF_JOBS_IN_TOTAL = 0;  /* The number of submitted jobs in total*/
int NUM_OF_JOBS_IN_Q = 0;	/*The number of submitted jobs currently in the Q*/

void *scheduler( void *ptr ); /* To simulate job submissions and scheduling */
void *dispatcher( void *ptr );    /* To simulate job execution */

pthread_mutex_t job_queue_lock;  /* Lock for critical sections */
pthread_cond_t job_buf_not_full; /* Condition variable for buf_not_full */
pthread_cond_t job_buf_not_empty; /* Condition variable for buf_not_empty */

/* Global shared variables */
u_int buf_head;
u_int buf_tail;
u_int avg_turnaround_time;
u_int avg_waiting_time;
u_int avg_cpu_time;
u_int condition = 1;
u_int cur_policy = 1;
struct tm arrival_time;
struct tm finish_time;
u_int wait = 1;
u_int quitting = 1;

/*Creation of the Job Structure in which everyjob with have these characteristics*/
struct Job
{
	char name[10];
	int burst_time;
	int priority;
	char progress[10];
	double arrival;
	double finish;
	int turnaround_time;
};

/*Creating the Job Queue and Job History (arrays)*/
struct Job jobq_buffer[JOBQ_BUF_SIZE];
struct Job jobq_history[100];

/*Quit Function begins when the user inputs quit. Its goal is to do a performance evaluation on the inputted jobs */
void quit()
{	
	if(NUM_OF_JOBS_IN_TOTAL > 0) /*Making sure there are jobs to evaluate, this is if the user inputs quit when the program begins*/
	{
		/*The following is looping through the Job History and calculating the average turnaround time, average wait time, average cpu time, and throughput. */
		printf("Starting to Quit\n");
		for(int i = 0; i < NUM_OF_JOBS_IN_TOTAL; i++)
		{
			int turnaround = jobq_history[i].finish - jobq_history[i].arrival;
			jobq_history[i].turnaround_time = turnaround;
			avg_turnaround_time = avg_turnaround_time + turnaround; 
		}
		avg_turnaround_time = avg_turnaround_time / NUM_OF_JOBS_IN_TOTAL;
	
		double throughput = 1 / (double)avg_turnaround_time;	

		for(int i = 0; i < NUM_OF_JOBS_IN_TOTAL; i++)
		{
			int waiting_time = jobq_history[i].turnaround_time - jobq_history[i].burst_time;
			avg_waiting_time = avg_waiting_time + waiting_time; 
		}
		double average_waiting_time = (double)avg_waiting_time / (double)NUM_OF_JOBS_IN_TOTAL;

		for(int i = 0; i < NUM_OF_JOBS_IN_TOTAL; i++)
		{
			avg_cpu_time = avg_cpu_time + jobq_history[i].burst_time;  
		}
		avg_cpu_time = avg_cpu_time / NUM_OF_JOBS_IN_TOTAL;

		printf("Quitting the Program!\n");
		printf("Performance Evaluation!\n\n");
		printf("Total Number of Jobs Submitted: %d", (NUM_OF_JOBS_IN_TOTAL));
		printf("\n");
		printf("Average Turnaround Time: %d", avg_turnaround_time);
		printf(" seconds\n");	
		printf("Average CPU Time: %d", avg_cpu_time);
		printf(" seconds\n"); 
		printf("Average Waiting Time: %lf", average_waiting_time);
		printf(" seconds\n");
		printf("Throughput: %lf", throughput);
		printf(" No./sec\n"); 
	}
	
	else
	{
		printf("No Jobs Submitted\n");
	}
}

char input[MAX_JOB_LEN];

/*Main Function designed to create the threads needed and complete the prorgam once the threads have done their jobs*/
int main() 
{
    pthread_t scheduler_thread, dispatcher_thread; /* Two concurrent threads */
    char *message1 = "Scheduler Thread";
    char *message2 = "Dispatcher Thread";
    int  iret1, iret2;
	
	printf("Jacob Calvin's AUBatch Program 1.0\n");
	printf("Type 'help' for further instructions on how this program functions\n");
	
	printf("Please note that when inputting priority of a job, low number means high priority.\n");
    printf("Please submit a batch processing job:\n");
    printf(">"); 
	
	/* Initialize the lock & the two condition variables */
    pthread_mutex_init(&job_queue_lock, NULL);
    pthread_cond_init(&job_buf_not_full, NULL);
    pthread_cond_init(&job_buf_not_empty, NULL);

	/*This continues as long as the user does not input quit*/
	while(strcmp(input, "quit"))
	{
		/* Create two independent threads:scheduler and dispatcher */
		iret1 = pthread_create(&scheduler_thread, NULL, scheduler, (void*) message1);
		iret2 = pthread_create(&dispatcher_thread, NULL, dispatcher, (void*) message2);
	}	
	
	printf("Completed Quitting\n");

    exit(0);
}

/*Run Function that executes when the user inputs a run job command*/
void run()
{
	char name[10];
	int burst_time;
	int priority;	

	/*Scans User input*/
	scanf("%s %d %d", name, &burst_time, &priority);
	
	printf("Job %s has been Submitted.\n", name);
	printf("Total Jobs in Q before this one is %d \n", NUM_OF_JOBS_IN_Q); 
	if(cur_policy == 1)
	{
		printf("Scheduling policy is FCFS.\n");
	}

	else if(cur_policy == 2)
	{
		printf("Scheduling policy is SJF.\n");
	}

	else if(cur_policy == 3)
	{
		printf("Scheduling policy is Priority.\n");
	}

	/*Creates a new Job based on the user input || Job is put in the waiting status*/
	struct Job new_job;
	strcpy(new_job.name, name);
	new_job.burst_time = burst_time;
	new_job.priority = priority;
	strcpy(new_job.progress, "Waiting");
	time_t t;
	t = time(NULL);
	arrival_time = *localtime(&t); /*marking down arrival time*/
	int hour = arrival_time.tm_hour;
	hour*=3600;
	int min = arrival_time.tm_min;
	min*=60;
	int sec = arrival_time.tm_sec; /*putting the current time in seconds*/
	int total = hour+min+sec;
	new_job.arrival = total;
	new_job.finish = 0;
	new_job.turnaround_time = 0;
	
	/*Adds the new jobs to the Job Q & signals that the Q is not empty anymore*/
	jobq_buffer[NUM_OF_JOBS_IN_Q] = new_job;
	pthread_cond_signal(&job_buf_not_empty);
	NUM_OF_JOBS_IN_Q++;
}

/*This function rearranges the job Q if the policy is fcfs*/
void fcfs()
{
	/*For every job in the Q, we need to arrange it in order of their arrival rate*/
	if(NUM_OF_JOBS_IN_Q > 0)
	{
		for(int i = 0; i < NUM_OF_JOBS_IN_Q; i++)
		{
			for(int j = i + 1; j < NUM_OF_JOBS_IN_Q; j++)
			{
				if(jobq_buffer[i].arrival >= jobq_buffer[j].arrival)
				{
					struct Job temp = jobq_buffer[i];
					jobq_buffer[i] = jobq_buffer[j];
					jobq_buffer[j] = temp; 
				}
			}
		}
	}
}

/*This function rearranges the job Q if the policy is SJF*/
void sjf()
{
	/*For every job in the Q, we need to arrange it in order of their burst time*/
	if(NUM_OF_JOBS_IN_Q > 0)
	{
		for(int i = 0; i < NUM_OF_JOBS_IN_Q; i++)
		{
			for(int j = i + 1; j < NUM_OF_JOBS_IN_Q; j++)
			{
				if(jobq_buffer[i].burst_time >= jobq_buffer[j].burst_time)
				{
					struct Job temp = jobq_buffer[i];
					jobq_buffer[i] = jobq_buffer[j];
					jobq_buffer[j] = temp; 
				}
			}
		}
	}
}

/*This function rearranges the job Q if the policy is priority*/
void priority()
{
	/*For every job in the Q, we need to arrange it in order of their priority*/
	if(NUM_OF_JOBS_IN_Q > 0)
	{
		for(int i = 0; i < NUM_OF_JOBS_IN_Q; i++)
		{
			for(int j = i + 1; j < NUM_OF_JOBS_IN_Q; j++)
			{
				if(jobq_buffer[i].priority >= jobq_buffer[j].priority)
				{
					struct Job temp = jobq_buffer[i];
					jobq_buffer[i] = jobq_buffer[j];
					jobq_buffer[j] = temp; 
				}
			}
		}
	}
}


/*The list function executes when the user inputs the list command on the command line*/
void list()
{	
	/*Prints the Job Q History of every job that the user has submitted and their progress*/
	printf("Total Number of Jobs in Q: %d", NUM_OF_JOBS_IN_Q);
	printf("\n");
	if(cur_policy == 1)
	{
		printf("Scheduling policy is FCFS.\n");
	}

	else if(cur_policy == 2)
	{
		printf("Scheduling policy is SJF.\n");
	}

	else if(cur_policy == 3)
	{
		printf("Scheduling policy is Priority.\n");
	}

	printf("Current Job Queue\n");
	for(int i = 0; i < NUM_OF_JOBS_IN_Q; i++)
	{
		printf("Name: %s ", jobq_buffer[i].name);
		printf("Burst Time: %d ",jobq_buffer[i].burst_time);
		printf("Priority: %d ", jobq_buffer[i].priority);
		printf("Progress: %s ", jobq_buffer[i].progress);
		printf("\n\n");
	}

	printf("Job Queue History\n");
	for(int i = 0; i < NUM_OF_JOBS_IN_TOTAL; i++)
	{
		printf("Name: %s ", jobq_history[i].name);
		printf("Burst Time: %d ",jobq_history[i].burst_time);
		printf("Priority: %d ", jobq_history[i].priority);
		printf("Progress: %s ", jobq_history[i].progress);
		printf("\n");
	}
}

/*This function executes when the user inputs a benchmark to be tested*/
void test()
{
	char benchmark[10];
	char policy[10];
	int job_num;
	int priority_level;
	int min_cpu_time;
	int max_cpu_time;

	/*Scans the user input*/
	scanf("%s %s %d %d %d %d", benchmark, policy, &job_num, &priority_level, &min_cpu_time, &max_cpu_time); 
	
	printf("\n");
	printf("Testing Benchmark, Please Wait,\n");	
	
	/*Creates array for the benchmarks's policy, number of jobs, their priortity level, and their min/max burst time*/
	int times[job_num];
	int priorities[job_num];
	int tnr[job_num];
	int wait[job_num];

	/*This marks down the arrival time of the jobs*/
	time_t t3;
	t3 = time(NULL);
	struct tm arr = *localtime(&t3);
	int hour = arr.tm_hour;
	hour*=3600;
	int min = arr.tm_min;
	min*=60;
	int sec = arr.tm_sec;
	int ar = hour+min+sec;

	//From: https://www.geeksforgeeks.org/generating-random-number-range-c/
	/*This creates a random burst time for each job based on the min and max cpu time supplied by the user*/
	for(int i = 0; i < job_num; i++)
	{		
		times[i] = randomize(max_cpu_time, min_cpu_time);	
	}

	/*This supplies a random priority for each job based on the priority level supplied by the user*/
	for(int i = 0; i < job_num; i++)
	{		
		priorities[i] = randomize(priority_level, 1);	
	}
	
	/*The following code tests if the policy inputted is SJF, Priority, or FCFS & rearranges the array based on these policies*/
	if(!strcmp(policy, "sjf")) 
	{
		printf("Current Policy is SJF\n");
		for(int i = 0; i < job_num; i++)
		{
			for(int j = i + 1; j < job_num; j++)
			{
				if(times[i] >= times[j])
				{
					int temp = times[i];
					times[i] = times[j];
					times[j] = temp;
				}
			}
		}
	}

	else if(!strcmp(policy, "priority"))
	{
		printf("Current Policy is Priority\n");
		for(int i = 0; i < job_num; i++)
		{
			for(int j = i + 1; j < job_num; j++)
			{	
					if(priorities[i] >= priorities[j])
					{
						int temp = times[i];
						times[i] = times[j];
						times[j] = temp;
					}
			}
		}
	}

	else if(!strcmp(policy, "fcfs"))
	{
		printf("Current Policy is FCFS\n");
	}

	else
	{
		printf("Invalid Job Input: Default is FCFS\n");
	}

	/*Similating the execution of the jobs*/
	for(int i = 0; i < job_num; i++)
	{
		int ti = times[i];
		sleep(ti);
		time_t t4;
		t4 = time(NULL);
		struct tm fin = *localtime(&t4);
		int hour = fin.tm_hour;
		hour*=3600;
		int min = fin.tm_min;
		min*=60;
		int sec = fin.tm_sec;
		int f = hour+min+sec;			
		tnr[i] = f - ar;
		wait[i] = tnr[i] - ti;
	}
	
	int totaltnr = 0;
	int totalwait = 0;
	int totalcpu = 0;	

	/*Calculating the total turnaround, waiting, and cpu times. */
	for(int i = 0; i < job_num; i++)
	{
		totaltnr = totaltnr + tnr[i];
		totalwait = totalwait + wait[i];
		totalcpu = totalcpu + times[i];
	}
	
	/*Calculating the avg turnaround, waiting, cpu time, and throughput. */
	int avgtnr = totaltnr / job_num;
	double avgwait = (double)totalwait / (double)job_num;
	double through = 1 / (double)avgtnr;
	int avgcpu = totalcpu / job_num;

	printf("Performance Eval for Benchmark %s", benchmark);
	printf("\n");
	printf("Average Turnaround Time: %d", avgtnr);
	printf("\n");
	printf("Average Waiting Time: %lf", avgwait);
	printf("\n");
	printf("Average CPU Time: %d", avgcpu);
	printf("\n");
	printf("Throughput: %lf", through);
	printf(" No./sec\n");
	
}


/* 
 * This function simulates a terminal where users may 
 * submit jobs into a batch processing queue.
 * The user will be prompted with a list of commands of what they can do.
 * This threads job is scheduling tasks/jobs that the user inputs
 */
void *scheduler(void *ptr) 
{	
	/*Checking whether the Job Q is full or not*/
	pthread_mutex_lock(&job_queue_lock);
    while (NUM_OF_JOBS_IN_Q == JOBQ_BUF_SIZE) 
	{
    	pthread_cond_wait(&job_buf_not_full, &job_queue_lock);
    }		

	/*Scannign user input*/
	scanf("%s", input);		
	
	/*If the user inputs run*/
	if (!strcmp(input, "run"))
	{
		run();
		printf(">");
	}

	/*If the user inputs help*/
	else if (!strcmp(input, "help"))
	{
		help();
	}

	/*If the user inputs list*/
	else if (!strcmp(input, "list"))
	{
		list();
		printf(">");
	}

	/*If the user inputs fcfs*/
	else if (!strcmp(input, "fcfs"))
	{
		printf("Changing Scheduling Policy to First Come First Serve (FCFS).\n");
		cur_policy = 1;
		printf(">");
	}

	/*If the user inputs sjf*/
	else if (!strcmp(input, "sjf"))
	{
		printf("Changing Scheduling Policy to Shortest Job First (SJF).\n");
		cur_policy = 2;
		printf(">");
	}

	/*If the user inputs priority*/
	else if (!strcmp(input, "priority"))
	{
		printf("Changing Scheduling Policy to Priority.\n");
		cur_policy = 3;
		printf(">");
	}

	/*If the user inputs test*/
	else if (!strcmp(input, "test"))
	{
		test();
		printf(">");
	}
	
	/*If the user inputs quit*/
	else if(!strcmp(input, "quit"))
	{
		quit();
	}		

	/*If the user inputs anything else it will prompt them with the help menu*/
	else
	{
		help();
	}
	
	/*the following code rearranges the job Q based on the current policy*/
	if(cur_policy == 1)
	{
		fcfs();
	}
	
	else if(cur_policy == 2)
	{
		sjf();
	}
	
	else if(cur_policy == 3)
	{
		priority();
	}

	pthread_mutex_unlock(&job_queue_lock);
}



/*
 * This function simulates a server running jobs in a batch mode.
 * This threads job is to execute tasks/jobs in the Job Queue and run a performance evaluation when the user is done.
 */
void *dispatcher(void *ptr) 
{	
	/*Checking to see if the Job Q is empty*/
	pthread_mutex_lock(&job_queue_lock);
	while (NUM_OF_JOBS_IN_Q == 0) 
	{
        pthread_cond_wait(&job_buf_not_empty, &job_queue_lock);
	}		

	/*Looping through the job Q and executing the jobs in there*/
	for(int i = 0; i < NUM_OF_JOBS_IN_Q; i++)
	{
		int timer = jobq_buffer[0].burst_time;
		sleep(timer);
		system(jobq_buffer[0].name);
		strcpy(jobq_buffer[0].progress, "Complete");
		time_t t2;
		t2 = time(NULL);
		finish_time = *localtime(&t2);
		int hour = finish_time.tm_hour;
		hour*=3600;
		int min = finish_time.tm_min;
		min*=60;
		int sec = finish_time.tm_sec;
		int total = hour+min+sec;
		jobq_buffer[0].finish = total;
				
		jobq_history[NUM_OF_JOBS_IN_TOTAL] = jobq_buffer[0];
		/* Moving every element of the Q over 1 so we execute the next job in Q*/
		for(int j = 0; j < NUM_OF_JOBS_IN_Q - 1; j++)
		{
			jobq_buffer[j] = jobq_buffer[j+1];
		}

		NUM_OF_JOBS_IN_Q--;
		NUM_OF_JOBS_IN_TOTAL++;
		pthread_cond_signal(&job_buf_not_full);
	}

	pthread_mutex_unlock(&job_queue_lock);
}




