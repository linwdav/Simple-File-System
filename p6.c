/* project 6 file for UHM ICS 612, Spring 2011*/

/* This program is public domain.  As written, it does very little.
 * You are welcome to use it and modify it as you see fit, as long
 * as I am not responsible for anything you do with it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "block.h"
#include "file_operations.h"
#include "directory_operations.h"
#include "bitmap_operations.h"

// Number of blocks on disk - value returned via dev_open()
int num_blocks;

// Array of open files
unsigned int open_files[MAX_OPEN_FILES];
unsigned int open_files_current_position[MAX_OPEN_FILES];

/* open an existing file for reading or writing */
int my_open (const char * path)
{
  int block_num = get_path_block_num(path);

  if (block_num < 0) {
		return -1;
  }
  
  int i;
  for (i = 0; i < MAX_OPEN_FILES; i++) {
    if (open_files[i] == 0) {
      
      // Store block number for file in open file array
      open_files[i] = block_num;
      
      // Reset current position of file to the start of the file.
      open_files_current_position[i] = HEADER_SIZE;
      
      return i;
    } // End if
  } // End for
  
  // Return -1 if no open slot in the open file list.
  return -1;
}

/* open a new file for writing only */
int my_creat (const char * path)
{
  int fd;
  
  // Get next free block
  unsigned int file_block_num = requestNextFreeBlock();

  // Flip bit in free block list
  setBlockInBitmapToStatus (1, file_block_num);
  
  // Split path into directory path and filename
  char *separator = strrchr(path, PATH_SEPARATOR);
  char filename_buffer[MAX_FILE_NAME_LENGTH];
  char path_buffer[strlen(path) + 1];
  
  strcpy(filename_buffer, separator + 1);
  strncpy(path_buffer, path, separator - path);
  path_buffer[separator - path] = '\0';

  // Get directory file block number
  int directory_block_num = get_path_block_num(path_buffer);
  
  // If directory does not exist, then set return value to -1 (error)
  if (directory_block_num < 0) {
    return -1;
  }
  
  // Append filename to directory file 
  // Must search directory to ensure doesn't file doesn't already exist
  if (add_entry_to_directory(directory_block_num, filename_buffer, file_block_num) < 0) {
    return -1;
  }
  
  // Add to open file array
  fd = my_open(path);
  
  // Write file header information
  char header_buffer[BLOCKSIZE];
  populate_file_header(header_buffer, ROOT_BLOCK, HEADER_SIZE, 'f');
  write_block(file_block_num, header_buffer);
  
  return fd;
} // end my_creat

/* sequentially read from a file */
int my_read (int fd, void * buf, int count)
{
	// Ptr to track where we are in buf
	char * buf_ptr = buf;
	
  // Returns number of bytes read.
  int bytes_read = 0;
  
  // Total number of bytes available to store data in each block.
  int valid_bytes_in_block = BLOCKSIZE - HEADER_SIZE;
  
  unsigned int current_block_num = open_files[fd];
  unsigned int current_position = open_files_current_position[fd];
  
  // Check to ensure that the file descriptor is valid
  if (current_block_num == 0) {
    // If file is not open, then return error value.
		perror("my_read error: file descriptor invalid");
		return -1;
  }
  if (fd < 0) {
		perror("my_read error: file not open");
		return -1;
  }

  // Header file information containers
  unsigned int next_block;
  unsigned short bytes_allocated;
  char buffer[BLOCKSIZE];  

  int exit_loop = 0;
  
  // Find the block where the current position is.
  while (current_block_num != 0 && current_position > 0 && exit_loop == 0) {

    // Read in header data from current block number
  	get_file_block_and_header_information(buffer, current_block_num, &next_block, &bytes_allocated);

    // If the bytes allocated in this block is more than the current position, then this is the block
    // to start reading at.
    if (bytes_allocated >= current_position) {
	  	exit_loop = 1;
    }	
    else {
			current_position -= bytes_allocated;
			current_block_num = next_block;
    }
  } // End while

  // If the loop did not exit properly, then this means current position is incorrect (too large for file)
  if (exit_loop == 0) {
		return -1;
  }
  // start reading at the current_position in the current block.
  
  // Read in header data from current block number
 	get_file_block_and_header_information(buffer, current_block_num, &next_block, &bytes_allocated);
	
	// Data remaining between current position and end of block
	int data_bytes_remaining;
	
	if (current_position >= HEADER_SIZE) {
		data_bytes_remaining = bytes_allocated - current_position;
	}
	else {
		data_bytes_remaining = bytes_allocated - current_position - HEADER_SIZE;
	}
	
  // Loop until we get to the last block to read from
  while (current_block_num != 0 && count >= data_bytes_remaining) {
	
	  // Calculate how many bytes are read in this block
		int bytes_read_this_block = bytes_allocated - current_position;
		
		if (current_position < HEADER_SIZE) {
			current_position = HEADER_SIZE;
		}  
	
	  // Read all bytes in starting at current_position until the end of the block.
		memcpy(buf_ptr, &buffer[current_position], data_bytes_remaining);
		
		// update buf_ptr and open_files_current_position
		buf_ptr += data_bytes_remaining;
		open_files_current_position[fd] += bytes_read_this_block;
		bytes_read += data_bytes_remaining;
		
		// Need to decrement the byte count to read. 
		count -= data_bytes_remaining;
		
		// Go to next block
		current_block_num = next_block;
		
		// Reset position ptr in block to the start of the next block.
		current_position = 0;
		
		// Load information from next block as long as more bytes need to be read in and we're not at the last
		// block in the file.
		if (current_block_num != 0 && count > 0) {
		  get_file_block_and_header_information(buffer, current_block_num, &next_block, &bytes_allocated);
 	  } 
		data_bytes_remaining = bytes_allocated - HEADER_SIZE;
  } // End while

  // Handle remainder of bytes
  if (count > 0) {
		int new_current_position = current_position;
		
		if (current_position < HEADER_SIZE) {
			new_current_position = HEADER_SIZE;
		}
	  
	  // Read all bytes in starting at current_position until the end of count
		memcpy(buf_ptr, &buffer[new_current_position], count);
		
		// update bytes_read and open_files_current_position
		if (current_position < HEADER_SIZE) {
		  open_files_current_position[fd] += count + HEADER_SIZE;
	  }
	  else {
			open_files_current_position[fd] += count;
	  }
		bytes_read += count;
  }  

	return bytes_read;
}

/* sequentially write to a file */
int my_write (int fd, const void * buf, int count)
{
  int block_num = open_files[fd];
  int pointer = open_files_current_position[fd];
  int num_blocks = pointer / BLOCKSIZE;
  int remainder = pointer % BLOCKSIZE;

  // The block in which the pointer is pointing.
  if ((block_num = find_block_to_write_to(block_num, num_blocks)) < 0) {
	return -1;
  }

  if (write_to_block(buf, block_num, remainder, count) < 0) {
	return -1;
  }

  // count is just the number of data bytes, current position also has to account for any
  // new headers created when writing to the file.

  open_files_current_position[fd] += count;

  int whole_blocks = count - (BLOCKSIZE - remainder); // Minus off amount written to partial block.
  if (whole_blocks < 0) {
	return count;
  }
  num_blocks = (whole_blocks / (BLOCKSIZE - HEADER_SIZE)) + 1;
  int header_amount = num_blocks * HEADER_SIZE;
  open_files_current_position[fd] += header_amount;

  return count;
}

int my_close (int fd)
{
  // Remove from open file list if there is a valid block number.
  if (open_files[fd] != 0) {
    open_files[fd] = 0;
    return 0;
  }
  
  // If no file block number in the list at the fd index, return error code.
  return -1;
}

int my_remove (const char * path)
{
  int block_nums[2];
  char * filename;

  // Get the filename of file to remove as well as block number of directory file is in, and
  // the root block number of the file as well.
  filename = parseRemoveNums(path, block_nums);
  if (block_nums[1] == -1) {
	return -1;
  }

  // If this file/directory is the last one in the directory block have to delete directory block
  // and redo next block pointers.
  int parent_blocks[2];
  char * dir = malloc((FILENAME_SIZE + 1) * sizeof(char));

  if ((dir = get_parent_blocks(path, parent_blocks)) == NULL) {
	return -1;
  }
  
  if (deleteFileRecursively(block_nums[1]) < 0) {
	return -1;
  }

  // Remove the file's entry from the directory.
  if (removeEntry(filename, block_nums[0]) < 0) {
	return -1;
  }
  
  if (update_directory_blocks(dir, parent_blocks) < 0) {
	return -1;
  }
  
  // Remove the file from the open list if it's in there.
  close_file_if_open(block_nums[1]);

  return 0;
}

int my_rename (const char * old, const char * new)
{
  int blockNums[2];
  char * oldFile;

  // find the block number of where the old entry is.
  oldFile = parseRemoveNums(old, blockNums);

  if (blockNums[1] == -1) {
    return -1;
  }

  // find the block number of where the new entry should go and add it
  // do this first so that if it errors out haven't removed the entry yet.
  if (parseAndCreateDirectory(new, blockNums[1], 'r') < 0) {
	return -1;
  }

  // remove it.
  if (removeEntry(oldFile, blockNums[0]) < 0) {
	return -1;
  }

  return 0;
}

/* only works if all but the last component of the path already exists */
int my_mkdir (const char * path)
{
  // Second and third arguments only necessary if renaming a directory not creating a new one.
  if (parseAndCreateDirectory(path, 0, 'c') < 0) {
	return -1;
  }

  return 0;
}

int my_rmdir (const char * path)
{
  int blockNums[2];
  char * filename;

  filename = parseRemoveNums(path, blockNums);
  if (blockNums[1] == -1 || filename == NULL) {
    return -1;
  }

  int parent_blocks[2];
  char * dir;
  if ((dir = get_parent_blocks(path, parent_blocks)) == NULL) {
	return -1;
  }

  if (determineFileType(blockNums[1]) != 'd') {
	return -1;
  }

  if (deleteDirectoryRecursively(blockNums[1]) < 0) {
	return -1;
  }

  // Remove entry from parent directory.
  if (removeEntry(filename, blockNums[0]) < 0) {
	return -1;
  }

  if (update_directory_blocks(dir, parent_blocks) < 0) {
	return -1;
  }
  
  return 0;
}

/* check to see if the device already has a file system on it,
 * and if not, create one. */
