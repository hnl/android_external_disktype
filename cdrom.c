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

static void dump_boot_catalog(SECTION *section, u8 pos, int level);

/*
 * ISO9660 file system
 */

void detect_iso(SECTION *section, int level)
{
  char s[256], t[256];
  int i, sector, type;
  u4 blocksize;
  u8 blocks, bcpos;
  unsigned char *buf;

  /* get the volume descriptor */
  if (get_buffer(section, 32768, 2048, (void **)&buf) < 2048)
    return;

  /* check signature */
  if (memcmp(buf, "\001CD001", 6) != 0)
    return;

  print_line(level, "ISO9660 file system");

  /* read Volume ID and other info */
  get_padded_string(buf + 40, 32, ' ', s);
  print_line(level+1, "Volume name \"%s\"", s);

  get_padded_string(buf + 318, 128, ' ', s);
  if (s[0])
    print_line(level+1, "Publisher   \"%s\"", s);

  get_padded_string(buf + 446, 128, ' ', s);
  if (s[0])
    print_line(level+1, "Preparer    \"%s\"", s);

  get_padded_string(buf + 574, 128, ' ', s);
  if (s[0])
    print_line(level+1, "Application \"%s\"", s);

  /* some other interesting facts */
  blocks = get_le_long(buf + 80);
  blocksize = get_le_short(buf + 128);
  format_size(s, blocks, blocksize);
  print_line(level+1, "Data size %s (%llu blocks of %lu bytes)",
	     s, blocks, blocksize);

  for (sector = 17; ; sector++) {
    /* get next descriptor */
    if (get_buffer(section, sector * 2048, 2048, (void **)&buf) < 2048)
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
      print_line(level+1, "El Torito boot record, catalog at %llu", bcpos);

      /* boot catalog */
      dump_boot_catalog(section, bcpos * 2048, level + 2);

      break;

    case 1:  /* Primary Volume Descriptor */
      print_line(level+1, "Additional Primary Volume Descriptor");
      break;

    case 2:  /* Supplementary Volume Descriptor, Joliet */
      /* read Volume ID */
      memcpy(s, buf + 40, 32);
      s[32] = 0;
      s[33] = 0;
      format_unicode(s, t);
      for (i = strlen(t)-1; i >= 0 && t[i] == ' '; i--)
	t[i] = 0;
      print_line(level+1, "Joliet extension, volume name \"%s\"", t);
      break;

    case 3:  /* Volume Partition Descriptor */
      print_line(level+1, "Volume Partition Descriptor");
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

static void dump_boot_catalog(SECTION *section, u8 pos, int level)
{
  unsigned char *buf;
  int bootable, media, more;
  u4 start, preload;
  char s[256];

  /* get boot catalog */
  if (get_buffer(section, pos, 2048, (void **)&buf) < 2048)
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
  print_line(level, "%s %s image, starts at %lu, preloads %s",
	     bootable ? "Bootable" : "Non-bootable",
	     media_types[media], start, s);
  if (start > 0) {
    analyze_recursive(section, level + 1,
		      (u8)start * 2048, 0, 0);
    /* TODO: calculate size in some way */
  }

  if (more)
    print_line(level, "Vendor-specific sections follow");
}

/* EOF */
