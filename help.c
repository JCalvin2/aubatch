#include <stdio.h>
void help()
{
		printf("run <job> <time> <priority>\n");
		printf("^ submit a job named <job>, execution time is <time>, Priority is <priority>.\n\n");
		printf("list: display the job status.\n");
		printf("fcfs: change the scheduling policy to FCFS.\n");
		printf("sjf: change the scheduling policy to SJF.\n");
		printf("priority: change the scheduling policy to priority.\n");
		printf("test <benchmark> <policy> <num_of_jobs> <priority_levels> <min_CPU_time> <max_CPU_time>\n");
		printf("quit: exit AUbatch\n\n");
		printf(">");
}
