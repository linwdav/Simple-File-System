#include "directory_operations.h"

#define DEBUG 0
/* Used when creating a directory.  Returns the block number of the directory block in which
 * to place the new entry and the block number of the first free block available for the new
 * directory.
 * path - The path to parse.
 * rename_start_block - Only needed if renaming not creating a new directory, the start block of the
 * already existing directory, needed for pairing with name in directory.
 * flag - whether renaming or creating a new directory.
 */
int parseAndCreateDirectory(const char * path, int rename_start_block, char flag) {
  // Start at root.
  int parentDirectory = 0;

  char * token = strchr(path, '/') + 1;
  char * nextToken = strchr(token, '/');
  char directory[strlen(token)];

  // Ex. /foo, where token would be foo and nextToken would be NULL
  if (nextToken == NULL) {
	strcpy(directory, token);
	directory[strlen(token)] = '\0';
  }
  // Ex. /foo/bar, where token would be foo/bar and nextToken would be /bar
  else {
	strncpy(directory, token, nextToken - token);
	directory[nextToken - token] = '\0';
  }

  while (token != NULL) {
	// Then directory is the directory we want to add
	if (nextToken == NULL) {
	  if (strlen(directory) > FILENAME_SIZE) {
	    printf("Directory name too long\n");
		return -1;
	  }

	  int exists;
	  // Check if directory exists before trying to add it, if it does return error.
	  if ((exists = check_directory_exists(directory, parentDirectory)) > 0) {
		printf("Directory %s already exists in block %d\n", directory, exists);
		return -1;
	  }

	  // Not creating a new directory simply renaming it.
	  if (flag == 'r') {
		// Add entry with name: directory, starting block number: rename_start_block, to the directory: parentDirectory
	    if ((parentDirectory = addEntry(directory, rename_start_block, parentDirectory)) < 0) {
		  return -1;
	    }

		if (DEBUG) {
		  printf("Renamed directory to %s\nAdded the entry at %d\twith name: %s, at block %d\n", path, parentDirectory, directory, rename_start_block);
		}

	    return 0;
	  }

	  int newBlockNum;
	  if ((newBlockNum = requestNextFreeBlock()) < 0) {
		printf("No more free blocks\n");
		return -1;
	  }
	  setBlockInBitmapToStatus(1, newBlockNum);

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
	  if ((parentDirectory = addEntry(directory, newBlockNum, parentDirectory)) < 0) {
		return -1;
	  }
	  if (DEBUG) {
	    printf("Created directory at %s\nAdded the entry at %d\twith name: %s, at block %d\n", path, parentDirectory, directory, newBlockNum);
	  }

	  return 0;
	}

	// Nested path so update the parentDirectory number to be the next one in the path.
	// Ex. directory = foo (from /foo/bar), so starting in root directory (0) look for foo and return it's starting block
	if ((parentDirectory = updateParentDirectoryNum(directory, parentDirectory)) < 0) {
	  printf("Couldn't update parent directory number\nwith directory %s at block number %d\n", directory, parentDirectory);
	  return -1;
	}

	// Update the directory
	// Ex. first iteration: token = foo/bar, nextToken = /bar, directory = foo
	// After update: token = bar, nextToken = NULL, directory = bar
	// So now we know bar is the directory to be added since it was the last part of the path (i.e. nextToken = NULL)
	// memset(nextToken, '\0', strlen(nextToken));
	// memset(directory, '\0', FILENAME_SIZE);
	token = strchr(token, '/') + 1;
	nextToken = strchr(token, '/');
	if (nextToken == NULL) {
	  strcpy(directory, token);
	  directory[strlen(token)] = '\0';
	}
	else {
	  strncpy(directory, token, nextToken - token);
	  directory[nextToken - token] = '\0';
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
  char * directory = malloc(strlen(token) * sizeof(char));
  if (nextToken == NULL) {
	  strcpy(directory, token);
	  directory[strlen(token)] = '\0';
  }
  else {
    strncpy(directory, token, nextToken - token);
	directory[nextToken - token] = '\0';
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
	  // memset(nextToken, '\0', strlen(nextToken));
	  // memset(directory, '\0', FILENAME_SIZE);
	  token = strchr(token, '/') + 1;
	  nextToken = strchr(token, '/');
	  if (nextToken == NULL) {
		strcpy(directory, token);
		directory[strlen(token)] = '\0';
	  }
	  else {
		strncpy(directory, token, nextToken - token);
		directory[nextToken - token] = '\0';
	  }

	  blockNums[0] = blockNums[1];
	}

	if (read_block(blockNums[1], buffer) < 0) {
	  printf("Error reading block #%d\n", blockNums[1]);
      return NULL;
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
      return -1;
	}

	char *buffer_ptr = buffer + 1;
	int blockNum;

	int nextBlock;
	memcpy(&nextBlock, buffer_ptr, sizeof(nextBlock));

	short bytesAllocated;
	memcpy(&bytesAllocated, buffer_ptr + sizeof(nextBlock), sizeof(bytesAllocated));
	int loops = bytesAllocated / FILE_NUM_PAIRINGS_SIZE;
	buffer_ptr = buffer + HEADER_SIZE;

	int num;
	while (loops != 0) {
	  char name[FILENAME_SIZE];
	  strncpy(name, buffer_ptr, FILENAME_SIZE);
	  memcpy(&num, buffer_ptr + FILENAME_SIZE, sizeof(num));
  	  if (strcmp(filename, name) == 0) {
		blockNums[1] = num;
		return 0;
	  }

	  loops--;
	  buffer_ptr = buffer_ptr + FILE_NUM_PAIRINGS_SIZE;
	  memset(name, '\0', FILENAME_SIZE);
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
int deleteDirectoryRecursively(int blockNum) {
  char buffer[BLOCKSIZE];

  if (read_block(blockNum, buffer) < 0) {
    printf("Error reading block #%d\n", blockNum);
    return -1;
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
	buffer_ptr = buffer + HEADER_SIZE;

    while (loops != 0) {
	  char name[FILENAME_SIZE];
	  strncpy(name, buffer_ptr, FILENAME_SIZE);

	  memcpy(&fileStartBlock, buffer_ptr + FILENAME_SIZE, sizeof(fileStartBlock));
	  fileType = determineFileType(fileStartBlock);
	  if (fileType == 'f') {
		close_file_if_open(fileStartBlock);
        if (deleteFileRecursively(fileStartBlock) < 0) {
		  return -1;
		}
      }
      else {
	    if (deleteDirectoryRecursively(fileStartBlock) < 0) {
		  return -1;
		}
	  }

	  loops--;
	  buffer_ptr = buffer_ptr + FILE_NUM_PAIRINGS_SIZE;
    }

    // Remove directory block from freeblock list
    if (deleteBlock(blockNum) < 0) {
	  return -1;
	}

  }
  else {
    deleteDirectoryRecursively(nextBlock);
    // Delete every entry in directory block
	buffer_ptr = buffer + 8;


    while (loops != 0) {
	  char name[FILENAME_SIZE];
	  strncpy(name, buffer_ptr, FILENAME_SIZE);

	  memcpy(&fileStartBlock, buffer_ptr + FILENAME_SIZE, sizeof(fileStartBlock));
	  fileType = determineFileType(fileStartBlock);
	  if (fileType == 'f') {
		close_file_if_open(fileStartBlock);
        if (deleteFileRecursively(fileStartBlock) < 0) {
		  return -1;
		}
      }
      else {
	    if (deleteDirectoryRecursively(fileStartBlock) < 0) {
		  return -1;
		}
	  }

	  loops--;
	  buffer_ptr = buffer_ptr + FILE_NUM_PAIRINGS_SIZE;
    }
    // Remove directory block from freeblock list
    if (deleteBlock(blockNum) < 0) {
	  return -1;
	}

	return 0;
  }
}

/* Delete file starting at last block and working backwards.
 * 
 */
int deleteFileRecursively(int blockNum) {
  char buffer[BLOCKSIZE];

  if (read_block(blockNum, buffer) < 0) {
    printf("Error reading block #%d\n", blockNum);
    return -1;
  }

  char * buffer_ptr = buffer;
  
  int nextBlock;
  memcpy(&nextBlock, buffer_ptr+1, sizeof(nextBlock));

  if (nextBlock == 0) {
    // flip the bit in the freeblock bitmap
    if (deleteBlock(blockNum) < 0) {
	  return -1;
	}

  }
  else {
    if (deleteFileRecursively(nextBlock) < 0) {
	  return -1;
	}
    // flip the bit in the freeblock bitmap
    if (deleteBlock(blockNum) < 0) {
	  return -1;
	}

  }

  return 0;
}

/* Deleting a block means: setting the bit, corresponding to the block, in the freemap to be 0
 * and setting every byte in the block to be '\0'.
 */
int deleteBlock(int blockNum) {
  char buffer[BLOCKSIZE];
  int freeblockBlock = 2;

  freeblockBlock += (blockNum / BLOCKSIZE);

  if (read_block(freeblockBlock, buffer) < 0) {
    printf("Error reading block #%d\n", freeblockBlock);
    return -1;
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
    return -1;
  }

  // Reset block to all 0's
  memset(buffer, '\0', BLOCKSIZE);
  if (write_block(blockNum, buffer) < 0) {
    printf("Error writing block #%d\n", freeblockBlock);
    return -1;
  }

  if (DEBUG) {
    printf("Deleted block %d\n", blockNum);
  }

  return 0;
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

/* Removes an entry from a directory.  When it removes the entry it moves the rest of
 * the data in the directory block up so that the free space in the directory block
 * is all at the end.
 * filename - The name of the directory/file to remove.
 * blockNum - The block number of the directory that has the entry.
 */
int removeEntry(char * filename, int blockNum) {
  char buffer[BLOCKSIZE];

  if (read_block(blockNum, buffer) < 0) {
	printf("Error reading block #%d\n", blockNum);
	return -1;
  }

  char * buffer_ptr = buffer + 1;
  short bytesAllocated;
  memcpy(&bytesAllocated, buffer_ptr + sizeof(int), sizeof(bytesAllocated));

  int loops = bytesAllocated / FILE_NUM_PAIRINGS_SIZE;
  buffer_ptr = buffer + HEADER_SIZE;
  int filePos = HEADER_SIZE;

  while (loops != 0) {
    char name[FILENAME_SIZE + 1];
    strncpy(name, buffer_ptr, FILENAME_SIZE);
	name[FILENAME_SIZE] = '\0';

	if (strcmp(filename, name) == 0) {
		char tempName[FILENAME_SIZE];
		strncpy(tempName, buffer_ptr + FILE_NUM_PAIRINGS_SIZE, FILENAME_SIZE);
		int tempNum;
		memcpy(&tempNum, buffer_ptr + (FILE_NUM_PAIRINGS_SIZE + FILENAME_SIZE), sizeof(tempNum));

		// Create a temporary buffer to hold everything from the block after the entry to be deleted.
		int temp_size = (BLOCKSIZE - filePos) - FILE_NUM_PAIRINGS_SIZE;
		char temp[temp_size + 1];
		int i;
		for (i = 0; i < temp_size; i++) {
		  temp[i] = buffer[filePos + FILE_NUM_PAIRINGS_SIZE + i];
		}
		temp[temp_size] = '\0';

		// Clear the directory block starting from beginning of entry, and copy back in 
		// everything from after the deleted entry.
		memset(buffer + filePos, '\0', BLOCKSIZE - filePos);
		for (i = 0; i < temp_size; i++) {
		  buffer[filePos + i] = temp[i];
		}

		// Adjust the bytes allocated.
		bytesAllocated = bytesAllocated - FILE_NUM_PAIRINGS_SIZE;
		buffer_ptr = buffer + 1 + sizeof(int);
		memcpy(buffer_ptr, &bytesAllocated, sizeof(bytesAllocated));

		if (write_block(blockNum, buffer) < 0) {
		  printf("Error writing block #%d\n", blockNum);
		  return -1;
		}

		return 0;
	}

	loops--;
	buffer_ptr = buffer_ptr + FILE_NUM_PAIRINGS_SIZE;
	filePos += FILE_NUM_PAIRINGS_SIZE;
  }

  return -1;
}

/* Adds an entry to a directory.
 * directory - the name of the file/directory to be placed in the entry
 * newBlock - the block number associated with the directory.
 * parentBlock - the directory to place the entry in.
 */
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
		  setBlockInBitmapToStatus(1, newNextBlock);

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

		  // add in the filename block number entry to the new directory block.
		  buffer_ptr = newDirectoryBlock + HEADER_SIZE;
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

		  return newNextBlock;
		}
		// Directory block is full but there is a next block.
		else {
			// recursive call that tries to add the new directory to the new block.
			return addEntry(directory, newBlock, nextBlock);
		}
	}
	// There is enough space in the block.
	else {
	  // Go to end of allocated block
	  buffer_ptr = buffer + bytesAllocated;

	  // Copy in the filename and associated block number
	  strncpy(buffer_ptr, directory, FILENAME_SIZE);
	  buffer_ptr = buffer_ptr + FILENAME_SIZE;
	  memcpy(buffer_ptr, &newBlock, sizeof(newBlock));

	  // Recompute bytes allocated and copy it in
	  buffer_ptr = buffer + 1 + sizeof(nextBlock);
	  bytesAllocated = bytesAllocated + FILE_NUM_PAIRINGS_SIZE;
	  memcpy(buffer_ptr, &bytesAllocated, sizeof(bytesAllocated));

	  if (write_block(parentBlock, buffer) < 0) {
		printf("Error writing block #%d\n", parentBlock);
		return -1;
	  }

	  return parentBlock;
	}
}

/* Used when parsing a path.  Iterates through a directory to find the block number of the next
 * step in the path.  Ex.  /foo/bar - You can search for foo in the root directory, or for bar
 * within the foo directory (as long as you know the beginning block number of the foo directory)
 * directory - The name of the file/directory you are searching for.
 * parentDirectory - The block number of the directory you are searching through.
 */
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

/* Check if the directory exists before trying to create it.  Returns -1 if already exists.
 * directory - the name of the directory to check for.
 * blockNum - the block number of the directory to check in.
 */
int check_directory_exists(char * directory, int blockNum) {
	char buffer[1024];

	if (read_block(blockNum, buffer) < 0) {
	  printf("Error reading block #%d\n", blockNum);
	  return -1;
	}

	char * buffer_ptr = buffer + 1;

	int nextBlock;
	memcpy(&nextBlock, buffer_ptr, sizeof(nextBlock));

	short bytesAllocated;
	memcpy(&bytesAllocated, buffer_ptr + sizeof(nextBlock), sizeof(bytesAllocated));

	int loops = bytesAllocated / FILE_NUM_PAIRINGS_SIZE;
	char check_filename[FILENAME_SIZE];
	buffer_ptr = buffer + 8;
	// Loop through the directory block for each filename.
	while (loops != 0) {
	  strncpy(check_filename, buffer_ptr, FILENAME_SIZE);

	  // If the filename is the same as the one trying to add, return error
	  if (strcmp(check_filename, directory) == 0) {
		return blockNum;
	  }

	  loops--;
	  buffer_ptr = buffer_ptr + FILE_NUM_PAIRINGS_SIZE;
	}

	int check = -1;
	if (nextBlock != 0) {
	  check = check_directory_exists(directory, nextBlock);
	}

	return check;
}

char * get_parent_blocks(const char * path, int parent_blocks[2]) {
  parent_blocks[0] = 0;
  parent_blocks[1] = 0;

  char * first = strchr(path, '/') + 1;
  char * second = strchr(first, '/');
  char * name = malloc(strlen(first) * sizeof(char));
  char * final_name = strrchr(path, '/') + 1;
  char * return_name = malloc((FILENAME_SIZE + 1) * sizeof(char));
  if (second == NULL) {
	strcpy(name, first);
  }
  else {
	strncpy(name, first, second - first);
	name[second - first] = '\0';
  }

  strncpy(return_name, name, strlen(name));
  return_name[strlen(name)] = '\0';

  char buffer[BLOCKSIZE];
  char * buffer_ptr;
  int check = -4;

  while (first != NULL) {
	check = contains_file(name, final_name, parent_blocks);
	// found the last token in the path.
    if (check == -1) {
	  return return_name;
    }
	// Haven't found parent block yet.
	else if (check == -2) {
	  // Couldn't find the next step in path.
	  if (parent_blocks[0] == parent_blocks[1]) {
		return NULL;
	  }
	  continue;
	}
	// Found the token, now update token.
	else if (check == 0) {
	  strncpy(return_name, name, strlen(name));
	  return_name[strlen(name)] = '\0';
	  if (second == NULL) {
		first == NULL;
	  }
	  else {
	    first = strchr(first, '/') + 1;
	    second = strchr(first, '/');
	    if (second == NULL) {
	      strcpy(name, first);
	    }
	    else {
	      strncpy(name, first, second - first);
	      name[second - first] = '\0';
	    }
	  }
	}
	else {
	  return NULL;
	}
  }
}

int contains_file(char * name, char * final_name, int parent_block[2]) {
  char buffer[BLOCKSIZE];

  if (read_block(parent_block[1], buffer) < 0) {
	printf("Error reading block #%d\n", parent_block[1]);
	return -3;
  }

  short bytesAllocated;
  memcpy(&bytesAllocated, buffer + 1 + sizeof(int), sizeof(bytesAllocated));

  int loops = bytesAllocated / FILE_NUM_PAIRINGS_SIZE;

  int i, offset;
  for (i = 0; i < loops; i++) {
	char filename[FILENAME_SIZE + 1];
	offset = HEADER_SIZE + (i * FILE_NUM_PAIRINGS_SIZE);
	strncpy(filename, buffer + offset, FILENAME_SIZE);
	filename[FILENAME_SIZE] = '\0';

	if (strcmp(filename, name) == 0) {
	  if (strcmp(name, final_name) == 0) {
		return -1;
	  }
	  int num;
	  memcpy(&num, buffer + offset + FILENAME_SIZE, sizeof(num));

	  parent_block[0] = parent_block[1];
      parent_block[1] = num;

	  return 0;
	}
  }

  int next_block;
  memcpy(&next_block, buffer + 1, sizeof(next_block));

  parent_block[0] = parent_block[1];
  parent_block[1] = next_block;
  return -2;
}

int update_directory_blocks(char * dir, int blocks[2]) {
	char buffer_one[BLOCKSIZE];

	if (blocks[0] == blocks[1]) {
	  return 0;
	}

	if (read_block(blocks[1], buffer_one) < 0) {
	  printf("Error reading block #%d\n", blocks[1]);
	  return -1;
	}

	short bytes_allocated;
	memcpy(&bytes_allocated, buffer_one + 1 + sizeof(int), sizeof(bytes_allocated));

	if (bytes_allocated < (FILE_NUM_PAIRINGS_SIZE + HEADER_SIZE)) {
	  char buffer_zero[BLOCKSIZE];

	  if (read_block(blocks[0], buffer_zero) < 0) {
	    printf("Error reading block #%d\n", blocks[0]);
	    return -1;
	  }

	  short loop_bytes;
	  memcpy(&loop_bytes, buffer_zero + 1 + sizeof(int), sizeof(loop_bytes));
	  int loops = loop_bytes / FILE_NUM_PAIRINGS_SIZE;
	  int i;
	  char * buf_zero_ptr = buffer_zero + HEADER_SIZE;
	  for (i = 0; i < loops; i++) {
		  char filename[FILENAME_SIZE + 1];

		  strncpy(filename, buf_zero_ptr, FILENAME_SIZE);
		  filename[FILENAME_SIZE] = '\0';
		  if (strcmp(filename, dir) == 0) {
			int num;
			memcpy(&num, buf_zero_ptr + FILENAME_SIZE, sizeof(num));

			if (num == blocks[1]) {
			  return 0;
			}
		  }

		  buf_zero_ptr = buf_zero_ptr + FILE_NUM_PAIRINGS_SIZE;
	  }

	  int next_block;
	  memcpy(&next_block, buffer_one + 1, sizeof(next_block));
	  memcpy(buffer_zero + 1, &next_block, sizeof(next_block));

	  if (write_block(blocks[0], buffer_zero) < 0) {
		printf("Error writing block #%d\n", blocks[0]);
		return -1;
	  }

	  deleteBlock(blocks[1]);
	}

	return 0;
}