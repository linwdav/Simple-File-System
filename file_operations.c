#include "file_operations.h"

/*
 * Requests the next free block from the free block list.
 */
int get_next_free_block() {
  // TO DO
  static int next_free_block = 50;
  next_free_block += 100;
  return next_free_block;
}

/*
 * given a list of block numbers, free them in the free block list
 */
int free_blocks(int n, int *listOfBlocks) {
  // TO DO
  return 0;
}

/*
 * Sets a given block as either free (status = 0) or taken (status = 1)
 * within the free block bitmap 
 */
int set_block_in_bitmap_to_status(int status, int blockNumber) {
  // TO DO
  return 0;
}

/* 
 * Returns the block number for a given directory
 */
int get_directory_block_num (char * path, int current_directory_block_num) {
  // Get contents of directory at beginning of path
  char buffer[BLOCKSIZE];
  
  // If an invalid block number, return -1
  if (read_block(current_directory_block_num, buffer) < 0) {
    return -1;
  }
    
  // Search for next separator
  char *ptr_next_separator = strchr(path + 1, PATH_SEPARATOR);
  
  if (ptr_next_separator) {
    int next_directory_block_num;
    
    // split out next directory name
    // search through buffer for directory name
    // look for delimiter
    // look for next newline character
    // assign block number after delimiter to next_directory_block_num
    
    return get_directory_block_num(ptr_next_separator, next_directory_block_num);
  }
  else {
    // If this is the last directory, then return this block number
    return current_directory_block_num;
  }
} // end get_directory_block_num