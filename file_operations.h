#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p6.h"
#include "block.h"

int get_path_block_num (char * path, int current_directory_block_num);
int find_block_to_write_to(int start_block, int num_blocks);
int write_to_block (const void * buf, int block_num, int pointer, int amount);
int initialize_header(int block_num, char flag);
#endif