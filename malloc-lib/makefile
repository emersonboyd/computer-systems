CFLAGS=-O0 -Wall -Werror

all: check

default: check

clean:
	$(RM) .DS_Store
	$(RM) .clang-format
	$(RM) helper.o
	$(RM) malloc.o
	$(RM) free.o
	$(RM) calloc.o
	$(RM) realloc.o
	$(RM) memalign.o
	$(RM) main.o
	$(RM) main
	$(RM) t-test1.o
	$(RM) t-test1
	$(RM) libmalloc.so

helper.o: helper.c helper.h
	gcc ${CFLAGS} -c -fpic -o helper.o helper.c

malloc.o: malloc.c helper.h
	gcc ${CFLAGS} -c -fpic -o malloc.o malloc.c

free.o: free.c helper.h
	gcc ${CFLAGS} -c -fpic -o free.o free.c

calloc.o: calloc.c helper.h
	gcc ${CFLAGS} -c -fpic -o calloc.o calloc.c

realloc.o: realloc.c helper.h
	gcc ${CFLAGS} -c -fpic -o realloc.o realloc.c

memalign.o: memalign.c helper.h
	gcc ${CFLAGS} -c -fpic -o memalign.o memalign.c

libmalloc.so: helper.o malloc.o free.o calloc.o realloc.o memalign.o
	gcc ${CFLAGS} -shared -o libmalloc.so helper.o malloc.o free.o calloc.o realloc.o memalign.o

main.o: main.c helper.h
	gcc ${CFLAGS} -c -fpic -o main.o main.c

main: main.o helper.o
	gcc ${CFLAGS} -g -o main main.o helper.o

t-test1.o: t-test1.c
	gcc ${CFLAGS} -c -fpic -o t-test1.o t-test1.c

t-test1: t-test1.o
	gcc ${CFLAGS} -g -o t-test1 t-test1.o -lpthread

restart: restart.c helper.h helper.o
	gcc ${CFLAGS} -g -static \
	-Wl,-Ttext-segment=5000000 -Wl,-Tdata=5100000 -Wl,-Tbss=5200000 \
	-o restart restart.c helper.o

clang-format:
	clang-format -style=mozilla -dump-config > .clang-format

build: clang-format libmalloc.so main t-test1

check: clean build
	LD_PRELOAD=./libmalloc.so ./main

test: clean build
	LD_PRELOAD=./libmalloc.so ./t-test1

dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar
