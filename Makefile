# Makefile for ICS612 Project 6: File System 

CC=gcc -g

testp6: testp6.o p6.o block.o

clean:
	rm -f testp6 *.o