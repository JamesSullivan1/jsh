%.c: %.y
%.c: %.l

CC=gcc
CFLAGS=-g -Wall

all: jsh

jsh: parser.o parser.h scanner.yy.o scanner.yy.h
	$(CC) main.c parser.o parser.h scanner.yy.o scanner.yy.h -o jsh

parser.o: lemonfiles
	$(CC) parser.h parser.c -c -O

scanner.yy.o: flexfiles
	$(CC) scanner.yy.c scanner.yy.h -c -O

.PHONY: lemonfiles
lemonfiles: parser.y
	lemon parser.y -s

.PHONY: flexfiles
flexfiles: scanner.l
	flex --outfile=scanner.yy.c --header-file=scanner.yy.h scanner.l


.PHONY: clean
clean:
	rm -f *.o
	rm -f *.gch
	rm -f scanner.yy.c scanner.yy.h
	rm -f parser.c parser.h parser.out
	rm -f jsh
	rm -rf *.dSYM
