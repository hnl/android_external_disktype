/*
 * apple.c
 * Detection of Apple partition maps and file systems
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
 * Apple partition map detection
 */

void detect_apple_partmap(SECTION *section, int level)
{
  int i, magic, count;
  char s[256];
  unsigned char *buf;
  u8 start, size;
  SECTION rs;

  /* partition maps only occur at the start of a device */
  if (section->pos != 0)
    return;

  /*
    if (buf[off] == 0x45 && buf[off+1] == 0x52) {
      print_line(level, "HFS driver description map at sector %d", i);
    }
  */

  if (get_buffer(section, 512, 512, (void **)&buf) < 512)
    return;

  magic = get_be_short(buf);
  if (magic == 0x5453) {
    print_line(level, "Old-style Apple partition map");
    return;
  }
  if (magic != 0x504D)
    return;

  /* get partition count and print info */
  count = get_be_long(buf + 4);
  print_line(level, "Apple partition map, %d entries", count);

  for (i = 1; i <= count; i++) {
    /* read the right sector */
    /* NOTE: the previous run through the loop might have called
     *  get_buffer indirectly, invalidating our old pointer */
    if (i > 1 && get_buffer(section, i * 512, 512, (void **)&buf) < 512)
      return;

    /* check signature */
    if (get_be_short(buf) != 0x504D) {
      print_line(level, "Partition %d: invalid signature, skipping", i);
      continue;
    }

    /* get position and size */
    start = get_be_long(buf + 8);
    size = get_be_long(buf + 12);
    format_size(s, size, 512);
    print_line(level, "Partition %d: %s (%llu sectors starting at %llu)",
	       i, s, size, start);

    /* get type */
    memcpy(s, buf + 48, 32);
    s[32] = 0;
    print_line(level+1, "Type \"%s\"", s);

    /* recurse for content detection */
    if (start > count) {  /* avoid recursion on self */
      rs.source = section->source;
      rs.pos = section->pos + start * 512;
      rs.size = size * 512;
      rs.flags = section->flags;
      detect(&rs, level + 1);
    }
  }
}

/*
 * Apple volume formats: MFS, HFS, HFS Plus
 */

void detect_apple_volume(SECTION *section, int level)
{
  char s[256], t[256];
  unsigned char *buf;
  u2 magic, version;
  u4 blocksize, blockstart;
  u8 blockcount, offset;
  SECTION rs;

  if (get_buffer(section, 1024, 512, (void **)&buf) < 512)
    return;

  magic = get_be_short(buf);
  version = get_be_short(buf + 2);

  if (magic == 0xD2D7) {
    print_line(level, "MFS file system");

  } else if (magic == 0x4244) {
    print_line(level, "HFS file system");
    blockcount = get_be_short(buf + 18);
    blocksize = get_be_long(buf + 20);
    blockstart = get_be_short(buf + 28);

    get_pstring(buf + 36, s);
    format_ascii(s, t);
    print_line(level + 1, "Volume name \"%s\"", t);

    format_size(s, blockcount, blocksize);
    print_line(level + 1, "Volume size %s (%llu blocks of %lu bytes)",
	       s, blockcount, blocksize);

    if (get_be_short(buf + 0x7c) == 0x482B) {
      print_line(level, "HFS wrapper for HFS Plus");

      offset = (u8)get_be_short(buf + 0x7e) * blocksize +
	(u8)blockstart * 512;

      rs.source = section->source;
      rs.pos = section->pos + offset;
      rs.size = 0;  /* TODO */
      rs.flags = section->flags;
      detect(&rs, level + 1);
    }

  } else if (magic == 0x482B) {
    print_line(level, "HFS Plus file system");

    blocksize = get_be_long(buf + 40);
    blockcount = get_be_long(buf + 44);

    format_size(s, blockcount, blocksize);
    print_line(level + 1, "Volume size %s (%llu blocks of %lu bytes)",
	       s, blockcount, blocksize);

    /* TODO: is there a volume name somewhere? */
  }
}

/* EOF */
