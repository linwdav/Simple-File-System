#include "test_file_operations.h"

void test_file_operations_setup() {
  int size_of_disk = dev_open();
  my_mkfs();
}

void test_search_directory_block_for_name()  {
  test_file_operations_setup();
  printf("hello");
}

int main (char argc, char ** argv)  {
  test_search_directory_block_for_name();
  return 0;
}