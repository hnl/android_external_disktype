/*
 * amiga.c
 * Detection of Amiga partition maps and file systems
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
 * Amiga "Rigid Disk" partition map
 */

void detect_amiga_partmap(SECTION *section, int level)
{
  int i, off, found;
  unsigned char *buf;
  char s[256];
  u4 blocksize, part_ptr;
  u8 cylsize, start, size;

  for (off = 0, found = 0; off < 16; off++) {
    if (get_buffer(section, off * 512, 512, (void **)&buf) < 512)
      break;

    if (memcmp(buf, "RDSK", 4) == 0) {
      found = 1;
      break;
    }
  }
  if (!found)
    return;

  if (off == 0)
    print_line(level, "Amiga Rigid Disk partition map");
  else
    print_line(level, "Amiga Rigid Disk partition map at sector %d", off);

  /* get device block size (?) */
  blocksize = get_be_long(buf + 16);
  if (blocksize < 256 || (blocksize & (blocksize-1))) {
    print_line(level+1, "Illegal block size %lu", blocksize);
    return;
  } else if (blocksize != 512) {
    print_line(level+1, "Unusual block size %lu, not sure this will work...", blocksize);
  }
  /* TODO: get geometry data for later use */

  /* walk the partition list */
  part_ptr = get_be_long(buf + 28);
  for (i = 1; part_ptr != 0xffffffffUL; i++) {
    if (get_buffer(section, (u8)part_ptr * 512, 256,
		   (void **)&buf) < 256) {
      print_line(level, "Partition %d: Can't read partition info block");
      break;
    }

    /* check signature */
    if (memcmp(buf, "PART", 4) != 0) {
      print_line(level, "Partition %d: Invalid signature");
      break;
    }

    /* get "next" pointer for next iteration */
    part_ptr = get_be_long(buf + 16);

    /* get sizes */
    cylsize = (u8)get_be_long(buf + 140) * (u8)get_be_long(buf + 148);
    start = get_be_long(buf + 164) * cylsize;
    size = (get_be_long(buf + 168) + 1 - get_be_long(buf + 164)) * cylsize;
    format_size(s, size, 512);
    print_line(level, "Partition %d: %s (%llu sectors starting at %llu)",
               i, s, size, start);

    /* get name */
    get_pstring(buf + 36, s);
    if (s[0])
      print_line(level + 1, "Drive name \"%s\"", s);

    /*
 192/c0 char4DosType 'DOS' and the FFS/OFS flag only
      also 'UNI'\0 = AT&T SysV filesystem
      'UNI'\1 = UNIX boot filesystem
      'UNI'\2 = BSD filesystem for SysV
      'resv' = reserved (swap space)
    */

    /* detect contents */
    if (size > 0 && start > 0) {
      analyze_recursive(section, level + 1,
			start * 512, size * 512, 0);
    }
  }
}

/*
 * Amiga file system
 */

void detect_amiga_fs(SECTION *section, int level)
{
  unsigned char *buf;
  int flags;
  char s[256];

  if (get_buffer(section, 0, 512, (void **)&buf) < 512)
    return;

  if (memcmp(buf, "DOS", 3) == 0){
    flags = buf[4];
    if (flags & 1)
      strcpy(s, "Amiga FFS file system");
    else
      strcpy(s, "Amiga OFS file system");
    if (flags & 4)
      strcat(s, " (intl., dir cache)");
    else if (flags & 2)
      strcat(s, " (intl., no dir cache)");
    else
      strcat(s, " (non-intl., no dir cache)");

    print_line(level, "%s", s);

    if (section->size == 512*11*2*80) {
      print_line(level+1, "Size matches DD floppy");
    } else if (section->size == 512*22*2*80) {
      print_line(level+1, "Size matches HD floppy");
    }

  } else if (memcmp(buf, "PFS", 3) == 0) {
    print_line(level, "Amiga Professional File System");
  }
}

/* EOF */
