/*
 * unix.c
 * Detection of Unix file systems and boot codes
 *
 * Copyright (c) 2003 Christoph Pfisterer
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
 */

#include "global.h"

/*
 * ext2/ext3 file system
 */

void detect_ext23(int8 pos, int level)
{
  unsigned char *buf;
  char s[256];
  int blocksize;
  int8 blockcount;

  if (get_buffer(pos + 1024, 1024, (void **)&buf) < 1024)
    return;

  if (get_le_short(buf + 56) == 0xEF53) {
    print_line(level, "Ext2/Ext3 file system");

    memcpy(s, buf + 120, 16);
    s[16] = 0;
    if (s[0])
      print_line(level + 1, "Volume name \"%s\"", s);

    memcpy(s, buf + 136, 64);
    s[64] = 0;
    if (s[0])
      print_line(level + 1, "Last mounted at \"%s\"", s);

    blocksize = 1024 << get_le_long(buf + 24);
    blockcount = get_le_long(buf + 4);
    format_size(s, blockcount, blocksize);
    print_line(level + 1, "Volume size %s (%lld blocks of %d bytes)",
	       s, blockcount, blocksize);

    /* 76 4 s_rev_level */
    /* 62 2 s_minor_rev_level */
    /* 72 4 s_creator_os */
    /* 92 3x4 s_feature_compat, s_feature_incompat, s_feature_ro_compat */
    /* 104 16 s_uuid */
  }
}

/*
 * various signatures
 */

void detect_unix_misc(int8 pos, int level)
{
  int magic, fill;
  unsigned char *buf;

  fill = get_buffer(pos, 4096, (void **)&buf);
  if (fill < 512)
    return;

  /* boot sector stuff */
  if (memcmp(buf + 2, "LILO", 4) == 0)
    print_line(level, "LILO boot code");
  if (memcmp(buf + 3, "SYSLINUX", 8) == 0)
    print_line(level, "SYSLINUX boot code");
  if (find_memory(buf, 512, "GRUB ", 5) >= 0)
    print_line(level, "GRUB boot code");
  if (fill >= 1024 && find_memory(buf, 1024, "ISOLINUX", 8) >= 0)
    print_line(level, "ISOLINUX boot code");

  /* Debian install floppy splitter */
  if (memcmp(buf, "Floppy split ", 13) == 0) {
    char *name = (char *)buf + 32;
    char *number = (char *)buf + 164;
    char *total = (char *)buf + 172;
    print_line(level, "Debian floppy split, name \"%s\", disk %s of %s",
	       name, number, total);
  }

  /* minix file system */
  if (fill >= 2048) {
    magic = get_le_short(buf + 1024 + 0x10);
    if (magic == 0x137F)
      print_line(level, "Minix file system (v1, 14 chars)");
    if (magic == 0x138F)
      print_line(level, "Minix file system (v1, 30 chars)");
    if (magic == 0x2468)
      print_line(level, "Minix file system (v2, 14 chars)");
    if (magic == 0x2478)
      print_line(level, "Minix file system (v2, 30 chars)");
  }

  /* more Linux stuff */
  if (fill >= 4096 && memcmp((char *)buf + 4096 - 10, "SWAP-SPACE", 10) == 0) {
    print_line(level, "Linux swap, version 1");
    /* TODO: size */
  }
  if (fill >= 4096 && memcmp((char *)buf + 4096 - 10, "SWAPSPACE2", 10) == 0) {
    print_line(level, "Linux swap, version 2");
    /* TODO: size */
  }
  if (fill >= 1024 && memcmp((char *)buf + 512 + 2, "HdrS", 4) == 0) {
    print_line(level, "Linux kernel build-in loader");
  }
}

/* EOF */
