# Makefile for ICS612 Project 6: File System 

CC=gcc -g

all: testp6 test_file_operations

testp6: testp6.o p6.o block.o file_operations.o directory_operations.o bitmap_operations.o

test_file_operations: test_file_operations.o p6.o block.o file_operations.o directory_operations.o bitmap_operations.o

clean:
	rm -f testp6 test_file_operations *.o
