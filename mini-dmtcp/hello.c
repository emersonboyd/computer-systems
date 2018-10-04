#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
  pid_t pid = getpid();
  printf("Process PID: %d\n", pid);

  unsigned int counter;

  // run the program forever
  for (counter = 1; counter >= 0; counter++) {
    printf("Hello count: %d\n", counter);

    int sleep_result = sleep(1);
    if (sleep_result < 0) {
      printf("Unexpected sleep result: %d\n", sleep_result);
      exit(1);
    }
  }

  printf("Done.\n");
  return 0;
}
