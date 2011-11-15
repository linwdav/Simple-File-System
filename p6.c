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

// Number of blocks on disk - value returned via dev_open()
int num_blocks;

// Array of open files
static int open_files[MAX_OPEN_FILES];
static int open_files_current_position[MAX_OPEN_FILES];
static int last_open_file_index = 0;

/* open an existing file for reading or writing */
int my_open (const char * path)
{
  printf ("my_open (%s) not implemented\n", path);
  return -1;
}

/* open a new file for writing only */
int my_creat (const char * path)
{
  int fd;
  
  // Get next free block
  int file_block_num = requestNextFreeBlock();
  
  // Split path into directory path and filename
  char *separator = strrchr(path, PATH_SEPARATOR);
  char filename_buffer[MAX_FILE_NAME_LENGTH];
  char path_buffer[strlen(path)];
  
  strcpy(filename_buffer, separator + 1);
  strncpy(path_buffer, path, separator - path);
  path_buffer[separator - path] = '\0';

  // Get directory file block number
  int directory_block_num = get_path_block_num(path_buffer, ROOT_BLOCK);
  
  // If directory does not exist, then set return value to -1 (error)
  
  // Append filename to directory file 
  // Must search directory to ensure doesn't file doesn't already exist
  
  // Add to open file array
  
  // Write file header information
  
  return fd;
} // end my_creat

/* sequentially read from a file */
int my_read (int fd, void * buf, int count)
{
  printf ("my_read (%d, %x, %d) not implemented\n", fd, *((unsigned int *)buf), count);
  return -1;
}

/* sequentially write to a file */
int my_write (int fd, const void * buf, int count)
{
  printf ("my_write (%d, %x, %d) not implemented\n", fd, *((unsigned int *)buf), count);
  return -1;
}

int my_close (int fd)
{
  printf ("my_close (%d) not implemented\n", fd);
  return -1;
}

int my_remove (const char * path)
{
  printf ("my_remove (%s) not implemented\n", path);
  return -1;
}

int my_rename (const char * old, const char * new)
{
  printf ("my_remove (%s, %s) not implemented\n", old, new);
  return -1;
}

/* only works if all but the last component of the path already exists */
int my_mkdir (const char * path)
{
  printf ("my_mkdir (%s) not implemented\n", path);
  return -1;
}

int my_rmdir (const char * path)
{
  int blockNums[2];
  char * filename;

  filename = parseRemoveNums(path, blockNums, 'r');
  if (blockNums[1] == -1) {
    return -1;
  }

  if (determineFileType(blockNums[1]) != 'd') {
	return -1;
  }

  deleteDirectoryRecursively(blockNums[1]);

  // Remove entry from parent directory.
  removeEntry(filename, blockNums[0]);

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
    int i;
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

/* Return the block number to the next free block in free block list. */
int requestNextFreeBlock ()
{
  // Buffer to read blocks from file system on disk.
  unsigned char buffer[BLOCKSIZE];

  // Get the number of whole bitmap blocks
  int num_whole_blocks = num_blocks / BITS_IN_BLOCK;

  // remainder = # of bits in the last bitmap block
  int remainder = num_blocks - (num_whole_blocks * BITS_IN_BLOCK);

  // Loop through each full bitmap block
  int i;
  for (i = FREE_LIST_BITMAP_START; i < num_whole_blocks + FREE_LIST_BITMAP_START; i++) {
    // Read in bitmap block
    if (read_block (i, buffer) < 0) {
      printf("Error reading block #%d\n", i);
      exit(1);
    }
    // Loop through each byte in the block
    int j;
    for (j = 0; j < BLOCKSIZE; j++) {
      // If the byte is not 11111111 then it has a 0 bit somewhere in the byte
      if (buffer[j] != 0xFF) {
        int bit_position = 0;
        int k;
        // Examine each bit in the byte for the 0
        for (k = BITS_IN_BYTE - 1; k >= 0; k--) {
          int bit = (buffer[j]>>k)&1;
          if (bit == 0) {
            // Return the final block number
            int block_number = ((i - 2) * BITS_IN_BLOCK) + (j * BITS_IN_BYTE) +
                                                                          bit_position;
            return block_number;
          }
          bit_position++;
        }
      }
    }
  }

  // If execution gets to this point, then we could not find a free block in any
  // of the whole blocks. Check the last partial bitmap block.
  if (remainder > 0) {
    // Read in bitmap block
    if (read_block (i, buffer) < 0) {
      printf("Error reading block #%d\n", i);
      exit(1);
    }
    int bytes_to_check = remainder / 8;
    // Loop through each byte in the block
    int j;
    for (j = 0; j < bytes_to_check; j++) {
      // If the byte is not 11111111 then it has a 0 bit somewhere in the byte
      if (buffer[j] != 0xFF) {
        int bit_position = 0;
        int k;
        // Examine each bit in the byte for the 0
        for (k = BITS_IN_BYTE - 1; k >= 0; k--) {
          int bit = (buffer[j]>>k)&1;
          if (bit == 0) {
            // Return the final block number
            int block_number = ((i - 2) * BITS_IN_BLOCK) + (j * BITS_IN_BYTE) +
                                                                          bit_position;
            return block_number;
          }
          bit_position++;
        }
      }
    }
  }

  // Return error code if a free block does not exist
  return -1;
}

/* Given a list of block numbers, free them in the bitmap (flip all to 0).
 * To free the 1st block, pass in 0 in the array. Likewise, pass in 1023 to free
 * the 1024th block.
 */
int freeBlocks (int n, int *listOfBlocks)
{
  int i;
  // Set each block to 0
  for (i = 0; i < n; i++) {
    setBlockInBitmapToStatus(0, listOfBlocks[i]);
  }
  return 1;
}

/* Set a bit in bitmap and return success or failure. */
int setBlockInBitmapToStatus (int status, int blockNumber)
{
  // Get the bit to set
  int blockToFree = blockNumber;

  // Find the bitmap block that contains the bit to set
  int bitmapBlock = blockToFree / BITS_IN_BLOCK;

  // Buffer to read block from file system on disk
  unsigned char buffer[BLOCKSIZE];

  // Read in the bitmap block that contains the bit we want to find
  if (read_block (FREE_LIST_BITMAP_START + bitmapBlock, buffer) < 0) {
    printf("Error reading block #%d\n", FREE_LIST_BITMAP_START + bitmapBlock);
    exit(1);
  }

  // Find the index of the bit to set within the block
  int bitIndexWithinBlock = blockToFree % BITS_IN_BLOCK;

  // Find which byte the bit is in
  int byteIndex = bitIndexWithinBlock / BITS_IN_BYTE;

  // Find the index of the bit within the byte. For example, the bit at index
  // 6 for 1011 1111 is 0.
  int bitIndexWithinByte = blockToFree % BITS_IN_BYTE;

  // Actual index is from right to left. For example, flipping the bit at index 7
  // for 1111 1111 results in 0111 1111.
  int actualIndex = BITS_IN_BYTE - bitIndexWithinByte - 1;

  // Set the bit
  if (status == 0) {
    // Set to 0
    buffer[byteIndex] = buffer[byteIndex]&=~(1<<actualIndex);
  }
  else if (status == 1) {
    // Set to 1
    buffer[byteIndex] = buffer[byteIndex]|=(1<<actualIndex);
  }

  // Write buffer back to disk
  write_block(FREE_LIST_BITMAP_START + bitmapBlock, buffer);

  return 1;
}
