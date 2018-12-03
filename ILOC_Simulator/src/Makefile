# Makefile for ILOC simulator
# modified by Uli  09/13/06

# CFLAGS=-g 
 CFLAGS=-g -Wall 

CC= gcc

sim:		sim.o machine.o instruction.o hash.o lex.yy.o y.tab.o
		$(CC) $(CFLAGS) -lm -o sim sim.o machine.o instruction.o hash.o lex.yy.o y.tab.o

pure:		sim.o machine.o instruction.o hash.o lex.yy.o y.tab.o
		/usr/site/purify/purify $(CC) $(CFLAGS) -o pure sim.o machine.o instruction.o hash.o lex.yy.o y.tab.o

sim.o:		sim.c instruction.h machine.h sim.h
		$(CC) $(CFLAGS) -lm -c sim.c

machine.o:	machine.c machine.h
		$(CC) $(CFLAGS) -c machine.c

instruction.o:	instruction.c instruction.h hash.h
		$(CC) $(CFLAGS) -c instruction.c

hash.o:		hash.c hash.h
		$(CC) $(CFLAGS) -c hash.c

lex.yy.o:	lex.yy.c
		$(CC) -g -c lex.yy.c

y.tab.o:	y.tab.c
		$(CC) -g -c y.tab.c

lex.yy.c:	iloc.l y.tab.c instruction.h
		lex iloc.l

y.tab.c:	iloc.y instruction.h
		yacc -dtv iloc.y

clean:
		rm *.o
		rm lex.yy.c
		rm y.tab.c
		rm y.tab.h
		rm y.output

depend:
	makedepend -I. *.c

wc:		
		wc iloc.y iloc.l hash.h hash.c instruction.h instruction.c machine.h machine.c sim.h sim.c
# DO NOT DELETE

hash.o: hash.h
