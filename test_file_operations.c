#include "test_file_operations.h"

#define DIRECTORY_BLOCK_0            50
#define DIRECTORY_BLOCK_1            60
#define DIRECTORY_BLOCK_1_NEXT_BLOCK 70
#define DIRECTORY_BLOCK_1_1          80
#define FILE_BLOCK_1                 100
#define DEBUG						 0

int size_of_disk;

/*
 * Hard codes the following directory structure:
 * root
 *   foo1 50  (Directory block 0) - NOTE: no actual directory file created
 *   foo2 60,70  (Directory block 1)
 *     bar1 80 (Directory block 1_1)
 *     hello.txt 100 (File block 1)
 */
void test_file_operations_setup() {
  size_of_disk = dev_open();
  
  if (size_of_disk != 250000) {
    printf("\ntest_file_operations_setup FAIL.  Size of disk must be 250,000 blocks.\n");
  }
  
  my_mkfs();
  
  // Zero out buffers
  char buffer[BLOCKSIZE];
  char *buffer_ptr = buffer;
  char name[MAX_FILE_NAME_LENGTH];
  
  /** ROOT BLOCK **/  
  unsigned short bytes_allocated = HEADER_SIZE + 2 * FILE_NUM_PAIRINGS_SIZE;
  
  populate_file_header(buffer, ROOT_BLOCK, bytes_allocated, 'd');
  
  // Set directory one name and block number
  memset(name, 0, MAX_FILE_NAME_LENGTH);
  strcpy(name, "foo1");
  buffer_ptr += HEADER_SIZE;
  memcpy(buffer_ptr, name, MAX_FILE_NAME_LENGTH);
  
  unsigned int block_num = DIRECTORY_BLOCK_0;
  buffer_ptr += MAX_FILE_NAME_LENGTH;
  memcpy(buffer_ptr, &block_num, BYTES_IN_INT);
  
  // Set directory two name and block number
  memset(name, 0, MAX_FILE_NAME_LENGTH);
  buffer_ptr += BYTES_IN_INT;
  strcpy(name, "foo2");
  memcpy(buffer_ptr, name, MAX_FILE_NAME_LENGTH);
  
  block_num = DIRECTORY_BLOCK_1;
  buffer_ptr += MAX_FILE_NAME_LENGTH;
  memcpy(buffer_ptr, &block_num, BYTES_IN_INT);

  write_block(ROOT_BLOCK, buffer);
  setBlockInBitmapToStatus (1, ROOT_BLOCK);
  
  /** DIRECTORY 1 **/
  bytes_allocated = HEADER_SIZE + 1 * FILE_NUM_PAIRINGS_SIZE;
    
  populate_file_header(buffer, DIRECTORY_BLOCK_1_NEXT_BLOCK, bytes_allocated, 'd');
  
  // Set directory one name and block number
  memset(name, 0, MAX_FILE_NAME_LENGTH);
  strcpy(name, "bar1");
  buffer_ptr = buffer;
  buffer_ptr += HEADER_SIZE;
  memcpy(buffer_ptr, name, MAX_FILE_NAME_LENGTH);
  
  block_num = DIRECTORY_BLOCK_1_1;
  buffer_ptr += MAX_FILE_NAME_LENGTH;
  memcpy(buffer_ptr, &block_num, BYTES_IN_INT);

  write_block(DIRECTORY_BLOCK_1, buffer);
  setBlockInBitmapToStatus (1, DIRECTORY_BLOCK_1);
  
  
  /** DIRECTORY 1_NEXT_BLOCK **/
  bytes_allocated = HEADER_SIZE + 1 * FILE_NUM_PAIRINGS_SIZE;
    
  populate_file_header(buffer, ROOT_BLOCK, bytes_allocated, 'd');
  
  // Set directory one name and block number
  memset(name, 0, MAX_FILE_NAME_LENGTH);
  strcpy(name, "hello.txt");
  buffer_ptr = buffer;
  buffer_ptr += HEADER_SIZE;
  memcpy(buffer_ptr, name, MAX_FILE_NAME_LENGTH);
  
  block_num = FILE_BLOCK_1;
  buffer_ptr += MAX_FILE_NAME_LENGTH;
  memcpy(buffer_ptr, &block_num, BYTES_IN_INT);

  write_block(DIRECTORY_BLOCK_1_NEXT_BLOCK, buffer);
  setBlockInBitmapToStatus (1, DIRECTORY_BLOCK_1_NEXT_BLOCK);
  
  /** DIRECTORY 1_1 **/
  bytes_allocated = HEADER_SIZE;
    
  populate_file_header(buffer, ROOT_BLOCK, bytes_allocated, 'd');
  
  // Set directory one name and block number
  write_block(DIRECTORY_BLOCK_1_1, buffer);
  setBlockInBitmapToStatus (1, DIRECTORY_BLOCK_1_1);
}

void test_my_read() {
  // Create file /foo2/read-test.txt
	int fd = my_creat("/foo2/read-test.txt");
	
	int block_num = open_files[fd];
	
	printf("\ntest_my_read: file created at block %i\n", block_num);
	
	// Buffer to store data to write to file
	char buffer[BLOCKSIZE];
	char *buffer_ptr = buffer;
	
	int next_block = requestNextFreeBlock();
  setBlockInBitmapToStatus(1, next_block);

	printf("test_my_read: file 2nd block created at block %i\n", next_block);
	
	// Write first block of file
	short bytes_allocated = HEADER_SIZE + 20;
	*buffer_ptr = 'f';
  memcpy(++buffer_ptr, &next_block, BYTES_IN_INT);
	buffer_ptr += BYTES_IN_INT;
	memcpy(buffer_ptr, &bytes_allocated, BYTES_IN_SHORT);
	buffer_ptr += BYTES_IN_SHORT;
	char data[] = "1234567890abcdefghij";
	buffer_ptr = buffer;
	buffer_ptr += HEADER_SIZE;
	memcpy(buffer_ptr, data, 20);
	write_block(block_num, buffer);
	
	// Write second (last) block of file
	block_num = next_block;
	next_block = ROOT_BLOCK;
	bytes_allocated = HEADER_SIZE + 10;
	
	memset(buffer, 0, BLOCKSIZE);
	buffer_ptr = buffer;
	*buffer_ptr = 'f';
  memcpy(++buffer_ptr, &next_block, BYTES_IN_INT);
	buffer_ptr += BYTES_IN_INT;
	memcpy(buffer_ptr, &bytes_allocated, BYTES_IN_SHORT);
	buffer_ptr += BYTES_IN_SHORT;
	char data2[] = "ABCDEFGHIJ";
	buffer_ptr = buffer;
	buffer_ptr += HEADER_SIZE;
	memcpy(buffer_ptr, data2, 10);
	write_block(block_num, buffer);
	
	char buf[BLOCKSIZE * 4];
	memset(buf, 0, BLOCKSIZE * 4);
	my_close(fd);
	
	// This should produce an error
	printf("my_test_read: should produce error below.\n");
	int result = my_read(fd, buf, 25);
	
	if (result != -1) {
		printf("my_test_read: FAILED (should not read closed file).\n");
	}
	
	fd = my_open("/foo2/read-test.txt");
	int num_bytes_requested = 23;
	result = my_read(fd, buf, num_bytes_requested);
	buf[num_bytes_requested] = '\0';
	
	if (strcmp(buf, "1234567890abcdefghijABC") == 0) {
	  printf("my_test_read: PASSED 1. %i bytes read.  Data read: %s\n", result, buf);
  }
  else {
		printf("my_test_read: FAILED 1.\n");
  }

	num_bytes_requested = 5;
	result = my_read(fd, buf, num_bytes_requested);
	buf[num_bytes_requested] = '\0';
	if (strcmp(buf, "DEFGH") == 0) {
	  printf("my_test_read: PASSED 2. %i bytes read.  Data read: %s\n", result, buf);
  }
  else {
		printf("my_test_read: FAILED 2.\n");
  }	
	my_close(fd);
	
	fd = my_open("/foo2/read-test.txt");
	num_bytes_requested = 2;
	result = my_read(fd, buf, num_bytes_requested);
	buf[num_bytes_requested] = '\0';
	
	if (strcmp(buf, "12") == 0) {
	  printf("my_test_read: PASSED 3. %i bytes read.  Data read: %s\n", result, buf);
  }
  else {
		printf("my_test_read: FAILED 3.\n");
  }

	num_bytes_requested = 20;
	result = my_read(fd, buf, num_bytes_requested);
	buf[num_bytes_requested] = '\0';
	
	if (strcmp(buf, "34567890abcdefghijAB") == 0) {
	  printf("my_test_read: PASSED 4. %i bytes read.  Data read: %s\n", result, buf);
  }
  else {
		printf("my_test_read: FAILED 4.\n");
  }

	num_bytes_requested = 20;
	result = my_read(fd, buf, num_bytes_requested);
	buf[result] = '\0';
	
	if (strcmp(buf, "CDEFGHIJ") == 0) {
	  printf("my_test_read: PASSED 5. %i bytes read.  Data read: %s\n", result, buf);
  }
  else {
		printf("my_test_read: FAILED 5.\n");
  }
}

void test_search_directory_block_for_name() {
  // OPTIONAL.  Already tested via gdb
}


void test_get_path_block_num() {
  
  if (get_path_block_num("/foo2/hello.txt") < 0) {
    printf("\ntest_get_path_block_num: FAILED.\n");
  }
  else if (get_path_block_num("/foo2/hello.txt") == FILE_BLOCK_1){
    printf("\ntest_get_path_block_num: PASSED.\n");
  }
}

void test_my_creat() {
  
  int my_creat_result = 1;
  
  char path[] = "/foo2/hello2.txt";
  
  // Check if file exists already
  unsigned int directory_block_num = get_path_block_num("/foo2/");
  int filename_found = search_directory_block_for_name("hello2.txt", directory_block_num);
  if ( filename_found < 0) {
    printf("\ntest_my_creat: file does not exist yet.\n");
  }
  else {
    printf("\ntest_my_creat: file already exists.\n");
    my_creat_result = 0;
  }
  
  int fd = my_creat(path);
    
  // Check to ensure that the filename has been added to it's directory
  int block_found = search_directory_block_for_name("hello2.txt", directory_block_num);
  if ( block_found < 0) {
    printf("test_my_creat: FAILED.  File not added to directory.\n");
    my_creat_result = 0;
  }
  else {
    printf("test_my_creat: file is in directory.\n");
  }
  
  // Check file header information
  char buffer[BLOCKSIZE];
  unsigned int next_block;
  unsigned short bytes_allocated;
  
  get_file_block_and_header_information(buffer, open_files[fd], &next_block, &bytes_allocated);
  if (bytes_allocated == HEADER_SIZE && next_block == ROOT_BLOCK && buffer[0] == 'f') {
    printf("test_my_creat: file created successfully.\n");
  }
  else {
    printf("test_my_creat: file not created.\n");
    my_creat_result = 0;
  }
  
  // Check to ensure file is open
  if (fd != 0) {
    printf("test_my_creat: FAILED.  Incorrect file descriptor.\n");
    my_creat_result = 0;
  }
  else {
    printf("test_my_creat: file block returned - %i\n", open_files[fd]);
  }

  // reset free block list bit
  setBlockInBitmapToStatus (0, open_files[fd]);
  
  my_close(fd);
  if (my_creat_result) {
    printf("test_my_creat: PASSED.\n");
  }
  else {
    printf("test_my_creat: FAILED.\n");
  }
}

void test_my_open () {
  unsigned int fd = my_creat("/foo2/hello4.txt");
  unsigned int block_num = open_files[fd];
  my_close(fd);
  
  int i, first_open_slot;
  for (i = 0; i < MAX_OPEN_FILES; i++) {
    if(open_files[i] == 0) {
      first_open_slot = i;
      i = MAX_OPEN_FILES;
    }
  }
  
  fd = my_open("/foo2/hello4.txt");
  if (fd == first_open_slot) {
    printf("\ntest_my_open: PASSED\n");
  }
  else {
    printf("\ntest_my_open: FAILED\n");
  }
  
  // reset free block list bit
  setBlockInBitmapToStatus (0, open_files[fd]);
  
  my_close(fd);
}

void test_my_close () {
  unsigned int fd = my_creat("/foo2/hello3.txt");
  unsigned int block_num = open_files[fd];
  
  if (block_num == 0) {
    printf("\ntest_my_close: failed to create file\n");
    return;
  }
  
  // reset free block list bit
  setBlockInBitmapToStatus (0, open_files[fd]);
  
  my_close(fd);
  
  if (open_files[fd] == 0) {
    printf("\ntest_my_close: PASSED\n");
  }
  else {
    printf("\ntest_my_close: FAILED\n");
  }
}

void test_multiple_creat() {
	int success0, success1, success2, success3, success4;
	
	success0 = my_creat("/test0.txt");
	success1 = my_creat("/test1.txt");
	success2 = my_creat("/test2.txt");
	success3 = my_creat("/test3.txt");
	success4 = my_creat("/test4.txt");
	
	if (success0 = success1 * success2 * success3 * success4 < 0) {
		printf("\ntest_multiple_creat: FAILED\n");
	}
	printf("\ntest_multiple_creat test0.txt at block %i\n", open_files[success0]);
	printf("test_multiple_creat test1.txt at block %i\n", open_files[success1]);
	printf("test_multiple_creat test2.txt at block %i\n", open_files[success2]);
	printf("test_multiple_creat test3.txt at block %i\n", open_files[success3]);
	printf("test_multiple_creat test4.txt at block %i\n", open_files[success4]);

	if ((my_remove("/test0.txt") > 0) || (my_remove("/test1.txt") > 0) || 
		(my_remove("/test2.txt") > 0) || (my_remove("/test3.txt") > 0) || 
		(my_remove("/test4.txt") > 0)) {
	  printf("Failed at removing after multiple creates\n");
	  exit(1);
	}

	printf("test_multiple_creat removed all txt files\n");
	printf("test_multiple_creat: PASSED\n");
}

void test_my_write() {
	printf("\ntest_my_write:\n");
	if (my_mkdir("/foo") < 0) {
	  printf("Failed at making directory /foo\n");
	  exit(1);
	}

	if (my_mkdir("/foo/bar") < 0) {
	  printf("Failed at making directory /foo/bar\n");
	  exit(1);
	}

	int fd;
	if ((fd = my_creat("/foo/bar/test.txt")) < 0) {
	  printf("Failed at creating file /foo/bar/test.txt\n");
	  exit(1);
	}

	int block_num = get_associated_block_num(fd);
	printf("Created file at block number %d\n", block_num);

	char string[101] = "abcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghijabcdefghij\0";
	if (DEBUG) {
	  printf("Writing string of length 100\n");
	}

	if (my_write(fd, string, 100) < 0) {
	 printf("Failed to write string to file /foo/bar/test.txt\n");
	 exit(1);
	}

	char buffer[1024];
	if (read_block(block_num, buffer) < 0) {
	  printf("Error reading block #%d\n", block_num);
	  exit(1);
	}

	short bytesAllocated;
	memcpy(&bytesAllocated, buffer + 1 + sizeof(int), sizeof(bytesAllocated));

	if (DEBUG) {
	  printf("bytes allocated: %d\n", bytesAllocated);
	}

	char * buffer_ptr = buffer + 8;
	char data[101];
	strncpy(data, buffer_ptr, 100);
	data[100] = '\0';

	if (strcmp(data, string) != 0) {
	  printf("Error in reading back out data\n");
	  exit(1);
	}
	if (DEBUG) {
	  printf("Closing file %d\n", fd);
	}

	if (my_close(fd) < 0) {
	  printf("Failed to close file %d\n", fd);
	  exit(1);
	}

	if ((fd = my_open("/foo/bar/test.txt")) < 0) {
	  printf("Failed to open file /foo/bar/test.txt\n");
	  exit(1);
	}
	if (DEBUG) {
	  printf("Opened file %d\n", fd);
	}

	char string2[50] = "jihgfedcbajihgfedcbajihgfedcbajihgfedcbajihgfedcba";
	if (DEBUG) {
	  printf("Writing string of length 50\n");
	}

	strncpy(string, string2, 50);
	if (my_write(fd, string2, 50) < 0) {
	  printf("Failed to write string2 to /foo/bar/test.txt\n");
	  exit(1);
	}

	block_num = get_associated_block_num(fd);
	if (read_block(block_num, buffer) < 0) {
	  printf("Error reading block #%d\n", block_num);
	  exit(1);
	}

	memcpy(&bytesAllocated, buffer + 1 + sizeof(int), sizeof(bytesAllocated));
	if (DEBUG) {
	  printf("bytes allocated: %d\n", bytesAllocated);
	}

	buffer_ptr = buffer + HEADER_SIZE;
	memset(data, '\0', strlen(data));
	strncpy(data, buffer_ptr, 101);

	if (strcmp(data, string) != 0) {
	  printf("Error in reading back out data\n");
	  exit(1);
	}
	if (DEBUG) {
	  printf("Closing file %d\n", fd);
	}

	if (my_close(fd) < 0) {
	  printf("Failed to close file %d\n", fd);
	  exit(1);
	}

	if ((fd = my_open("/foo/bar/test.txt")) < 0) {
	  printf("Failed to open file /foo/bar/test.txt\n");
	  exit(1);
	}
	if (DEBUG) {
	  printf("Opened file %d\n", fd);
	}

	block_num = get_associated_block_num(fd);

	char alphabet[26] = "abcdefghijlkmnopqrstuvwxyz";
	char longString[1501];
	int i;
	for (i = 0; i < 1500; i++) {
	  longString[i] = alphabet[(i % 26)];
	}
	longString[1500] = '\0';

	if (DEBUG) {
	  printf("Writing string of length 1500\n");
	}

	if (my_write(fd, longString, 1500) < 0) {
	  printf("Failed to write string2 to /foo/bar/test.txt\n");
	  exit(1);
	}

	if (read_block(block_num, buffer) < 0) {
	  printf("Error reading block #%d\n", block_num);
	  exit(1);
	}

	memcpy(&bytesAllocated, buffer + 1 + sizeof(int), sizeof(bytesAllocated));
	if (DEBUG) {
	  printf("bytes allocated in first block excluding header: %d\n", (bytesAllocated - HEADER_SIZE));
	}

	if (bytesAllocated != 1024) {
	 printf("Error allocated %d bytes\n", bytesAllocated);
	}

	int next_block;
	memcpy(&next_block, buffer + 1, sizeof(next_block));

	if (next_block == 0) {
	 printf("Didn't properly create a new block.\n");
	 exit(1);
	}
	else {
	  printf("Created new file block at block %d, when writing\n", next_block);
	}

	char new_buffer[1024];
	if (read_block(next_block, new_buffer) < 0) {
	  printf("Error reading block #%d\n", next_block);
	  exit(1);
	}

	short newBytesAllocated;
	memcpy(&newBytesAllocated, new_buffer + 1 + sizeof(int), sizeof(newBytesAllocated));
	if (DEBUG) {
	  printf("Number of bytes allocated in second block excluding header: %d\n", (newBytesAllocated - HEADER_SIZE));
	  printf("Total number of bytes allocated from string of length 1500: %d\n", (bytesAllocated + newBytesAllocated - (2 * HEADER_SIZE)));
	}

	char longData[1501];
	buffer_ptr = buffer + HEADER_SIZE;
	strncpy(longData, buffer_ptr, (bytesAllocated - HEADER_SIZE));
	char * longData_ptr = longData + (bytesAllocated - HEADER_SIZE);
	char * new_buffer_ptr = new_buffer + HEADER_SIZE;
	strncpy(longData_ptr, new_buffer_ptr, (newBytesAllocated - HEADER_SIZE));
	longData[1500] = '\0';

	if (strcmp(longString, longData) != 0) {
	  printf("Error in writing data\n");
	  exit(1);
	}

	char longerString[1601];
	for (i = 0; i < 1600; i++) {
      if (i < 1500) {
	    longerString[i] = longString[i];
	  }
	  else {
		longerString[i] = string[(i - 1500)];
	  }
	}
	longerString[1600] = '\0';

	if (DEBUG) {
	  printf("Appending string of length 100 to open file %d\n", fd);
	}

	if (my_write(fd, string, 100) < 0) {
	  printf("Failed to write string2 to /foo/bar/test.txt\n");
	  exit(1);
	}

	if (read_block(block_num, buffer) < 0) {
	  printf("Error reading block #%d\n", block_num);
	  exit(1);
	}

	buffer_ptr = buffer + 1;
	memcpy(&next_block, buffer_ptr, sizeof(next_block));

	if (read_block(next_block, buffer) < 0) {
	  printf("Error reading block #%d\n", block_num);
	  exit(1);
	}

	buffer_ptr = buffer + 1 + sizeof(next_block);
	memcpy(&bytesAllocated, buffer_ptr, sizeof(bytesAllocated));

	if (bytesAllocated != (newBytesAllocated + 100)) {
	  printf("Error not enough bytes allocated\n");
	  exit(1);
	}

	if (DEBUG) {
	  printf("bytes allocated in block appended too excluding header: %d\n", (bytesAllocated - HEADER_SIZE));
	}

	buffer_ptr = buffer + newBytesAllocated;
	memset(data, '\0', 100);
	strncpy(data, buffer_ptr, 100);
	data[100] = '\0';

	if (strcmp(data, string) != 0) {
	  printf("Error in writing to already written to file that wasn't closed.\n");
	  exit(1);
	}

	if (DEBUG) {
	  printf("Recursively removing directory /foo and all it's contents\n");
	}

	if (my_rmdir("/foo") < 0) {
	  printf("Failed at removing directory /foo\n");
	  exit(1);
	}

	block_num = get_associated_block_num(fd);
	if (block_num != 0) {
	 printf("Error file still seen as open\n");
	 exit(1);
	}

	printf("Recursively deleted directory /foo\n");
	printf("test_my_write: PASSED\n");
}

void test_my_remove() {
	printf("\ntest_my_remove:\n");
  	int fd, fd2, fd3, fd4, fd5, fd6, fd7;
	if ((fd = my_creat("/test.txt")) < 0) {
	  printf("Failed at creating file /test.txt\n");
	  exit(1);
	}

	if ((fd2 = my_creat("/test2.txt")) < 0) {
	  printf("Failed at creating file /test.txt\n");
	  exit(1);
	}

	if ((fd3 = my_creat("/test3.txt")) < 0) {
	  printf("Failed at creating file /test.txt\n");
	  exit(1);
	}

	if ((fd4 = my_creat("/test4.txt")) < 0) {
	  printf("Failed at creating file /test.txt\n");
	  exit(1);
	}

	if ((fd5 = my_creat("/test5.txt")) < 0) {
	  printf("Failed at creating file /test.txt\n");
	  exit(1);
	}

	if ((fd6 = my_creat("/test6.txt")) < 0) {
	  printf("Failed at creating file /test.txt\n");
	  exit(1);
	}

	if ((fd7 = my_creat("/test7.txt")) < 0) {
	  printf("Failed at creating file /test.txt\n");
	  exit(1);
	}

	int block_num, block_num_2, block_num_3, block_num_4, block_num_5, block_num_6, block_num_7;
	block_num = get_associated_block_num(fd);
	block_num_2 = get_associated_block_num(fd2);
	block_num_3 = get_associated_block_num(fd3);
	block_num_4 = get_associated_block_num(fd4);
	block_num_5 = get_associated_block_num(fd5);
	block_num_6 = get_associated_block_num(fd6);
	block_num_7 = get_associated_block_num(fd7);
	printf("Created file /test.txt at block %d\n", block_num);
	printf("Created file /test2.txt at block %d\n", block_num_2);
	printf("Created file /test3.txt at block %d\n", block_num_3);
	printf("Created file /test4.txt at block %d\n", block_num_4);
	printf("Created file /test5.txt at block %d\n", block_num_5);
	printf("Created file /test6.txt at block %d\n", block_num_6);
	printf("Created file /test7.txt at block %d\n", block_num_7);

	if (my_remove("/test.txt") < 0) {
	  printf("Failed to remove file /test.txt\n");
	  exit(1);
	}

	if (my_remove("/test2.txt") < 0) {
	  printf("Failed to remove file /test2.txt\n");
	  exit(1);
	}

	if (my_remove("/test3.txt") < 0) {
	  printf("Failed to remove file /test3.txt\n");
	  exit(1);
	}

	if (my_remove("/test4.txt") < 0) {
	  printf("Failed to remove file /test4.txt\n");
	  exit(1);
	}

	if (my_remove("/test5.txt") < 0) {
	  printf("Failed to remove file /test5.txt\n");
	  exit(1);
	}

	if (my_remove("/test6.txt") < 0) {
	  printf("Failed to remove file /test6.txt\n");
	  exit(1);
	}

	if (my_remove("/test7.txt") < 0) {
	  printf("Failed to remove file /test7.txt\n");
	  exit(1);
	}

	block_num = get_associated_block_num(fd);
	block_num_2 = get_associated_block_num(fd2);
	block_num_3 = get_associated_block_num(fd3);
	block_num_4 = get_associated_block_num(fd4);
	block_num_5 = get_associated_block_num(fd5);
	block_num_6 = get_associated_block_num(fd6);
	block_num_7 = get_associated_block_num(fd7);

	if (block_num != 0 || block_num_2 != 0 || block_num_3 != 0 || block_num_4 != 0 
		|| block_num_5 != 0 || block_num_6 != 0 || block_num_7 != 0) {
	  printf("BLOCK 1: %d\nBLOCK 2: %d\nBLOCK 3: %d\nBLOCK 4: %d\nBLOCK 5: %d\nBLOCK 6: %d\nBLOCK 7: %d\n", 
		  block_num, block_num_2, block_num_3, block_num_4, block_num_5, block_num_6, block_num_7);
	  exit(1);
	}

	printf("Successfully removed all files\n");
	printf("test_my_remove: PASSED\n");
}

int main (char argc, char ** argv)  {
  test_file_operations_setup();
  
  /** TESTED - PASS **/
  test_get_path_block_num();
  
  /** TESTED - PASS **/
  test_my_creat();

  /** TESTED - PASS **/
  test_multiple_creat();

  /** TESTED - PASS **/
  test_my_close();
  
  /** TESTED - PASS **/
  test_my_open();

  /** TESTED - PASS **/
  test_my_write();

  /** TESTED - PASS **/
  test_my_remove();
  
  /** TESTED - ??? **/
  test_my_read();
  
  return 0;
}