#include "file_operations.h"

int search_directory_for_name(char * name, int directory_block_num) {
   // Return value
   int block_num_of_name;
   
   // Get contents of directory
   char buffer[BLOCKSIZE];
   
   // If an invalid block number, return -1
   if (read_block(directory_block_num, buffer) < 0) {
     return -1;
   }
   
   // Directory information
   unsigned short allocated_bytes;
   unsigned int next_block;
   
   char * buffer_ptr = buffer;
  
   // Get directory information
   memcpy(&next_block, buffer_ptr + 1, BYTES_IN_INT);
   memcpy(&allocated_bytes, buffer_ptr + 5, BYTES_IN_SHORT);
   
   
} // end search_directory_for_name

/* 
 * Returns the block number for a given directory.  Example: /foo/bar/hello.txt
 */
int get_path_block_num (char * path, int current_directory_block_num) {
  
  // Search for next separator
  char *ptr_next_separator = strchr(path + 1, PATH_SEPARATOR);
  
  // split out next directory/file name
  char next_name[MAX_FILE_NAME_LENGTH];
  
  // There are more directories to traverse
  if (ptr_next_separator) {

    strncpy(next_name, path + 1, ptr_next_separator - path);
    
    int next_directory_block_num = 
      search_directory_for_name(next_name, current_directory_block_num);
    
    return get_path_block_num(ptr_next_separator, next_directory_block_num);
  } // End if
  
  // This is the last file/directory 
  else {
    strncpy(next_name, path + 1, strlen(path));
    next_name[strlen(path)] = '\0';
    
    // Search for the file/directory in the current directory block
    return search_directory_for_name(next_name, current_directory_block_num);
  } // End else
} // End get_directory_block_num