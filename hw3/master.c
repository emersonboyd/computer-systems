#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>

#include "helper.h"

// static const char const * const MECH_SEQUENTIAL = "sequential";
// static const char const * const MECH_SELECT = "select";
// static const char const * const MECH_POLL = "poll";
// static const char const * const MECH_EPOLL = "epoll";

static const char *ARG_WORKER_PATH = "--worker_path";
static const char *ARG_NUM_WORKERS = "--num_workers";
static const char *ARG_WAIT_MECHANISM = "--wait_mechanism";
static const char *ARG_X = "-x";
static const char *ARG_N = "-n";
static const char *MECHANISM_STRINGS[] = {"sequential", "select", "poll", "epoll"};
static const int NUM_MECHANISMS = 4;
static const char *USAGE_MESSAGE = "Usage: ./master [--worker_path path] [--num_workers num] [--wait_mechanism sequential | select | poll | epoll] [-x num] [-n num]";

enum mechanism {
	SEQUENTIAL = 0,
	 SELECT,
	  POLL,
	   EPOLL
	} MechanismArray[] = {SEQUENTIAL, SELECT, POLL, EPOLL};
typedef enum mechanism Mechanism;

typedef struct {
	char *worker_path;
	int *num_workers;
	Mechanism *wait_mechanism;
	char *x_str;
	int *x;
	int *n;
} Arg;

const Arg* parse_arguments(int argc, char **argv) {
	if (argc != 11) {
		perror("Please enter 11 arguments");
		perror(USAGE_MESSAGE);
		exit(1);
	}

	Arg *a = (Arg *) malloc(sizeof(Arg));

	int i;
	for (i = 1; i < argc; i += 2) {
		const char *argo = argv[i];
		const char *arg = argv[i + 1];

		// go through each argument and make sure we haven't already initialized it
		if (strcmp(argo, ARG_WORKER_PATH) == 0 && a->worker_path == NULL) {
			int arg_length = strlen(arg);
			a->worker_path = (char *) malloc(sizeof(char) * arg_length);
			memcpy(a->worker_path, arg, arg_length);
		}
		else if (strcmp(argo, ARG_NUM_WORKERS) == 0 && a->num_workers == NULL) {
			a->num_workers = (int *) malloc(sizeof(int));
			*(a->num_workers) = atoi(arg);

			if (*(a->num_workers) <= 0 || *(a->num_workers) > FD_SETSIZE) {
				perror("Please enter a number of workers where 0 < num_workers <= FD_SETSIZE");
				exit(1);
			}
		}
		else if (strcmp(argo, ARG_WAIT_MECHANISM) == 0 && a->wait_mechanism == NULL) {			
			int j;
			for (j = 0; j < NUM_MECHANISMS; j++) {
				if (strcmp(arg, MECHANISM_STRINGS[j]) == 0) {
					a->wait_mechanism = (Mechanism *) malloc(sizeof(Mechanism));
					*(a->wait_mechanism) = MechanismArray[j];
				}
			}

			// if we haven't determined the mechanism, the user entered an invalid mechanism value
			if (a->wait_mechanism == NULL) {
				perror("Invalid value provided for option --wait_mechanism");
				perror(USAGE_MESSAGE);
				exit(1);
			}
		}
		else if (strcmp(argo, ARG_X) == 0 && a->x_str == NULL) {
			int arg_length = strlen(arg);
			a->x_str = (char *) malloc(sizeof(char) * arg_length);
			a->x = (int *) malloc(sizeof(int));
			memcpy(a->x_str, arg, arg_length);
			*(a->x) = atoi(arg);
		}
		else if (strcmp(argo, ARG_N) == 0 && a->n == NULL) {
			a->n = (int *) malloc(sizeof(int));
			*(a->n) = atoi(arg);
		}
		else {
			perror("Invalid/duplicate argument option");
			perror(USAGE_MESSAGE);
			exit(1);
		}
 	}

	return a;
}

int main(int argc, char **argv) {

	const Arg *a = parse_arguments(argc, argv);

	int num_workers = *(a->num_workers);
	int n = *(a->n);

	// setup the array of child process IDs and the pipelines
	pid_t *cpids = (pid_t *) malloc(sizeof(pid_t) * num_workers);
	int **master_input_pipes = (int **) malloc(sizeof(int *) * num_workers);
	int **worker_input_pipes = (int **) malloc(sizeof(int *) * num_workers);
	int i;
	for (i = 0; i < num_workers; i++) {
		master_input_pipes[i] = (int *) malloc(sizeof(int) * 2);
		worker_input_pipes[i] = (int *) malloc(sizeof(int) * 2);
	}

	for (i = 0; i < num_workers; i++) {
		int pipe_result = pipe(master_input_pipes[i]);
		pipe_result |= pipe(worker_input_pipes[i]);
		if (pipe_result < 0) {
			perror("Failed to create pipes");
			exit(1);
		}

		pid_t cpid = fork();
		
		// check if this is the new process that spawned
		if (cpid == 0) {
			// int close_result = close(master_input_pipes[i][WRITE_END]);
			// close_result |= close(worker_input_pipes[i][READ_END]);
			// if (close_result != 0) {
			// 	printf("Failed to close unused pipes in worker on iteration %d: %s\n", i, strerror(errno));
			// }
			int dup_result = dup2(master_input_pipes[i][READ_END], 0);
			dup_result |= dup2(worker_input_pipes[i][WRITE_END], 1);
			if (dup_result < 0) { 
				perror("Failed to duplicate read/write pipes");
				exit(1);
			}
			execl(a->worker_path, a->worker_path, "-x", a->x_str, NULL);
			perror("Returned to main program from child process unexepectedly");
			exit(1);
		}
		else if (cpid < 0) {
			perror("Failed to create a child process");
			exit(1);
		}

		// int close_result = close(master_input_pipes[i][READ_END]);
		// close_result |= close(worker_input_pipes[i][WRITE_END]);
		// if (close_result != 0) {
		// 	printf("Failed to close unused pipes in master on iteration %d: %s\n", i, strerror(errno));
		// }

		cpids[i] = cpid;
	}

	// here we deliver the "n" to each of the workers
	int worker_counter = 0;
	for (i = 1; i <= n; i++) {
		worker_counter = (i - 1) % num_workers;
		int64_t t = (int64_t) i; // this could be a source of bugs

		// feed the current "n" value through the write pipe
		int write_result = write(master_input_pipes[worker_counter][WRITE_END], &t, sizeof(int64_t));
		if (write_result != sizeof(int64_t)) {
			perror("Could not write value from master to worker pipe");
			exit(1);
		}

		// printf("Just wrote %li to worker %d with %d total workers\n", t, worker_counter, num_workers);
	}

	double f = 0;
	switch(*(a->wait_mechanism)) {
		case SEQUENTIAL:
			f = read_sequential(worker_input_pipes, num_workers, n);
			break;
		case SELECT:
			f = read_select(worker_input_pipes, num_workers);
			break;
		default:
			perror("Invalid wait mechanism");
			break;
	}

	printf("Result: %f\n", f);

	// make sure we kill the workers
	for (i = 0; i < num_workers; i++) {
		int64_t c = 0;
		int write_result = write(master_input_pipes[i][WRITE_END], &c, sizeof(int64_t));
		if (write_result != sizeof(int64_t)) {
			perror("Couldn't write in master to worker");
			exit(1);
		}
	}

	return 0;
}

// TODO: ensure we don't have to write based on WAIT_MECHANISM
// TODO: uncomment the //close(fd) when duping in both scenarios









