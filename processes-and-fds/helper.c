#include <math.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

#include "helper.h"

char* to_char_array(int number) {
  int n = log(number) + 1;
  int i;
  char* numberArray = calloc(n, sizeof(char));
  for (i = 0; i < n; ++i, number /= 10) {
    numberArray[i] = number % 10;
  }
  return numberArray;
}

double read_sequential(int** worker_input_pipes, int num_workers, int n) {
  double f = 0;

  int n_count;
  for (n_count = 0; n_count < n; n_count++) {
    int worker_count = n_count % num_workers;

    double d;
    int read_result =
        read(worker_input_pipes[worker_count][READ_END], &d, sizeof(double));
    if (read_result != sizeof(double)) {
      perror("Couldn't read in master from worker");
      exit(1);
    }

    f += d;
  }

  return f;
}

int setup_fd_set_and_timeval(int** worker_input_pipes, int num_workers,
                             fd_set* set, struct timeval* tv) {
  FD_ZERO(set);

  int max_fd = -1;

  // setup the fd set
  int i;
  for (i = 0; i < num_workers; i++) {
    int fd = worker_input_pipes[i][READ_END];
    FD_SET(fd, set);
    max_fd = max(max_fd, fd);
  }

  tv->tv_sec = 0;
  tv->tv_usec = 500000;

  return max_fd;
}

double read_select(int** worker_input_pipes, int num_workers) {
  double f = 0;

  fd_set set;
  struct timeval tv;

  int max_fd =
      setup_fd_set_and_timeval(worker_input_pipes, num_workers, &set, &tv);

  int select_result;
  while ((select_result = select(max_fd + 1, &set, NULL, NULL, &tv)) > 0) {
    int i;
    for (i = 0; i < num_workers; i++) {
      int fd = worker_input_pipes[i][READ_END];

      // check if the read bit is set for the given file descriptor
      if (!FD_ISSET(fd, &set)) {
        continue;
      }

      double d;
      int read_result = read(fd, &d, sizeof(double));
      if (read_result != sizeof(double)) {
        perror("Couldn't read in master from worker");
        exit(1);
      }

      f += d;
    }

    max_fd =
        setup_fd_set_and_timeval(worker_input_pipes, num_workers, &set, &tv);
  }

  if (select_result < 0) {
    perror("Error using select function");
    exit(1);
  }

  return f;
}

double read_poll(int** worker_input_pipes, int num_workers) {
  double f = 0;

  struct pollfd* pfd =
      (struct pollfd*)malloc(sizeof(struct pollfd) * num_workers);
  int i;
  for (i = 0; i < num_workers; i++) {
    int fd = worker_input_pipes[i][READ_END];
    pfd[i].fd = fd;
    pfd[i].events = 1; // 1 signifies that we want to know when we can read
  }
  int timeout = 500; // one-half second

  int poll_result;
  while ((poll_result = poll(pfd, num_workers, timeout)) > 0) {
    for (i = 0; i < num_workers; i++) {
      int fd = worker_input_pipes[i][READ_END];

      // check if we can read from the file descriptor
      if (pfd[i].revents != 1) {
        continue;
      }

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

  int epoll_fd =
      epoll_create(num_workers); // the input argument does not matter
  if (epoll_fd < 0) {
    perror("Couldn't create an epoll file descriptor");
    exit(1);
  }

  struct epoll_event* read_events =
      (struct epoll_event*)malloc(sizeof(struct epoll_event) * num_workers);

  // add our file descriptors to the event
  int i;
  for (i = 0; i < num_workers; i++) {
    int pipe_fd = worker_input_pipes[i][READ_END];

    read_events[i].events = EPOLLIN;
    read_events[i].data.fd = pipe_fd;

    int epoll_ctl_result =
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, pipe_fd, &read_events[i]);
    if (epoll_ctl_result != 0) {
      perror("Could not add a file descriptor to the epoll event");
      exit(1);
    }
  }

  // keep looping through epoll events until there are none left to process
  int timeout = 250; // one-half second
  int event_count;
  while ((event_count =
              epoll_wait(epoll_fd, read_events, num_workers, timeout)) > 0) {
    for (i = 0; i < event_count; i++) {
      int read_fd = read_events[i].data.fd;

      double d;
      int read_result = read(read_fd, &d, sizeof(double));
      if (read_result != sizeof(double)) {
        perror("Couldn't read in master from worker");
      }

      f += d;
    }
  }

  if (event_count < 0) {
    perror("Error using epoll function");
    exit(1);
  }

  // lastly, we close the epoll file descriptor
  int close_result = close(epoll_fd);
  if (close_result != 0) {
    perror("Failed to close the epoll file descriptor");
  }

  return f;
}
