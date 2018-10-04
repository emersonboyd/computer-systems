#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>

static const int ASCII_0 = 48;
static const int ASCII_9 = 57;
static const char *ARG_X = "-x";
static const char *ARG_INPUT = "-i";
static const char *ARG_OUTPUT = "-o";
static const char *USAGE_MESSAGE = "Usage: ./worker [-x num] [-i num (optional)] [-o num (optional)]";
static const int INPUT_DEFAULT = 0;
static const int OUTPUT_DEFAULT = 1;

typedef struct {
	int *x;
	int *input;
	int *output;
} Arg;

const Arg* parse_arugments(int argc, char **argv) {
	if (argc != 3 && argc != 5 && argc != 7) {
		perror("Please enter 3, 5 or 7 arguments");
		perror(USAGE_MESSAGE);
		exit(1);
	}

	Arg *a = (Arg *) malloc(sizeof(Arg));

	int i;
	for (i = 1; i < argc; i += 2) {
		const char *argo = argv[i];
		const char *arg = argv[i + 1];

		// go through each argument and make sure we haven't already initialized it
		if (strcmp(argo, ARG_X) == 0 && a->x == NULL) {
			a->x = (int *) malloc(sizeof(int));
			*(a->x) = atoi(arg);
		}
		else if (strcmp(argo, ARG_INPUT) == 0 && a->input == NULL) {
			a->input = (int *) malloc(sizeof(int));
			*(a->input) = atoi(arg);
		}
		else if (strcmp(argo, ARG_OUTPUT) == 0 && a->output == NULL) {
			a->output = (int *) malloc(sizeof(int));
			*(a->output) = atoi(arg);
		}
		else {
			perror("Invalid/duplicate argument option");
			perror(USAGE_MESSAGE);
			exit(1);
		}
 	}

 	if (a->x == NULL) {
 		perror("Please enter a value for x");
 		perror(USAGE_MESSAGE);
 		exit(1);
 	}

 	// by default we assign the input file descriptor to 0 (stdin)
 	if (a->input == NULL) {
		a->input = (int *) malloc(sizeof(int));
 		*(a->input) = INPUT_DEFAULT;
 	}

 	// by default we assign the output file descriptor to 1 (stdout)
 	if (a->output == NULL) {
		a->output = (int *) malloc(sizeof(int));
		*(a->output) = OUTPUT_DEFAULT;
 	}

	return a;
}

// determines whether we are using the console as input or a pipe
bool get_is_fifo() {
	struct stat file_stat;
	int fstat_result = fstat(STDIN_FILENO, &file_stat);
	if (fstat_result < 0) {
		perror("Couldn't get fstat");
		exit(1);
	}

	return S_ISFIFO(file_stat.st_mode);
}

bool is_digit(char c) {
	return c >= ASCII_0 && c <= ASCII_9;
}

int64_t read_int_from_fd(int input_fd) {
	int64_t n;

	int read_result = read(input_fd, &n, sizeof(int64_t));
	if (read_result != sizeof(int64_t)) {
		perror("Could not read 64-bit integer in worker from master");
		exit(1);
	}

	return n;
}

void write_double_to_fd(int output_fd, double f) {
	int write_result = write(output_fd, &f, sizeof(double));
	if (write_result != sizeof(double)) {
		perror("Could not write double in worker to master");
		exit(1);
	}
	// char buff[100];
	// sprintf(buff, "Just wrote %f to file descriptor %d\n", f, output_fd);
	// perror(buff);
}

int read_int_from_stdin() {
	int input_fd = STDIN_FILENO;

	fflush(stdout);

	char n_str[1];
	int n = 0;

	while (true) {
		int read_result = read(input_fd, n_str, sizeof(char));
		if (read_result < 0) {
			perror("Couldn't read from stdin");
			exit(1);
		}

		char c = n_str[0];

		// check if the inputted character is not a number, and end the loop if so
		if (!is_digit(c)) {
			break;
		}

		// append the next character to n
		n *= 10;
		n += c - ASCII_0;
	}

	// if the user didn't just enter the newline character, we need to keep reading to flush the stdin
	if (n_str[0] != '\n') {
		char buff[100];
		// read in any extra characters the user may have entered
		int read_result = read(input_fd, buff, sizeof(buff));
		if (read_result < 0) {
			perror("Couldn't read from stdin");
			exit(1);
		}
	}

	return n;
}

double fnx_int(int n, int x) {
	return ((double) x) * n / (n + 1);
}

double fnx_int64(int n, int64_t x) {
	return ((double) x) * n / (n + 1);
}

int main(int argc, char **argv) {
	const Arg *a = parse_arugments(argc, argv);
	int x = *(a->x);
	int input_fd = *(a->input);
	int output_fd = *(a->output);

	bool is_fifo = get_is_fifo();

	while (true) {
		if (is_fifo) {
			int64_t n = read_int_from_fd(input_fd);

			if (n == 0) {
				break;
			}

			double f = fnx_int64(n, x);
			write_double_to_fd(output_fd, f);
		}
		else {
			printf("Input n (0 to exit): ");
			int n = read_int_from_stdin();
			printf("You entered: %d\n", n);

			if (n < 0) {
				printf("Please enter a non-negative integer n. ");
				continue;
			} 
			else if (n == 0) {
				break; 
			}

			double f = fnx_int(n, x);
			printf("x*n / (n + 1) ; %f\n", f);
		}
	}

	if (!is_fifo) {
		printf("Goodbye!\n");
	}

	exit(0);
}