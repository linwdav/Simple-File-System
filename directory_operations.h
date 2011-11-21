#ifndef DIRECTORY_OPERATIONS_H
#define DIRECTORY_OPERATIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "block.h"
#include "p6.h"

#define FILE_NUM_PAIRINGS_SIZE (200 + sizeof(int))
#define FILENAME_SIZE 200

int parseAndCreateDirectory(const char * path, unsigned int rename_start_block, char flag);
char * parseRemoveNums(const char * path, unsigned int blockNums[2]);
int findNextBlockNum(char * filename, unsigned int blockNums[2]);
int deleteDirectoryRecursively(unsigned int blockNum);
int deleteFileRecursively(unsigned int blockNum, char * buffer);
int deleteBlock(unsigned int blockNum);
char determineFileType(unsigned int blockNum);
int removeEntry(char * filename, unsigned int blockNum);
int addEntry(char directory[FILENAME_SIZE], unsigned int newBlock, unsigned int * parentBlock);
int updateParentDirectoryNum(char directory[FILENAME_SIZE], unsigned int * parentDirectory);
char * get_parent_blocks(const char * path, unsigned int parent_blocks[2]);
int update_directory_blocks(char * dir, unsigned int blocks[2]);

#endif