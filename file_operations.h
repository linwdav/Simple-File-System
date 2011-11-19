#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p6.h"
#include "block.h"
#include "directory_operations.h"

int get_file_block_and_header_information(char * buffer, unsigned int file_block_num, unsigned int *next_block, unsigned short * bytes_allocated);
int add_entry_to_directory(unsigned int directory_block_num, char * entry_name, unsigned int entry_block_num);
void populate_file_header(char * buffer, unsigned int next_block, unsigned short bytes_allocated, char flag);
int search_directory_block_for_name(char * name, unsigned int directory_block_num);
int get_path_block_num (const char * path);
int get_path_block_num_recursive (char * path, unsigned int current_directory_block_num);
int find_block_to_write_to(int start_block, int num_blocks);
int write_to_block (const void * buf, int block_num, int pointer, int amount);
int initialize_header(int block_num, char flag);

extern unsigned int open_files[];
extern unsigned int open_files_current_position[];

#endif