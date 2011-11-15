/* project 6 header file for UHM ICS 612, Spring 2007*/

#ifndef P6_H
#define P6_H

// File System Blocks
#define NUM_SYSTEM_BLOCKS       2
#define ROOT_BLOCK              0
#define FREE_LIST_BITMAP_PARAMS 1
#define FREE_LIST_BITMAP_START  2

#define MAX_FILE_SIZE           (2 * 1048576) // 2MB
#define MAX_FILE_NAME_LENGTH    200
#define MAX_PATH_LENGTH         1024
#define MAX_OPEN_FILES          10

#define HEADER_SIZE             8
#define BYTES_IN_SHORT          2
#define BYTES_IN_INT            4
#define BITS_IN_BYTE            8
#define BITS_IN_BLOCK           (8 * BLOCKSIZE)
#define PATH_SEPARATOR          '/'

/* file API */
extern int my_open (const char * path);
extern int my_creat (const char * path);
extern int my_read (int fd, void * buf, int count);
extern int my_write (int fd, const void * buf, int count);
extern int my_close (int fd);

extern int my_remove (const char * path);
extern int my_rename (const char * old, const char * new);
extern int my_mkdir (const char * path);
extern int my_rmdir (const char * path);

extern void my_mkfs ();

/* provided by the lower layer */

#define BLOCKSIZE 1024
/* not used in any declaration, just a reminder that each block is 1KB */
/* and may be useful inside the code. */
typedef char block [BLOCKSIZE];

extern int dev_open ();
extern int read_block (int block_num, char * block);
extern int write_block (int block_num, char * block);

#endif /* P6_H */
