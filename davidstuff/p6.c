/* project 6 file for UHM ICS 612, Spring 2011*/

/* This program is public domain.  As written, it does very little.
 * You are welcome to use it and modify it as you see fit, as long
 * as I am not responsible for anything you do with it.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "block.h"

#define NUM_SYSTEM_BLOCKS 2
#define ROOT_BLOCK 0
#define FREE_LIST_BITMAP_PARAMS 1
#define FREE_LIST_BITMAP_START 2
#define HEADER_SIZE 8
#define BITS_IN_BLOCK (8 * BLOCKSIZE)

int num_blocks;

/* open an exisiting file for reading or writing */
int my_open (const char * path)
{
  printf ("my_open (%s) not implemented\n", path);
  return -1;
}

/* open a new file for writing only */
int my_creat (const char * path)
{
  printf ("my_creat (%s) not implemented\n", path);
  return -1;
}

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
  printf ("my_rmdir (%s) not implemented\n", path);
  return -1;
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
    
    printf("never_free %d\n", never_free);
    printf("remainder %d\n", remainder);
    printf("never_free_blocks %d\n", never_free_blocks);

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
      
      printf("remainder_of_remainder %d\n", remainder_of_remainder);
      printf("dividend_of_remainder %d\n", dividend_of_remainder);

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

