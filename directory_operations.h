#ifndef DIRECTORY_OPERATIONS_H
#define DIRECTORY_OPERATIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "block.h"
#include "bitmap_operations.h"

#define FILE_NUM_PAIRINGS_SIZE 204
#define FILENAME_SIZE 200

int parseAndCreateDirectory(const char * path);
char * parseRemoveNums(const char * path, int blockNums[2], char flag);
int findNextBlockNum(char * filename, int blockNums[2]);
void deleteDirectoryRecursively(int blockNum);
void deleteFileRecursively(int blockNum);
void deleteBlock(int blockNum);
char determineFileType(int blockNum);
int removeEntry(char * filename, int blockNum);
int addEntry(char directory[FILENAME_SIZE], int newBlock, int parentBlock);
int updateParentDirectoryNum(char directory[FILENAME_SIZE], int parentDirectory);

#endif