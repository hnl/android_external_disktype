/*
 * udf.c
 * Detection of UDF file system
 *
 * UDF support integrated by Aaron Geyer of StorageQuest, Inc.
 *
 * UDF sniffer based on Richard Freeman's work at StorageQuest, Inc.
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

void detect_udf(SECTION *section, int level)
{
  unsigned char *buffer;
  unsigned char signature1[7] = {0x00, 0x42, 0x45, 0x41, 0x30, 0x31, 0x01};
  unsigned char signature2[7] = {0x00, 0x4e, 0x53, 0x52, 0x30, 0x32, 0x01};
  unsigned char signature3[7] = {0x00, 0x54, 0x45, 0x41, 0x30, 0x31, 0x01};
  unsigned char signature4[4] = {0x08, 0x00, 0x02, 0x00};
  unsigned char signature5[6] = {0x01, 0x00, 0x00, 0x00, 0xf0, 0x01};

  int sector_size;
  int detected = 0;
  int count, addr ;
  int sect ;
  int checksum, cksum ;
  int i, j ;

  /* seek past the 32KB lead in and detect signatures in order to
     determin sector size */
  if (get_buffer(section, 32768, 512, (void **)&buffer) < 512)
    return ;

  if(memcmp(buffer, signature1, 7) != 0)
    return ;

  if (get_buffer(section, 34816, 512, (void **)&buffer) < 512)
    return ;

  if(memcmp(buffer, signature2, 7) != 0)
    return ;

  if (get_buffer(section, 36864, 512, (void **)&buffer) < 512)
    return ;

  if(memcmp(buffer, signature3, 7) != 0)
    return ;

  // Determine sector size by comparing relative location of signatures
  // minimum sector size is 512 bytes
  sector_size = 512 ;

  while(!detected) {
    if(get_buffer(section, 38912 + sector_size, 512, (void **)&buffer) < 512)
      return ;
    
    if(memcmp(buffer, signature4, 4) == 0 && memcmp(buffer + 6, signature5, 6) == 0)
    {
      detected = 1 ;
      break ;
    }

    sector_size = sector_size + 512 ;
  }

  if(detected) {
  
    //Look at Sector 256: 

    // first read the anchor volume descriptor pointer
    if(get_buffer (section, sector_size * 256, 512, (void **)&buffer) < 512)
      return ;

    // get the volume descriptor area
    count = get_le_long (buffer + 16) / sector_size;
    addr = get_le_long (buffer + 20);
    //if (addr > media_size) break;

    // look for a logical volume descriptor and validate CRC
    for (i = 0; i < count; i++) {
      sect = addr + i;
      if(get_buffer (section, (sector_size * sect), 512, (void **)&buffer) < 512)
	return ;
      if (buffer[0] != 6) continue;
      if (buffer[1] != 0) continue;
      checksum = buffer[4];
      buffer[4] = 0;
      cksum = 0;
      for (j = 0; j < 16; j++) cksum += buffer[j];
      if ((cksum & 0xFF) != checksum) continue;
      if (buffer[5] != 0) continue;
      if (buffer[12] != (sect & 0xFF)) continue;
      if (buffer[13] != ((sect >> 8) & 0xFF)) continue;
      if (buffer[14] != ((sect >> 16) & 0xFF)) continue;
      if (buffer[15] != ((sect >> 24) & 0xFF)) continue;
      break;
    }
    print_line(level, "UDF file system") ;   // done
    print_line(level + 1, "Sector size %d bytes", sector_size) ;
  }
}

/* EOF */
