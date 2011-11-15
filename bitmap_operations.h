#ifndef BITMAP_OPERATIONS_H
#define BITMAP_OPERATIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p6.h"
#include "block.h"

int requestNextFreeBlock ();
int freeBlocks (int n, int *listOfBlocks);
int setBlockInBitmapToStatus (int status, int blockNumber);

#endif
