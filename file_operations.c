#include "file_operations.h"

/* Populates a buffer with the given file header information */
void populate_file_header(char * buffer, int next_block, int bytes_allocated, char flag) {
  memset(buffer, 0, BLOCKSIZE);
  char *buffer_ptr = buffer;
  
  /* Populate type of file 'f' for file, 'd' for directory */
  *buffer_ptr = flag;
  
  /* Fill in next block num */
  buffer_ptr++;
  memcpy(buffer_ptr, &next_block, BYTES_IN_INT);
  
  // Go to bytes allocated portion
  buffer_ptr += BYTES_IN_INT;
  memcpy(buffer_ptr, &bytes_allocated, BYTES_IN_INT);
} // End populate_file_header

/* Search a given directory block for a specified name.  Continue to the next
 * block in the directory or return -1 if nothing is found and the entire
 * directory has been searched.
 */
int search_directory_block_for_name(char * name, int directory_block_num) {
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
   memcpy(&allocated_bytes, buffer_ptr + 1 + BYTES_IN_INT, BYTES_IN_SHORT);
   
   // Ensure that there are file listings in this block
   if ((allocated_bytes - HEADER_SIZE) % FILE_NUM_PAIRINGS_SIZE != 0) {
     return -1;
   }
   // Determine how many entries are in this directory block. 
   int num_iterations = (allocated_bytes - HEADER_SIZE) / FILE_NUM_PAIRINGS_SIZE;
      
   int i;
   buffer_ptr = buffer + HEADER_SIZE;
   for (i = 0; i < num_iterations; i++) {
     
     // Look at each entry.  If it matches, then return the block number
     if (!strcmp(name, buffer_ptr)) {
       memcpy(&block_num_of_name, buffer_ptr + MAX_FILE_NAME_LENGTH, BYTES_IN_INT);
       return block_num_of_name;
     }
     // If no match, then go to the next one.
     buffer_ptr += FILE_NUM_PAIRINGS_SIZE;
   }
   
   // If not found, go to the next block
   if (next_block > 0) {
     return search_directory_block_for_name(name, next_block); 
   }
   // If this is the last block, then the entry is not in this directory.
   else {
     return -1;
   }
} // end search_directory_for_name

int get_path_block_num (const char * path) {
  // Copy path into a buffer
  char path_buffer[strlen(path) + 1];
  strcpy(path_buffer, path);
  
  int last_index = strlen(path_buffer) - 1;
  
  // Strip off any separators at the end of the path
  if (path_buffer[last_index] == PATH_SEPARATOR) {
    path_buffer[last_index] = '\0';
  }
    
  // Strip off the leading separator (root directory)
  char * path_ptr = path_buffer;
  
  if (path_buffer[0] == PATH_SEPARATOR) {
    path_ptr++;
  }
  
  // Start recursive search for block number from root
  return get_path_block_num_recursive(path_ptr, ROOT_BLOCK);
} // end get_path_block_num

/* 
 * Returns the block number for a given directory.  Example: /foo/bar/hello.txt
 */
int get_path_block_num_recursive (char * path, int current_directory_block_num) {
  
  // Search for next separator
  char *ptr_next_separator = strchr(path, PATH_SEPARATOR);
  
  // split out next directory/file name
  char next_name[MAX_FILE_NAME_LENGTH];
  
  // There are more directories to traverse
  if (ptr_next_separator) {

    // Copy the next directory in
    strncpy(next_name, path, ptr_next_separator - path);
    next_name[ptr_next_separator - path] = '\0';
    
    int next_directory_block_num = 
      search_directory_block_for_name(next_name, current_directory_block_num);
    
    return get_path_block_num_recursive(++ptr_next_separator, next_directory_block_num);
  } // End if
  
  // This is the last file/directory 
  else {
    // Search for the file/directory in the current directory block
    return search_directory_block_for_name(path, current_directory_block_num);
  } // End else
} // End get_directory_block_num

/* Find the block that is going to be written too.
 * start_block - The first block in the file.
 * num_blocks - The number of blocks you have to go into the file to get to where the current
 * pointer is pointing to.
 */
int find_block_to_write_to(int start_block, int num_blocks) {
	int current_block = start_block;
	char * buffer_ptr;
	int i;
	for (i = 0; i < num_blocks; i++) {
	  char buffer[BLOCKSIZE];
	  if (read_block(current_block, buffer) < 0) {
		printf("Error reading block #%d\n", current_block);
		exit(1);
	  }

	  buffer_ptr = buffer + 1;
	  memcpy(&current_block, buffer_ptr, sizeof(current_block));

	  // Something is wrong with the pointer, said to go in n blocks but at this point there
	  // is no more blocks to continue too.
	  if (current_block == 0) {
		return -1;
	  }
	}

	return current_block;
}

/* Writes to the block given by block_num.  
 * buffer - What to write to the block.
 * block_num - Block to write to.
 * pointer - Place in block to begin writing too.
 * amount - The amount to write.
 */
int write_to_block(const void * buf, int block_num, int pointer, int amount) {
  char buffer[BLOCKSIZE];

  if (read_block(block_num, buffer) < 0) {
	printf("Error reading to block #%d\n", block_num);
	return -1;
  }

  char * buffer_ptr = buffer + 1;
  char * buf_ptr = (char *)buf;

  int next_block;
  memcpy(&next_block, buffer_ptr, sizeof(next_block));

  short bytes_allocated;
  memcpy(&bytes_allocated, buffer_ptr + sizeof(next_block), sizeof(bytes_allocated));

  int extraBlocks = 0;
  int amount_written_to_block = amount;

  // If the amount to write is greater than what's left in the block.
  if ((BLOCKSIZE - pointer) < amount) {
	// Only request another block if this one will get filled up and need more.
	if (next_block == 0) {
	  if ((next_block = requestNextFreeBlock()) < 0) {
		return -1;
	  }
	  setBlockInBitmapToStatus(1, next_block);

	  // Initialize the header in the new block.
	  if (initialize_header(next_block, 'f') < 0) {
		return -1;
	  }

	  buffer_ptr = buffer + 1;
	  memcpy(buffer_ptr, &next_block, sizeof(next_block));
    }

	amount_written_to_block = BLOCKSIZE - pointer;
	extraBlocks = 1;
	// For every multiple of 1024 need another block for writing.
	extraBlocks += (amount - (BLOCKSIZE - pointer)) / BLOCKSIZE;
  }

  buffer_ptr = buffer + pointer;
  strncpy(buffer_ptr, buf_ptr, (BLOCKSIZE - pointer));

  // Trying to write in past the allocated amount
  if (pointer > bytes_allocated) {
	return -1;
  }
  // writing into a point in a block that has already been written.
  else if (pointer < bytes_allocated) {
	// What is being written is writing into an unallocated portion of the block.
    if ((pointer + amount) > bytes_allocated) {
      bytes_allocated += amount_written_to_block - (bytes_allocated - pointer);
	}
  }
  // pointer == bytes_allocated, writing at end of allocation
  else {
	bytes_allocated += amount_written_to_block;
  }

  buffer_ptr = buffer + 1 + sizeof(next_block);
  memcpy(buffer_ptr, &bytes_allocated, sizeof(bytes_allocated));

  // Regardless of if there is more to write, write back this block because it is either full or
  // done writing.
  if (write_block(block_num, buffer) < 0) {
	printf("Error writing to block #%d\n", block_num);
	return -1;
  }

  int i;
  int track_amount = amount - amount_written_to_block;
  buf = buf + amount_written_to_block;
  int amount_to_write;
  int current_block = next_block;

  // loop through for the necessary amount of blocks to hold the data.
  for (i = 0; i < extraBlocks; i++) {
	if (read_block(current_block, buffer) < 0) {
	  printf("Error reading to block #%d\n", next_block);
	  return -1;
    }

	buffer_ptr = buffer + 1;
	memcpy(&next_block, buffer_ptr, sizeof(next_block));
	memcpy(&bytes_allocated, buffer_ptr + sizeof(next_block), sizeof(bytes_allocated));

	// The amount to write to the block.
	amount_to_write = track_amount;

	// There is more than one blocks worth of data to be written.
	if ((BLOCKSIZE - HEADER_SIZE)  < amount_written_to_block) {
	  amount_to_write = BLOCKSIZE - HEADER_SIZE;
	}

	// Update the amount still left to be written.
    track_amount = track_amount - amount_to_write;

	buffer_ptr = buffer + HEADER_SIZE;
	strncpy(buffer_ptr, buf_ptr, amount_to_write);

	// The amount written to the block exceeded the amount previously allocated on block.
	if (bytes_allocated < (HEADER_SIZE + amount_to_write)) {
		bytes_allocated = HEADER_SIZE + amount_to_write;

		buffer_ptr = buffer + 1 + sizeof(next_block);
		memcpy(buffer_ptr, &bytes_allocated, sizeof(bytes_allocated));
	}

	// Still more to write but next block is 0.
	if (next_block == 0 && track_amount > 0) {
	  if ((next_block = requestNextFreeBlock()) < 0) {
		return -1;
	  }
	  setBlockInBitmapToStatus(1, next_block);

	  if (initialize_header(next_block, 'f') < 0) {
		return -1;
	  }

	  buffer_ptr = buffer + 1;
	  memcpy(buffer_ptr, &next_block, sizeof(next_block));
	}

	if (write_block(current_block, buffer) < 0) {
      printf("Error writing block #%d\n", current_block);
	  return -1;
	}

	current_block = next_block;
	buf = buf + amount_to_write;
  }

  return 0;
}

/* Initializes the header for a new block.
 * block_num - The new block to initialize the header for.
 * flag - whether file or directory.
 */
int initialize_header(int block_num, char flag) {
  char buffer[BLOCKSIZE];
  char * buffer_ptr;

  // Read in the new block.
  if (read_block(block_num, buffer) < 0) {
	printf("Error reading to block #%d\n", block_num);
	return -1;
  }

  // Set first byte
  buffer[0] = flag;
  buffer_ptr = buffer + 1;

  // No next block yet.
  int next_block = 0;
  memcpy(buffer_ptr, &next_block, sizeof(next_block));

  // Default bytes allocated is always the size of the header.
  short bytes_allocated = HEADER_SIZE;
  memcpy(buffer_ptr + sizeof(next_block), &bytes_allocated, sizeof(bytes_allocated));

  if (write_block(block_num, buffer) < 0) {
	printf("Error writing to block #%d\n", block_num);
	exit(1);
  }

  return 0;
}