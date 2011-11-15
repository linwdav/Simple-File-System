#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p6.h"
#include "block.h"

int get_path_block_num (char * path, int current_directory_block_num);

#endif