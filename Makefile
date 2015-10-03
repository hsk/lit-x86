CFLAGS = -m32 -O0 -std=c99
CC = clang  $(CFLAGS) 

lit: main.o lit.o asm.o expr.o parse.o stdfunc.o

main.o: main.c
	$(CC) -c main.c

lit.o: lit.h lit.c
	$(CC) -c lit.c

asm.o: asm.h asm.c
	$(CC) -c asm.c

expr.o: expr.h expr.c
	$(CC) -c expr.c

parse.o: parse.h parse.c
	$(CC) -c parse.c

stdfunc.o: stdfunc.h stdfunc.c
	$(CC) -c stdfunc.c

clean:
	$(RM) a.out lit *.o

