CFLAGS=-O0 -Wno-deprecated-declarations -Wall -Werror -pthread

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
	$(RM) malloc_stats.o
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

malloc_stats.o: malloc_stats.c helper.h
	gcc ${CFLAGS} -c -fpic -o malloc_stats.o malloc_stats.c

lib: helper.o malloc.o free.o calloc.o realloc.o memalign.o malloc_stats.o
	gcc ${CFLAGS} -shared -o libmalloc.so helper.o malloc.o free.o calloc.o realloc.o memalign.o malloc_stats.o

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
	clang-format -i helper.h
	clang-format -i helper.c
	clang-format -i malloc.c
	clang-format -i free.c
	clang-format -i calloc.c
	clang-format -i realloc.c
	clang-format -i memalign.c
	clang-format -i malloc_stats.c
	clang-format -i main.c

build: clang-format lib main t-test1

check: clean build
	LD_PRELOAD=./libmalloc.so ./main

test: clean build
	LD_PRELOAD=./libmalloc.so ./t-test1

dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar
