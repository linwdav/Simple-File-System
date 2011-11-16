#include "directory_operations.h"

/* Used when creating a directory.  Returns the block number of the directory block in which
 * to place the new entry and the block number of the first free block available for the new
 * directory.
 */
int parseAndCreateDirectory(const char * path) {
  // Start at root.
  int parentDirectory = 0;

  char * token = strchr(path, '/') + 1;
  char * nextToken = strchr(token, '/');
  char directory[FILENAME_SIZE];

  // Ex. /foo, where token would be foo and nextToken would be NULL
  if (nextToken == NULL) {
	strcpy(directory, token);
  }
  // Ex. /foo/bar, where token would be foo/bar and nextToken would be /bar
  else {
	strncpy(directory, token, nextToken - token);
  }

  while (token != NULL) {
	// Then directory is the directory we want to add
	if (nextToken == NULL) {
	  int newBlockNum;
	  if ((newBlockNum = requestNextFreeBlock()) < 0) {
		return -1;
	  }

	  char newBlock[BLOCKSIZE];

	  // Read in the newly acquired block.
	  if (read_block(newBlockNum, newBlock) < 0) {
		printf("Error reading block #%d\n", newBlockNum);
		return -1;
	  }

	  // Format the header for the new block and write it back.
	  newBlock[0] = 'd';
	  char * newBlock_ptr = newBlock + 1;

	  // No next block.
	  int nextBlock = 0;
	  memcpy(newBlock_ptr, &nextBlock, sizeof(nextBlock));

	  // No data except header.
	  short bytesAllocated = HEADER_SIZE;
	  memcpy(newBlock_ptr + sizeof(nextBlock), &bytesAllocated, sizeof(bytesAllocated));

	  if (write_block(newBlockNum, newBlock) < 0) {
		printf("Error reading block #%d\n", newBlockNum);
		return -1;
	  }
	  // Add entry with name: directory, starting block number: newBlockNum, to the directory: parentDirectory
	  if (addEntry(directory, newBlockNum, parentDirectory) < 0) {
		return -1;
	  }
	  return 0;
	}

	// Nested path so update the parentDirectory number to be the next one in the path.
	// Ex. directory = foo (from /foo/bar), so starting in root directory (0) look for foo and return it's starting block
	parentDirectory = updateParentDirectoryNum(directory, parentDirectory);

	// Update the directory
	// Ex. first iteration: token = foo/bar, nextToken = /bar, directory = foo
	// After update: token = bar, nextToken = NULL, directory = bar
	// So now we know bar is the directory to be added since it was the last part of the path (i.e. nextToken = NULL)
	memset(nextToken, '\0', strlen(nextToken));
	memset(directory, '\0', FILENAME_SIZE);
	token = strchr(token, '/') + 1;
	nextToken = strchr(token, '/');
	if (nextToken == NULL) {
	  strcpy(directory, token);
	}
	else {
	  strncpy(directory, token, nextToken - token);
	}
  }
}

/* Parse the path given to the function.  Returns the block number of the parent directory and
 * the last file in the path.  Ex. /foo/bar/hello.txt would return the block numbers for the
 * directory bar and first block in hello.txt.
 */
char * parseRemoveNums(const char * path, int blockNums[2], char flag) {
  blockNums[0] = 0;

  char * token = strchr(path, '/') + 1;
  char * nextToken = strchr(token, '/');
  char * directory = malloc(FILENAME_SIZE * sizeof(char));
  if (nextToken == NULL) {
	  strcpy(directory, token);
  }
  else {
    strncpy(directory, token, nextToken - token);
  }
  char buffer[BLOCKSIZE];

  while (token != NULL) {
    if (findNextBlockNum(directory, blockNums) < 0) {
	  return NULL;
	}

	if (nextToken == NULL) {
	  token = NULL;
	}
	else {
	  memset(nextToken, '\0', strlen(nextToken));
	  memset(directory, '\0', FILENAME_SIZE);
	  token = strchr(token, '/') + 1;
	  nextToken = strchr(token, '/');
	  if (nextToken == NULL) {
		strcpy(directory, token);
	  }
	  else {
		strncpy(directory, token, nextToken - token);
	  }
	}

	if (read_block(blockNums[1], buffer) < 0) {
	  printf("Error reading block #%d\n", blockNums[1]);
      exit(1);
	}

	if (buffer[0] != 'd' && flag == 'r') {
      blockNums[1] = -1;
      return NULL;
	}
  }

  return directory;
}

/* Find the block num of the file/directory within the directory.  Has to iterate
 * through the directory comparing the filename given to each file/directory
 * within the directory until it finds a match.
 */
int findNextBlockNum(char *filename, int blockNums[2]) {
	char buffer[BLOCKSIZE];
	if (read_block(blockNums[0], buffer) < 0) {
	  printf("Error reading block #%d\n", blockNums[0]);
      exit(1);
	}

	char *buffer_ptr = buffer;
	int blockNum;

	int nextBlock;
	memcpy(&nextBlock, ++buffer_ptr, sizeof(nextBlock));

	short bytesAllocated;
	memcpy(&bytesAllocated, buffer_ptr + sizeof(nextBlock), sizeof(bytesAllocated));
	int loops = bytesAllocated / FILE_NUM_PAIRINGS_SIZE;
	buffer_ptr = buffer_ptr + 7;

	char filenamePairing[FILE_NUM_PAIRINGS_SIZE];
	char name[FILENAME_SIZE];
	char num[sizeof(blockNum)];
	char * nullIndex;
	while (loops != 0) {
	  strncpy(filenamePairing, buffer_ptr, FILE_NUM_PAIRINGS_SIZE);
	  nullIndex = strchr(filenamePairing, '\0');
	  strncpy(name, filenamePairing, nullIndex - filenamePairing); //foo.txt\0
	  strncpy(num, filenamePairing + FILENAME_SIZE, sizeof(num));
  	  if (strcmp(filename, name) == 0) {
		memcpy(&blockNums[1], num, sizeof(blockNums[1]));
		return 0;
	  }

	  loops--;
	  buffer_ptr = buffer_ptr + FILE_NUM_PAIRINGS_SIZE;

	  memset(name, '\0', FILENAME_SIZE);
	  memset(num, '\0', sizeof(blockNum));
	}
	blockNums[1] = -1;

	if (nextBlock != 0) {
	  blockNums[0] = nextBlock;
	  findNextBlockNum(filename, blockNums);
	}

	return blockNums[1];
}

/* Deletes a directory recursively.  First goes to the last block of the directory and then
 * starts to delete files.  If it encounters a directory it goes to the last block of that
 * directory and deletes that directory recursively, then keeps deleting the previous directory.
 */
void deleteDirectoryRecursively(int blockNum) {
  char buffer[BLOCKSIZE];

  if (read_block(blockNum, buffer) < 0) {
    printf("Error reading block #%d\n", blockNum);
    exit(1);
  }

  char *buffer_ptr = buffer;
  int nextBlock;
  memcpy(&nextBlock, ++buffer_ptr, sizeof(nextBlock));

  short bytesAllocated;
  memcpy(&bytesAllocated, buffer_ptr + sizeof(nextBlock), sizeof(bytesAllocated));
  int loops = bytesAllocated / FILE_NUM_PAIRINGS_SIZE;

  int i;
  char fileType;
  int fileStartBlock;

  // Last block for directory
  if (nextBlock == 0) {
    // Delete every entry in directory block
	buffer_ptr = buffer_ptr + 7;
	char filenamePairing[FILE_NUM_PAIRINGS_SIZE];

    while (loops != 0) {
	  strncpy(filenamePairing, buffer_ptr, FILE_NUM_PAIRINGS_SIZE);

	  memcpy(&fileStartBlock, filenamePairing + FILENAME_SIZE, sizeof(fileStartBlock));
	  fileType = determineFileType(fileStartBlock);
	  if (fileType == 'f') {
        deleteFileRecursively(fileStartBlock);
      }
      else {
	    deleteDirectoryRecursively(fileStartBlock);
	  }

	  loops--;
	  buffer_ptr = buffer_ptr + FILE_NUM_PAIRINGS_SIZE;
    }

    // Remove directory block from freeblock list
    deleteBlock(blockNum);
  }
  else {
    deleteDirectoryRecursively(nextBlock);
    // Delete every entry in directory block
	buffer_ptr = buffer_ptr + 7;
	char filenamePairing[FILE_NUM_PAIRINGS_SIZE];

    while (loops != 0) {
	  strncpy(filenamePairing, buffer_ptr, FILE_NUM_PAIRINGS_SIZE);

	  memcpy(&fileStartBlock, filenamePairing + FILENAME_SIZE, sizeof(fileStartBlock));
	  fileType = determineFileType(fileStartBlock);
	  if (fileType == 'f') {
        deleteFileRecursively(fileStartBlock);
      }
      else {
	    deleteDirectoryRecursively(fileStartBlock);
	  }

	  loops--;
	  buffer_ptr = buffer_ptr + FILE_NUM_PAIRINGS_SIZE;
    }
    // Remove directory block from freeblock list
    deleteBlock(blockNum);
  }
}

/* Delete file starting at last block and working backwards.
 * 
 */
void deleteFileRecursively(int blockNum) {
  char buffer[BLOCKSIZE];

  if (read_block(blockNum, buffer) < 0) {
    printf("Error reading block #%d\n", blockNum);
    exit(1);
  }

  char * buffer_ptr = buffer;
  
  int nextBlock;
  memcpy(&nextBlock, buffer_ptr+1, sizeof(nextBlock));

  if (nextBlock == 0) {
    // flip the bit in the freeblock bitmap
    deleteBlock(blockNum);
  }
  else {
    deleteFileRecursively(nextBlock);
    // flip the bit in the freeblock bitmap
    deleteBlock(blockNum);
  }
}

/* Deleting a block means: setting the bit, corresponding to the block, in the freemap to be 0
 * and setting every byte in the block to be '\0'.
 */
void deleteBlock(int blockNum) {
  char buffer[BLOCKSIZE];
  int freeblockBlock = 2;

  freeblockBlock += (blockNum / BLOCKSIZE);

  if (read_block(freeblockBlock, buffer) < 0) {
    printf("Error reading block #%d\n", freeblockBlock);
    exit(1);
  }

  int byteToChange = (blockNum % BLOCKSIZE) / 8; //blockNum = 45, byteToChange = 5 (index into buffer) so 6th element
  int byteRemainder = 7 - (blockNum % BLOCKSIZE) % 8; // byteRemainder = 5 

  char mask = 0x01; // 0000 0001
  mask = mask << byteRemainder; // 0010 0000

  buffer[byteToChange] ^= mask; // buffer[byteToChange] =  1010 1010
													// XOR 0010 0000
													//     1000 1010

  if (write_block(freeblockBlock, buffer) < 0) {
    printf("Error writing block #%d\n", freeblockBlock);
    exit(1);
  }

  // Reset block to all 0's
  memset(buffer, '\0', BLOCKSIZE);
  if (write_block(blockNum, buffer) < 0) {
    printf("Error writing block #%d\n", freeblockBlock);
    exit(1);
  }
}

/* Helper function to determine file type of block.
 * 
 */
char determineFileType(int blockNum) {
  char buffer[BLOCKSIZE];

  if (read_block(blockNum, buffer) < 0) {
    printf("Error reading block #%d\n", blockNum);
    exit(1);
  }

  return buffer[0];
}

int removeEntry(char * filename, int blockNum) {
  char buffer[BLOCKSIZE];

  if (read_block(blockNum, buffer) < 0) {
	printf("Error reading block #%d\n", blockNum);
	exit(1);
  }

  char * buffer_ptr = buffer + 1;
  short bytesAllocated;
  memcpy(&bytesAllocated, buffer_ptr + sizeof(int), sizeof(bytesAllocated));

  int loops = bytesAllocated / FILE_NUM_PAIRINGS_SIZE;
  buffer_ptr = buffer_ptr + 7;
  int filePos = 8;
  char name[FILENAME_SIZE];

  while (loops != 0) {
	memset(name, '\0', FILENAME_SIZE);
    strncpy(name, buffer_ptr, FILENAME_SIZE);

	if (strcmp(filename, name) == 0) {
		char temp[(BLOCKSIZE - filePos) - FILE_NUM_PAIRINGS_SIZE];
		strcpy(temp, buffer_ptr + FILE_NUM_PAIRINGS_SIZE);
		memset(buffer + filePos, '\0', BLOCKSIZE - filePos);
		strcpy(buffer_ptr, temp);
		bytesAllocated = bytesAllocated - FILE_NUM_PAIRINGS_SIZE;
		buffer_ptr = buffer + 1 + sizeof(int);
		memcpy(buffer_ptr, &bytesAllocated, sizeof(bytesAllocated));
		return 0;
	}

	loops--;
	buffer_ptr = buffer_ptr + FILE_NUM_PAIRINGS_SIZE;
	filePos += FILE_NUM_PAIRINGS_SIZE;
  }

  return -1;
}

int addEntry(char directory[FILENAME_SIZE], int newBlock, int parentBlock) {
	char buffer[BLOCKSIZE];

	if (read_block(parentBlock, buffer) < 0) {
	  printf("Error reading block #%d\n", parentBlock);
	  return -1;
	}
	char * buffer_ptr = buffer + 1;

	int nextBlock;
	memcpy(&nextBlock, buffer_ptr, sizeof(nextBlock));

	short bytesAllocated;
	memcpy(&bytesAllocated, buffer_ptr + sizeof(nextBlock), sizeof(bytesAllocated));

	// There is not enough free space in this block
	if ((BLOCKSIZE - bytesAllocated) < FILE_NUM_PAIRINGS_SIZE) {
		//There is no next block so have to create one.
		if (nextBlock == 0) {
		  int newNextBlock;
		  if ((newNextBlock = requestNextFreeBlock()) < 0) {
			return -1;
		  }

		  char newDirectoryBlock[BLOCKSIZE];

	      if (read_block(newNextBlock, newDirectoryBlock) < 0) {
	        printf("Error reading block #%d\n", newNextBlock);
	        return -1;
	      }

		  newDirectoryBlock[0] = 'd';
		  buffer_ptr = newDirectoryBlock + 1;

		  // nextBlock is 0
		  memcpy(buffer_ptr, &nextBlock, sizeof(nextBlock));

		  // new allocation should be 8 + 204 for new entry being added
		  short newAllocation = HEADER_SIZE + FILE_NUM_PAIRINGS_SIZE;
		  memcpy(buffer_ptr + sizeof(nextBlock), &newAllocation, sizeof(newAllocation));

		  buffer_ptr = buffer_ptr + 7;
		  strncpy(buffer_ptr, directory, FILENAME_SIZE);
		  memcpy(buffer_ptr + FILENAME_SIZE, &newBlock, sizeof(newBlock));

		  if (write_block(newNextBlock, newDirectoryBlock) < 0) {
			printf("Error writing block #%d\n", newNextBlock);
			return -1;
		  }

		  // Change the nextblock field of the original directory block to point to new directory block.
		  buffer_ptr = buffer + 1;
		  memcpy(buffer_ptr, &newNextBlock, sizeof(newNextBlock));

		  if (write_block(parentBlock, buffer) < 0) {
			printf("Error writing block #%d\n", newNextBlock);
			return -1;
		  }

		  return 0;
		}
		// Directory block is full but there is a next block.
		else {
			// recursive call that tries to add the new directory to the new block.
			return addEntry(directory, nextBlock, newBlock);
		}
	}
	// There is enough space in the block.
	else {
	  // Go to end of allocated block
	  buffer_ptr = buffer_ptr + bytesAllocated;

	  // Copy in the filename and assocated block number
	  strncpy(buffer_ptr, directory, FILENAME_SIZE);
	  buffer_ptr = buffer_ptr + FILENAME_SIZE;
	  memcpy(buffer_ptr, &newBlock, sizeof(newBlock));

	  // Recompute bytes allocated and copy it in
	  buffer_ptr = buffer_ptr + 1 + sizeof(nextBlock);
	  bytesAllocated = bytesAllocated + FILE_NUM_PAIRINGS_SIZE;
	  memcpy(buffer_ptr, &bytesAllocated, sizeof(bytesAllocated));

	  if (write_block(parentBlock, buffer) < 0) {
		printf("Error writing block #%d\n", parentBlock);
		return -1;
	  }

	  return 0;
	}
}

int updateParentDirectoryNum(char directory[FILENAME_SIZE], int parentDirectory) {
  char buffer[BLOCKSIZE];

  if (read_block(parentDirectory, buffer) < 0) {
	printf("Error reading block #%d\n", parentDirectory);
	return -1;
  }

  char * buffer_ptr = buffer + 1;

  int nextBlock;
  memcpy(&nextBlock, buffer_ptr, sizeof(nextBlock));

  short bytesAllocated;
  memcpy(&bytesAllocated, buffer_ptr + sizeof(nextBlock), sizeof(bytesAllocated));

  int loops = bytesAllocated / FILE_NUM_PAIRINGS_SIZE;
  buffer_ptr = buffer_ptr + 7;

  // No next block, so if not in this block return an error.
  if (nextBlock == 0) {
	  while (loops != 0) {
		char name[FILENAME_SIZE];

		strncpy(name, buffer_ptr, FILENAME_SIZE);

		if (strcmp(name, directory) == 0) {
		  int associatedNum;
		  memcpy(&associatedNum, buffer_ptr + FILENAME_SIZE, sizeof(associatedNum));
		  return associatedNum;
		}

		loops--;
		buffer_ptr = buffer_ptr + FILE_NUM_PAIRINGS_SIZE;
	  }
	  return -1;
  }
  // There is a next block.
  else {
	// First recursively check if the directory is in later blocks.
	int check = updateParentDirectoryNum(directory, nextBlock);

	// If it was then return the number of the block that has the directory.
	if (check > 0) {
	  return check;
	}

	// Wasn't in other blocks so check this one.
	while (loops != 0) {
	  char name[FILENAME_SIZE];

	  strncpy(name, buffer_ptr, FILENAME_SIZE);

	  if (strcmp(name, directory) == 0) {
		int associatedNum;
		memcpy(&associatedNum, buffer_ptr + FILENAME_SIZE, sizeof(associatedNum));
		return associatedNum;
	  }

	  loops--;
	  buffer_ptr = buffer_ptr + FILE_NUM_PAIRINGS_SIZE;
	}
	// Wasn't in any blocks after this one or in this one itself.
	return -1;
  }
}