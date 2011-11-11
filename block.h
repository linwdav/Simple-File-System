/* blocks.c: routines to read and write blocks from a file. */
/* Copyright (C) 2007  Edoardo S. Biagioni

 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * this program is free.  You are welcome to use it if you are
 * my student or if you find it useful, but there are no guarantees
 * it will do what you want.
 */

#ifndef BLOCK_H
#define BLOCK_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "p6.h"

/* only open the file once */
static int fd = -1;
static int devsize = 0;

/* returns the device size (in blocks) if the operation is successful,
 * and -1 otherwise */
int dev_open ();
   
/* returns 0 if the operation is successful, and -1 otherwise */
int read_block (int block_num, char * block);

/* returns 0 if the operation is successful, and -1 otherwise */
int write_block (int block_num, char * block);
#endif /* BLOCK_H */
