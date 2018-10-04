#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include "helper.h"

char* to_char_array(int number)
    {
        int n = log(number) + 1;
        int i;
      char *numberArray = calloc(n, sizeof(char));
        for ( i = 0; i < n; ++i, number /= 10 )
        {
            numberArray[i] = number % 10;
        }
        return numberArray;
    }


double read_sequential(int** worker_input_pipes, int num_workers, int n) {
	double f = 0;

	int n_count;
	for (n_count = 0; n_count < n; n_count++) {
		int worker_count = n_count % num_workers;
		
		printf("Awaiting result from worker %d with %d total workers\n", worker_count, num_workers);

		double r = 0;
		int read_result = read(worker_input_pipes[worker_count][READ_END], &r, sizeof(double));
		if (read_result < 0) {
			perror("Couldn't read in master from worker");
			exit(1);
		}

		// printf("Gathered: %s\n", boof);
		printf("Gathered: %f\n", r);

		f += r;
	}

	return f;
}