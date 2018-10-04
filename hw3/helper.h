#ifndef EBOYD_HELPER_H
#define EBOYD_HELPER_H

#define READ_END 0
#define WRITE_END 1
#define max(x,y) ((x) >= (y)) ? (x) : (y)

char* to_char_array(int number);

double read_sequential(int** worker_input_pipes, int num_workers, int n);
double read_select(int** worker_input_pipes, int num_workers);
double read_poll(int** worker_input_pipes, int num_workers);

#endif
