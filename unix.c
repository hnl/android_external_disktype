/*
 * unix.c
 * Detection of general Unix file systems
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
 * JFS (for Linux)
 */

void detect_jfs(SECTION *section, int level)
{
  unsigned char *buf;
  int version;
  char s[256];
  u4 blocksize;
  u8 blockcount;

  if (get_buffer(section, 32768, 512, (void **)&buf) < 512)
    return;

  /* check signature */
  if (memcmp(buf, "JFS1", 4) != 0)
    return;

  /* tell the user */
  version = get_le_long(buf + 4);
  print_line(level, "JFS file system, version %d", version);

  memcpy(s, buf + 101, 11);
  s[11] = 0;
  print_line(level + 1, "Volume name \"%s\"", s);

  blocksize = get_le_long(buf + 24);
  blockcount = get_le_quad(buf + 8);
  format_size(s, blockcount, blocksize);
  print_line(level + 1, "Volume size %s (%llu h/w blocks of %lu bytes)",
	     s, blockcount, blocksize);
}

/*
 * XFS
 */

void detect_xfs(SECTION *section, int level)
{
  unsigned char *buf;
  u4 raw_version, blocksize;
  u8 blockcount;
  int version;
  char s[256];

  if (get_buffer(section, 0, 512, (void **)&buf) < 512)
    return;

  /* check signature */
  if (get_be_long(buf) != 0x58465342)
    return;

  /* tell the user */
  raw_version = get_be_short(buf + 0x64);
  version = raw_version & 0x000f;
  print_line(level, "XFS file system, version %d", version);

  memcpy(s, buf + 0x6c, 12);
  s[12] = 0;
  print_line(level + 1, "Volume name \"%s\"", s);

  format_uuid(buf + 32, s);
  print_line(level + 1, "UUID %s", s);

  blocksize = get_be_long(buf + 4);
  blockcount = get_be_quad(buf + 8);
  format_size(s, blockcount, blocksize);
  print_line(level + 1, "Volume size %s (%llu blocks of %lu bytes)",
	     s, blockcount, blocksize);
}

/*
 * UFS file system from various BSD strains
 */

void detect_ufs(SECTION *section, int level)
{
  unsigned char *buf;
  int i, at, en, offsets[5] = { 0, 8, 64, 256, -1 };
  u4 magic;
  char s[512];

  /* NextStep/OpenStep apparently can move the superblock further into
     the device. Linux uses steps of 8 blocks (of the applicable block
     size) to scan for it. We only scan at the canonical location for now. */
  /* Possible block sizes: 512 (old formats), 1024 (most), 2048 (CD media) */

  for (i = 0; offsets[i] >= 0; i++) {
    at = offsets[i];
    if (get_buffer(section, at * 1024, 1536, (void **)&buf) < 1536)
      break;

    for (en = 0; en < 2; en++) {
      magic = get_ve_long(en, buf + 1372);

      if (magic == 0x00011954) {
	print_line(level, "UFS file system, %s",
		   get_ve_name(en));
      } else if (magic == 0x00095014) {
	print_line(level, "UFS file system, long file names, %s",
		   get_ve_name(en));
      } else if (magic == 0x00195612) {
	print_line(level, "UFS file system, fs_featurebits, %s",
		   get_ve_name(en));
      } else if (magic == 0x05231994) {
	print_line(level, "UFS file system, fs_featurebits, >4GB support, %s",
		   get_ve_name(en));
      } else if (magic == 0x19540119) {
	print_line(level, "UFS2 file system, %s",
		   get_ve_name(en));
      } else
	continue;

      /* get volume name */
      memcpy(s, buf + 680, 32);
      s[32] = 0;
      if (s[0])
	print_line(level + 1, "Volume name \"%s\"", s);

      /* last mount point */
      memcpy(s, buf + 212, 468);
      s[468] = 0;
      if (s[0])
	print_line(level + 1, "Last mounted at \"%s\"", s);
    }
  }
}

/* EOF */
