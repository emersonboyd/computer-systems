#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "helper.h"

int main() {
int lock_result = pthread_mutex_lock(&BASE_MUTEX);
assert(lock_result == 0, __FILE__, __LINE__);
char *msg = "Hi, I'm a child writing with a lock.\n";
write(STDOUT_FILENO, msg, strlen(msg) + 1);
int unlock_result = pthread_mutex_unlock(&BASE_MUTEX);
assert(unlock_result == 0, __FILE__, __LINE__);

return 0;
}