void my_mkfs ()
{
  
  /* ALL OPERATIONS BELOW ASSUME THAT DISK HAS BEEN 
     INITIALIZED TO CONTAIN ALL 0s */

  // Buffer to be used to write file system information to disk.
  unsigned char buffer[BLOCKSIZE];
  
  // Attempt to open the disk
  if ((num_blocks = dev_open()) < 0) {
    printf("Disk does not exist or disk is corrupt.\n");
    exit(1); 
  }
  
  // Read in root block
  if (read_block (ROOT_BLOCK, buffer) < 0) {
    printf("Error reading root block\n");
    exit(1);
  }
  
  // Check block 0 to see if root directory exists (first byte on block 0 == 'd')
  if (buffer[0] != 'd') {
  
    // initialize open files list to all 0s.
    int i;
    for(i = 0; i < MAX_OPEN_FILES; i++) {
      open_files[i] = 0;
    }
  
    // If not create file system.
    // Create root directory at block 0.
    buffer[0] = 'd';
    
    // Next block field will always be initialized to 0
    int next_block = 0;
    
    // Number of used bytes (only header so far)
    short header_size = HEADER_SIZE;
    
    // since buffer is immutable, create a pointer to it.
    char * buffer_ptr = buffer;

    // Write header information (next block and remainder fields)
    memcpy(++buffer_ptr, &next_block, sizeof(next_block));
    memcpy((buffer_ptr+sizeof(next_block)), &header_size, sizeof(header_size));
    
    // Write header information to disk at the root block
    write_block(ROOT_BLOCK, buffer);
    
    // Create free list bitmap
    
    // number of blocks needed for bitmap
    int bitmap_num_blocks;
    
    // Determine how many blocks the free list bitmap will fit in.
    if (num_blocks % BITS_IN_BLOCK == 0) {
      bitmap_num_blocks = num_blocks / BITS_IN_BLOCK;
    }
    else {
      bitmap_num_blocks = (num_blocks / BITS_IN_BLOCK) + 1;
    }
    
    // Write the end block of the bitmap to the free list parameters
    int end_bitmap_block = FREE_LIST_BITMAP_START + bitmap_num_blocks - 1;
    
    // Clear buffer
    memset(buffer, 0, HEADER_SIZE);
    memcpy(buffer, &end_bitmap_block, sizeof(end_bitmap_block));
    
    write_block(FREE_LIST_BITMAP_PARAMS, buffer);
    
    // Allocate 1's to all system blocks and bitmap_num_blocks in the free
    // list bitmap
    
    // The number of blocks that are never free go from 0 -> (never_free - 1)
    int never_free = NUM_SYSTEM_BLOCKS + bitmap_num_blocks;
    
    // how many blocks the never_free bits will take up.
    int never_free_blocks;
     
    // remainder = how many bits of never_free section of free list bitmap
    // would be in the last block  
    int remainder = never_free % BLOCKSIZE; 
    if (remainder == 0) {
      never_free_blocks = never_free / BLOCKSIZE;
    }
    else if (never_free > BLOCKSIZE){
      never_free_blocks = (never_free / BLOCKSIZE) + 1;
    }
    else {
      never_free_blocks = 0;
    }
    
    // Set buffer to all 1s (entire block of all 1s)
    memset(buffer, 0xFF, BLOCKSIZE);
    
    // Write all 1s to as many full blocks as necessary starting
    // at the first block containing the free list bitmap
    for(i = 0; i < never_free_blocks; i++) {
      write_block(i + FREE_LIST_BITMAP_START, buffer);
    }    
    
    // put as many 1s as 'remainder' in the last block of 
    // the free list bitmap blocks.
    if (remainder != 0) {
      // Set buffer to all 0s (entire block of all 0s)
      memset(buffer, 0, BLOCKSIZE);
      
      // Find remainder and dividend to calculate how many
      // bytes are needed and the remainder.
      int remainder_of_remainder = remainder % 8;
      int dividend_of_remainder = remainder / 8;
      
      int j;
      // Write as many full bytes of 1s as necessary
      for (j = 0; j < dividend_of_remainder; j++) {
        buffer[j] = 0xFF;
      }
      
      // Write the remaining bits
      
      // Set all bits in last byte to all 1s
      buffer[j] = 0xFF;
      
      // Shift in zeros from the right
      buffer[j] = buffer[j] << (8 - remainder_of_remainder);

    } // End if
    
    // Write the next block (this is the last block of the free list bitmap)  
    write_block(i + FREE_LIST_BITMAP_START, buffer);

    printf("File system created.\n");
  }
  else {
    printf("File system already exists.\n");
  }

} // End my_mkfs

/* Needed for when deleting a file.  Don't want the file to remain in the open list.
 * blockNum - The block number of the file that's being deleted.
 */
void close_file_if_open(int block_num) {
	int i;
	for (i = 0; i < MAX_OPEN_FILES; i++) {
	  if (open_files[i] == block_num) {
		  open_files[i] = 0;
	  }
	}
}

int get_associated_block_num(int fd) {
  return open_files[fd];
}