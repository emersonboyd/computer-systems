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
	$(RM) main.o
	$(RM) main
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

libmalloc.so: helper.o malloc.o free.o calloc.o realloc.o
	gcc ${CFLAGS} -shared -o libmalloc.so helper.o malloc.o free.o calloc.o realloc.o

main.o: main.c helper.h
	gcc ${CFLAGS} -c -fpic -o main.o main.c

main: main.o helper.o
	gcc ${CFLAGS} -g -o main main.o helper.o

restart: restart.c helper.h helper.o
	gcc ${CFLAGS} -g -static \
	-Wl,-Ttext-segment=5000000 -Wl,-Tdata=5100000 -Wl,-Tbss=5200000 \
	-o restart restart.c helper.o

clang-format:
	clang-format -style=mozilla -dump-config > .clang-format

build: clang-format libmalloc.so main

check: clean build
	LD_PRELOAD=./libmalloc.so ./main

dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar
