#include "test_directory_operations.h"

void main(void) {
  int size_of_disk = dev_open();
  
  if (size_of_disk != 250000) {
    printf("\ntest_file_operations_setup FAIL.  Size of disk must be 250,000 blocks.\n");
  }
  
  my_mkfs();

  if (my_mkdir("/foo") < 0) {
	printf("Couldn't create directory /foo\n");
	exit(1);
  }

  if (my_mkdir("/foo/bar") < 0) {
	printf("Couldn't create directory /foo/bar\n");
	exit(1);
  }

  if (my_mkdir("/foo/baz") < 0) {
	printf("Couldn't create directory /foo/baz\n");
	exit(1);
  }

  if (my_mkdir("/foo/gas") < 0) {
	printf("Couldn't create directory /foo/gas\n");
	exit(1);
  }

  if (my_mkdir("/foo/hello") < 0) {
	printf("Couldn't create directory /foo/hello\n");
	exit(1);
  }

  if (my_mkdir("/foo/world") < 0) {
	printf("Couldn't create directory /foo/world\n");
	exit(1);
  }

  if (my_mkdir("/foo/again") < 0) {
	printf("Couldn't create directory /foo/again\n");
	exit(1);
  }

  if (my_rmdir("/foo") < 0) {
	printf("Couldn't remove directory /foo\n");
	exit(1);
  }

  if (my_mkdir("/foo") < 0) {
	printf("Couldn't create directory /foo\n");
	exit(1);
  }

  if (my_mkdir("/foo/bar") < 0) {
	printf("Couldn't create directory /foo/bar\n");
	exit(1);
  }

  if (my_mkdir("/foo/bar/baz") < 0) {
	printf("Couldn't create directory /foo/bar/baz\n");
	exit(1);
  }

  if (my_rename("/foo/bar/baz", "/foo/bar") > -1) {
	printf("Error: Renamed /foo/bar/baz to existing directory /foo/bar\n");
	exit(1);
  }

  if (my_rename("/foo/bar/baz", "/foo/baz") < 0) {
	printf("Unable to rename directory\n");
	exit(1);
  }

  if (my_rmdir("/foo") < 0) {
	printf("Couldn't remove directory /foo\n");
	exit(1);
  }
}