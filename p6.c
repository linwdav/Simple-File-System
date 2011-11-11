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
  char buffer[BLOCKSIZE];
  
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
  
  // Check block 0 to see if root directory exists
  if (buffer[0] != 'd') {
  
    // If not create file system.
    // Create root directory at block 0.
    buffer[0] = 'd';
    
    int next_block = 0;
    short header_size = HEADER_SIZE;
    char * buffer_ptr = buffer;

    memcpy(++buffer_ptr, &next_block, sizeof(next_block));
    
    memcpy((buffer_ptr+sizeof(next_block)), &header_size, sizeof(header_size));
    
    write_block(ROOT_BLOCK, buffer);
    
    // Create free list bitmap
    // Assumption, all blocks are set to 0.
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
    
    memset(buffer, 0, HEADER_SIZE);
    buffer_ptr = buffer;
    memcpy(buffer_ptr, &end_bitmap_block, sizeof(end_bitmap_block));
    
    write_block(FREE_LIST_BITMAP_PARAMS, buffer);
    
    // Allocate 1's to all system blocks and bitmap_num_blocks
    int never_free = NUM_SYSTEM_BLOCKS + bitmap_num_blocks;
    int never_free_blocks;
     
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
    
    memset(buffer, 0xFF, BLOCKSIZE);
    
    int i;
    for(i = 0; i < never_free_blocks; i++) {
      write_block(i + FREE_LIST_BITMAP_START, buffer);
    }    
    
    // Set remainder to 1s if needed
    int remainder_of_remainder = remainder % 8;
    int dividend_of_remainder = remainder / 8;
    
    int j;
    memset(buffer, 0, BLOCKSIZE);
    
    for (j = 0; j < dividend_of_remainder; j++) {
      buffer[j] = 0xFF;
    }
    /*
    if (remainder_of_remainder > 0) {
      memset(&buffer[j], 0xFF, 1);
      buffer[j] << (8 - remainder_of_remainder);
    }
    */
    
    
    write_block(i + FREE_LIST_BITMAP_START, buffer);
    
    printf("File system created.\n");
  }
  else {
    printf("File system already exists.\n");
  }
  
} // End my_mkfs

