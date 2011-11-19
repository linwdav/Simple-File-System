#ifndef TEST_FILE_OPERATIONS_H
#define TEST_FILE_OPERATIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "block.h"
#include "p6.h"
#include "file_operations.h"
#include "directory_operations.h"

void test_file_operations_setup();
void test_get_path_block_num();
void test_search_directory_block_for_name();
void test_my_creat();
void test_my_open();
void test_my_close();
void test_my_read();
void test_my_write();

#endif