#ifndef DIRECTORY_OPERATIONS_H
#define DIRECTORY_OPERATIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "block.h"
#include "p6.h"

#define FILE_NUM_PAIRINGS_SIZE (200 + sizeof(int))
#define FILENAME_SIZE 200

int parseAndCreateDirectory(const char * path, int rename_start_block, char flag);
char * parseRemoveNums(const char * path, int blockNums[2], char flag);
int findNextBlockNum(char * filename, int blockNums[2]);
int deleteDirectoryRecursively(int blockNum);
int deleteFileRecursively(int blockNum);
int deleteBlock(int blockNum);
char determineFileType(int blockNum);
int removeEntry(char * filename, int blockNum);
int addEntry(char directory[FILENAME_SIZE], int newBlock, int parentBlock);
int updateParentDirectoryNum(char directory[FILENAME_SIZE], int parentDirectory);
char * get_parent_blocks(const char * path, int parent_blocks[2]);
int update_directory_blocks(char * dir, int blocks[2]);

#endif