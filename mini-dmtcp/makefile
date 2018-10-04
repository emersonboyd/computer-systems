CFLAGS=-O0 -Wall -Werror -fno-stack-check -fno-stack-protector

all: check

default: check

clean-ckpt:
	$(RM) myckpt

clean: clean-ckpt
	$(RM) .DS_Store
	$(RM) .clang-format
	$(RM) hello
	$(RM) hello.o
	$(RM) helper.o
	$(RM) ckpt.o
	$(RM) ckpt
	$(RM) restart
	$(RM) libckpt.so	

hello.o: hello.c
	gcc $(CFLAGS) -c -fpic -o hello.o hello.c

hello: hello.o
	gcc $(CFLAGS) -g -o hello hello.o

helper.o: helper.c helper.h
	gcc ${CFLAGS} -c -fpic -o helper.o helper.c

ckpt.o: ckpt.c helper.h
	gcc ${CFLAGS} -c -fpic -o ckpt.o ckpt.c

libckpt.so: ckpt.o helper.o
	gcc ${CFLAGS} -shared -o libckpt.so ckpt.o helper.o

restart: restart.c helper.h helper.o
	gcc ${CFLAGS} -g -static \
	-Wl,-Ttext-segment=5000000 -Wl,-Tdata=5100000 -Wl,-Tbss=5200000 \
	-o restart restart.c helper.o

res: restart
	./restart myckpt

gdb:
	gdb --args ./restart myckpt

clang-format:
	clang-format -style=mozilla -dump-config > .clang-format
	clang-format -i hello.c
	clang-format -i helper.h
	clang-format -i helper.c
	clang-format -i ckpt.c
	clang-format -i restart.c

build: clang-format libckpt.so hello restart

check: clean-ckpt build
	(sleep 4 && kill -12 `pgrep -n hello` && sleep 3 && pkill -9 -n hello) & 
	(LD_PRELOAD=./libckpt.so ./hello) &
	(sleep 8 && make res)

dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar
