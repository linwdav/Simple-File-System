#include "test_file_operations.h"

#define DIRECTORY_BLOCK_0            50
#define DIRECTORY_BLOCK_1            60
#define DIRECTORY_BLOCK_1_NEXT_BLOCK 70
#define DIRECTORY_BLOCK_1_1          80
#define FILE_BLOCK_1                 100

int size_of_disk;

/*
 * Hard codes the following directory structure:
 * root
 *   foo1 50  (Directory block 0)
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


void test_search_directory_block_for_name() {
  
}


void test_get_path_block_num() {
  
  if (get_path_block_num("/foo2/hello.txt") < 0) {
    printf("\ntest_get_path_block_num FAILED.\n");
  }
  else if (get_path_block_num("/foo2/hello.txt") == FILE_BLOCK_1){
    printf("\ntest_get_path_block_num PASSED.\n");
  }
}

void test_my_creat() {
  char path[] = "/foo2/hello2.txt";
  
  // Check if file exists already
  unsigned int directory_block_num = get_path_block_num("/foo2/");
  unsigned int filename_found = search_directory_block_for_name("hello2.txt", directory_block_num);
  if ( filename_found == 0) {
    printf("test_my_creat: file does not exist yet.\n");
  }
  else {
    printf("test_my_creat: file already exists.\n");
  }
  
  int fd = my_creat(path);
    
  // Check to ensure that the filename has been added to it's directory
  unsigned int block_found = search_directory_block_for_name("hello2.txt", directory_block_num);
  if ( block_found == 0) {
    printf("test_my_creat FAILED.  File not added to directory.\n");
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
  }
  
  // Check to ensure file is open
  if (fd != 0) {
    printf("\ntest_my_creat FAILED.  Incorrect file descriptor.\n");
  }
  else {
    printf("\ntest_my_creat: file block returned - %i\n", open_files[fd]);
  }
  my_close(fd);
}

void test_my_open () {
  
}

void test_my_close () {
  
}

int main (char argc, char ** argv)  {
  test_file_operations_setup();
  
  /** TESTED - PASS **/
  test_get_path_block_num();
  
  test_my_creat();
  
  return 0;
}