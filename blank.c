/*
 * blank.c
 * Detection of Blank Disk (Formatted)
 *
 * Written By Aaron Geyer of StorageQuest, Inc.
 *
 * Copyright (c) 2003 Aaron Geyer
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE. 
 *
 */

#include "global.h"

/* The principle used to determine whether or not a disk
   is blank is to search the disk for a byte code that
   does not change over the disk.  This module only scans the first
   2MB of the disk.  This module will need more work. */


void detect_blank(SECTION *section, int level)
{
  unsigned char *buffer;
  int i, j;
  int max_blocks = 4096;
  int block_size = 512;
  unsigned char code;

  if(get_buffer(section, 0, 1, (void **)&buffer) < 1)
    return;
  code = buffer[0];

  // Look at first 4096 blocks of 512 bytes
  for(i = 0; i < max_blocks; i++) {

    if(get_buffer(section, i * block_size, block_size, (void **)&buffer) < block_size)
      return;

    for(j = 0; j < block_size; j++) {
      
      if (buffer[j] != code)
	return;
    }
  }

  print_line(level+0, "Blank disk/medium");
}
