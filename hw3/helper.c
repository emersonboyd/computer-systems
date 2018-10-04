#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <poll.h>
#include <sys/epoll.h>

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
		
		// printf("Awaiting result from worker %d with %d total workers\n", worker_count, num_workers);

		double d;
		int read_result = read(worker_input_pipes[worker_count][READ_END], &d, sizeof(double));
		if (read_result != sizeof(double)) {
			perror("Couldn't read in master from worker");
			exit(1);
		}

		// printf("Gathered: %f\n", d);

		f += d;
	}

	return f;
}

int setup_fd_set_and_timeval(int** worker_input_pipes, int num_workers, fd_set *set, struct timeval *tv) {
	FD_ZERO(set);

	int max_fd = -1;

	// setup the fd set
	int i;
	for (i = 0; i < num_workers; i++) {
		int fd = worker_input_pipes[i][READ_END];
		// printf("Setting up file descriptor %d\n", fd);
		FD_SET(fd, set);
		max_fd = max(max_fd, fd);
	}

	tv->tv_sec = 0;
	tv->tv_usec = 500000;

	// printf("Highest fd: %d\n", max_fd);

	return max_fd;
}

double read_select(int** worker_input_pipes, int num_workers) {
	double f = 0;

	fd_set set;
	struct timeval tv;

	int max_fd = setup_fd_set_and_timeval(worker_input_pipes, num_workers, &set, &tv);

	int select_result;
	while((select_result = select(max_fd + 1, &set, NULL, NULL, &tv)) > 0) {
		int i;
		for (i = 0; i < num_workers; i++) {
			int fd = worker_input_pipes[i][READ_END];

			// check if the read bit is set for the given file descriptor
			if (!FD_ISSET(fd, &set)) {
				// printf("not set: %d\n", fd);
				continue;
			}

			printf("Awaiting read from file descriptor %d\n", fd);

			double d;
			int read_result = read(fd, &d, sizeof(double));
			if (read_result != sizeof(double)) {
				perror("Couldn't read in master from worker");
				exit(1);
			}

			printf("Just read %f from file descriptor %d\n", d, fd);

			f += d;
		}

		max_fd = setup_fd_set_and_timeval(worker_input_pipes, num_workers, &set, &tv);
	}

	if (select_result < 0) {
		perror("Error using select function");
		exit(1);
	}

	return f;
}

double read_poll(int** worker_input_pipes, int num_workers) {
	double f = 0;

	struct pollfd *pfd = (struct pollfd *) malloc(sizeof(struct pollfd) * num_workers);
	int i;
	for (i = 0; i < num_workers; i++) {
		int fd = worker_input_pipes[i][READ_END];
		pfd[i].fd = fd;
		pfd[i].events = 1; // 1 signifies that we want to know when we can read
	}
	int timeout = 500; // one-half second

	int poll_result;
	while((poll_result = poll(pfd, num_workers, timeout)) > 0) {
		for (i = 0; i < num_workers; i++) {
			int fd = worker_input_pipes[i][READ_END];
			
			// check if we can read from the file descriptor
			if (pfd[i].revents != 1) {
				continue;
			}

			printf("Revents on worker %d is equal to 1, so we can read\n", i);

			double d;
			int read_result = read(fd, &d, sizeof(double));
			if (read_result < 0) {
				perror("Couln't read in master from worker");
				exit(1);
			}

			f += d;
		}
	}

	if (poll_result < 0) {
		perror("Error using poll function");
		exit(1);
	}

	return f;
}

double read_epoll(int** worker_input_pipes, int num_workers) {
	double f = 0;



	return f;
}