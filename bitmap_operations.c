#include "bitmap_operations.h"

/* Return the block number to the next free block in free block list. */
int requestNextFreeBlock ()
{
  // Buffer to read blocks from file system on disk.
  unsigned char buffer[BLOCKSIZE];

  // Number of blocks on disk - value returned via dev_open()
  int num_blocks;

  // Attempt to open the disk
  if ((num_blocks = dev_open()) < 0) {
    printf("Disk does not exist or disk is corrupt.\n");
    exit(1);
  }

  // Get the number of whole bitmap blocks
  int num_whole_blocks = num_blocks / BITS_IN_BLOCK;

  // remainder = # of bits in the last bitmap block
  int remainder = num_blocks - (num_whole_blocks * BITS_IN_BLOCK);

  // Loop through each whole bitmap block
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
      // If the byte is not 1111 1111 then it has a 0 bit somewhere within the byte
      if (buffer[j] != 0xFF) {
        int bit_position = 0;
        int k;
        // Examine each bit in the byte for the 0
        for (k = BITS_IN_BYTE - 1; k >= 0; k--) {
          // Store the bit
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
  // of the whole blocks. Check the last partially-filled bitmap block.
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
      // If the byte is not 1111 1111 then it has a 0 bit somewhere in the byte
      if (buffer[j] != 0xFF) {
        int bit_position = 0;
        int k;
        // Examine each bit in the byte for the 0
        for (k = BITS_IN_BYTE - 1; k >= 0; k--) {
          // Store the bit
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

  // Find the index of the byte within the block
  int byteIndex = bitIndexWithinBlock / BITS_IN_BYTE;

  // Find the index of the bit within the byte. For example, the bit at index
  // 6 for 1111 1101 is 0.
  int bitIndexWithinByte = blockToFree % BITS_IN_BYTE;

  // Actual index is from right to left. For example, flipping the bit at index 7
  // for 1111 1111 results in 0111 1111.
  int actualIndex = BITS_IN_BYTE - bitIndexWithinByte - 1;

  // Set the bit
  if (status == 0) {
    // Set to 0
    int bit = (buffer[byteIndex]>>actualIndex)&1;
    if (bit == 0) {
      printf("Warning. Trying to free block %d when it is already free.\n", blockNumber);
    }
    buffer[byteIndex] = buffer[byteIndex]&=~(1<<actualIndex);
  }
  else if (status == 1) {
    // Set to 1
    int bit = (buffer[byteIndex]>>actualIndex)&1;
    if (bit == 1) {
      printf("File system error. Trying to write to block %d when it is in use by another file.\n", blockNumber);
    }
    buffer[byteIndex] = buffer[byteIndex]|=(1<<actualIndex);
  }

  // Write buffer back to disk
  write_block(FREE_LIST_BITMAP_START + bitmapBlock, buffer);

  return 1;
}
