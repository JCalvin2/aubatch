#include <stdio.h>
#include <stdlib.h>

int randomize(int max, int min)
{
	return (rand() % (max - min + 1)) + min;
}

