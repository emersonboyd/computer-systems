CFLAGS=-O0 -Wall -Werror

all: check

default: check

clean-ckpt:

clean:
	$(RM) helper.o
	$(RM) helper
	$(RM) worker.o
	$(RM) worker
	$(RM) master.o
	$(RM) master

helper.o: helper.h helper.c
	gcc $(CFLAGS) -c -fpic -o helper.o helper.c

worker.o: helper.h worker.c
	gcc $(CFLAGS) -c -fpic -o worker.o worker.c

worker: helper.o worker.o
	gcc $(CFLAGS) -g -o worker worker.o helper.o -lm

master.o: helper.h master.c
	gcc $(CFLAGS) -c -fpic -o master.o master.c

master: helper.o worker master.o
	gcc $(CFLAGS) -g -o master master.o helper.o -lm

clang-format:

build: master

check: clean build

run_default: run_sequential

run_sequential: check
	./master --worker_path ./worker --num_workers 5 --wait_mechanism sequential -x 2 -n 12

run_select: check
	./master --worker_path ./worker --num_workers 5 --wait_mechanism select -x 2 -n 12

run_poll: check
	./master --worker_path ./worker --num_workers 5 --wait_mechanism poll -x 2 -n 12

run_epoll: check
	./master --worker_path ./worker --num_workers 5 --wait_mechanism epoll -x 2 -n 12

dist:
