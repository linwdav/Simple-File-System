#include "file_operations.h"

/* 
 * Returns the block number for a given directory
 */
int get_path_block_num (char * path, int current_directory_block_num) {
  // Get contents of directory at beginning of path
  char buffer[BLOCKSIZE];
    
  // Search for next separator
  char *ptr_next_separator = strchr(path + 1, PATH_SEPARATOR);
  
  if (ptr_next_separator) {
    int next_directory_block_num;
    
    // split out next directory name
    // search through buffer for directory name
    
    // If an invalid block number, return -1
    if (read_block(current_directory_block_num, buffer) < 0) {
      return -1;
    }
    
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