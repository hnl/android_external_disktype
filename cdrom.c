/*
 * cdrom.c
 * Detection of file systems for CD-ROM and similar media
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
 * sub-functions
 */

static void dump_boot_catalog(int8 basepos, int8 pos, int level);

/*
 * ISO9660 file system
 */

void detect_iso(int8 pos, int level)
{
  char s[256], t[256];
  int i, sector, type, blocksize;
  int8 blocks, bcpos;
  unsigned char *buf;

  /* get the volume descriptor */
  if (get_buffer(pos + 32768, 2048, (void **)&buf) < 2048)
    return;

  /* check signature */
  if (memcmp(buf, "\001CD001", 6) != 0)
    return;

  print_line(level, "ISO9660 file system");

  /* read Volume ID */
  memcpy(s, buf + 40, 32);
  s[32] = 0;
  for (i = 31; i >= 0 && s[i] == 32; i--)
    s[i] = 0;
  print_line(level+1, "Volume name \"%s\"", s);

  /* some other interesting facts */
  blocks = get_le_long(buf + 80);
  blocksize = get_le_short(buf + 128);
  format_size(s, blocks, blocksize);
  print_line(level+1, "Data Size %s (%lld blocks of %d bytes)",
	     s, blocks, blocksize);

  for (sector = 17; ; sector++) {
    /* get next descriptor */
    if (get_buffer(pos + sector * 2048, 2048, (void **)&buf) < 2048)
      return;

    /* check signature */
    if (memcmp(buf + 1, "CD001", 5) != 0) {
      print_line(level+1, "Signature missing in sector %d", sector);
      return;
    }
    type = buf[0];
    if (type == 255)
      break;

    switch (type) {

    case 0:  /* El Torito */
      /* check signature */
      if (memcmp(buf+7, "EL TORITO SPECIFICATION", 23) != 0) {
	print_line(level+1, "Boot record of unknown format");
	break;
      }

      bcpos = get_le_long(buf + 0x47);
      print_line(level+1, "El Torito boot record, catalog at %lld", bcpos);

      /* boot catalog */
      bcpos = pos + bcpos * 2048;
      dump_boot_catalog(pos, bcpos, level + 2);

      break;

    case 2:  /* Joliet */
      /* read Volume ID */
      memcpy(s, buf + 40, 32);
      s[32] = 0;
      s[33] = 0;
      format_unicode(s, t);
      for (i = strlen(t)-1; i >= 0 && t[i] == 32; i--)
	t[i] = 0;
      print_line(level+1, "Joliet extension, volume name \"%s\"", t);
      break;

    default:
      print_line(level+1, "Descriptor type %d at sector %d", type, sector);
      break;
    }
  }
}

/*
 * El Torito boot catalog
 */

static char *media_types[16] = {
  "non-emulated",
  "1.2M floppy",
  "1.44M floppy",
  "2.88M floppy",
  "hard disk",
  "reserved type 5", "reserved type 6", "reserved type 7",
  "reserved type 8", "reserved type 9", "reserved type 10",
  "reserved type 11", "reserved type 12", "reserved type 13",
  "reserved type 14", "reserved type 15"
};

static void dump_boot_catalog(int8 basepos, int8 pos, int level)
{
  unsigned char *buf;
  int bootable, media, start, preload, more;
  char s[256];

  /* get boot catalog */
  if (get_buffer(pos, 2048, (void **)&buf) < 2048)
    return;

  /* check validation entry (must be first) */
  if (buf[0] != 0x01 || buf[30] != 0x55 || buf[31] != 0xAA) {
    print_line(level, "Validation entry missing");
    return;
  }
  /* TODO: checksum */

  /* initial/default entry */
  if (buf[32] == 0x88 || buf[32] == 0x00) {
    bootable = (buf[32] == 0x88) ? 1 : 0;
    media = buf[33] & 15;
    start = get_le_long(buf+40);
    preload = get_le_short(buf+38);
  } else {
    print_line(level, "Initial/Default entry missing");
    return;
  }

  /* more stuff? */
  more = (buf[64] == 0x90 || buf[64] == 0x91) ? 1 : 0;

  /* print and analyze further */
  format_size(s, preload, 512);
  print_line(level, "%s %s image, starts at %d, preloads %s",
	     bootable ? "Bootable" : "Non-bootable",
	     media_types[media], start, s);
  if (start > 0)
    detect_from(basepos + (int8)start * 2048, level + 1);

  if (more)
    print_line(level, "Vendor-specific sections follow");
}

/* EOF */
