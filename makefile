CC     = gcc

default : main

#-------------------------------------------------------------------------------
main : makefile sinbordes.c pgm.o
	$(CC) sinbordes.c pgm.o -o sinbordes

pgm.o : pgm.c pgm.h
	$(CC) -c -o pgm.o pgm.c

#-------------------------------------------------------------------------------
clean:
	rm -f *.o *~ *.bck *.bak sinbordes
