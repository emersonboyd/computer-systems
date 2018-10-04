CFLAGS=-O0 -Wall -Werror
NUM_WORKERS=5
X=2
N=12

all: check

default: check

clean-ckpt:

clean:
	$(RM) .clang-format
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
	clang-format -style=mozilla -dump-config > .clang-format
	clang-format -i helper.h
	clang-format -i helper.c
	clang-format -i worker.c
	clang-format -i master.c

build: clang-format master

check: clean build

run_default: run_sequential

run_sequential: check
	./master --worker_path ./worker --num_workers $(NUM_WORKERS) --wait_mechanism sequential -x $(X) -n $(N)

run_select: check
	./master --worker_path ./worker --num_workers $(NUM_WORKERS) --wait_mechanism select -x $(X) -n $(N)

run_poll: check
	./master --worker_path ./worker --num_workers $(NUM_WORKERS) --wait_mechanism poll -x $(X) -n $(N)

run_epoll: check
	./master --worker_path ./worker --num_workers $(NUM_WORKERS) --wait_mechanism epoll -x $(X) -n $(N)

dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar

