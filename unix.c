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

void detect_ext23(SECTION *section, int level)
{
  unsigned char *buf;
  char s[256];
  u4 blocksize;
  u8 blockcount;

  if (get_buffer(section, 1024, 1024, (void **)&buf) < 1024)
    return;

  if (get_le_short(buf + 56) == 0xEF53) {
    if (get_le_long(buf + 92) & 0x0004)  /* HAS_JOURNAL flag */
      print_line(level, "Ext3 file system");
    else
      print_line(level, "Ext2 file system");

    memcpy(s, buf + 120, 16);
    s[16] = 0;
    if (s[0])
      print_line(level + 1, "Volume name \"%s\"", s);

    format_uuid(buf + 104, s);
    print_line(level + 1, "UUID %s", s);

    memcpy(s, buf + 136, 64);
    s[64] = 0;
    if (s[0])
      print_line(level + 1, "Last mounted at \"%s\"", s);

    blocksize = 1024 << get_le_long(buf + 24);
    blockcount = get_le_long(buf + 4);
    format_size(s, blockcount, blocksize);
    print_line(level + 1, "Volume size %s (%llu blocks of %lu bytes)",
	       s, blockcount, blocksize);

    /* 76 4 s_rev_level */
    /* 62 2 s_minor_rev_level */
    /* 72 4 s_creator_os */
    /* 92 3x4 s_feature_compat, s_feature_incompat, s_feature_ro_compat */
  }
}

/*
 * ReiserFS file system
 */

void detect_reiser(SECTION *section, int level)
{
  unsigned char *buf;
  int i, at, newformat;
  int offsets[3] = { 8, 64, -1 };
  char s[256];
  u8 blockcount;
  u4 blocksize;

  for (i = 0; offsets[i] >= 0; i++) {
    at = offsets[i];
    if (get_buffer(section, at * 1024, 1024, (void **)&buf) < 1024)
      continue;

    /* check signature */
    if (memcmp(buf + 52, "ReIsErFs", 8) == 0) {
      print_line(level, "ReiserFS file system (old 3.5 format, standard journal, starts at %dK)", at);
      newformat = 0;
    } else if (memcmp(buf + 52, "ReIsEr2Fs", 9) == 0) {
      print_line(level, "ReiserFS file system (new 3.6 format, standard journal, starts at %dK)", at);
      newformat = 1;
    } else if (memcmp(buf + 52, "ReIsEr3Fs", 9) == 0) {
      newformat = get_le_short(buf + 72);
      if (newformat == 0) {
	print_line(level, "ReiserFS file system (old 3.5 format, non-standard journal, starts at %dK)", at);
      } else if (newformat == 2) {
	print_line(level, "ReiserFS file system (new 3.6 format, non-standard journal, starts at %dK)", at);
	newformat = 1;
      } else {
	print_line(level, "ReiserFS file system (v3 magic, but unknown version %d, starts at %dK)", newformat, at);
	continue;
      }
    } else
      continue;

    /* get data */
    blockcount = get_le_long(buf);
    blocksize = get_le_short(buf + 44);
    /* for new format only:
       hashtype = get_le_long(buf + 64);
    */

    /* get label */
    memcpy(s, buf + 100, 16);
    s[16] = 0;
    if (s[0])
      print_line(level + 1, "Volume name \"%s\"", s);

    format_uuid(buf + 84, s);
    print_line(level + 1, "UUID %s", s);

    /* print size */
    format_size(s, blockcount, blocksize);
    print_line(level + 1, "Volume size %s (%llu blocks of %lu bytes)",
	       s, blockcount, blocksize);

    /* TODO: print hash code */
  }
}

/*
 * JFS for Linux
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
 * XFS for Linux
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
 * Linux RAID persistent superblock
 */

static char *levels[] = {
  "Multipath",
  "\'HSM\'",
  "\'translucent\'",
  "Linear",
  "RAID0",
  "RAID1",
  NULL, NULL,
  "RAID4(?)",
  "RAID5"
};

void detect_linux_raid(SECTION *section, int level)
{
  unsigned char *buf;
  u8 pos;
  int rlevel, nr_disks, raid_disks, spare;
  u1 uuid[16];
  char s[256];

  /* don't do this if:
   *  - the size is unknown (0)
   *  - the size is too small for the calculation
   *  - it is inefficient to read from the end of the source
   */
  if (section->size < 65536 || section->source->sequential)
    return;

  /* get RAID superblock from the end of the device */
  pos = (section->size & ~65535) - 65536;
  if (get_buffer(section, pos, 4096, (void **)&buf) < 4096)
    return;

  /* signature */
  if (get_le_long(buf) != 0xa92b4efc)
    return;

  print_line(level, "Linux RAID disk, version %lu.%lu.%lu",
	     get_le_long(buf + 4), get_le_long(buf + 8),
	     get_le_long(buf + 12));

  /* get some data */
  rlevel = (int)(long)get_le_long(buf + 28);   /* is signed, actually */
  nr_disks = get_le_long(buf + 36);
  raid_disks = get_le_long(buf + 40);
  spare = nr_disks - raid_disks;

  /* find the name for the personality in the table */
  if (rlevel < -4 || rlevel > 5 || levels[rlevel+4] == NULL) {
    print_line(level + 1, "Unknown RAID level %d using %d regular %d spare disks",
	       rlevel, raid_disks, spare);
  } else {
    print_line(level + 1, "%s set using %d regular %d spare disks",
	       levels[rlevel+4], raid_disks, spare);
  }

  /* get the UUID */
  memcpy(uuid, buf + 5*4, 4);
  memcpy(uuid + 4, buf + 13*4, 3*4);
  format_uuid(uuid, s);
  print_line(level + 1, "RAID set UUID %s", s);
}

/*
 * Linux LVM
 */

void detect_linux_lvm(SECTION *section, int level)
{
  unsigned char *buf;
  char s[256], t[256];
  u8 pe_size, pe_count, pe_start;
  SECTION rs;

  if (get_buffer(section, 0, 1024, (void **)&buf) < 1024)
    return;

  /* signature */
  if (buf[0] != 'H' || buf[1] != 'M')
    return;
  /* helpful sanity check... */
  if (get_le_long(buf + 36) == 0 || get_le_long(buf + 40) == 0)
    return;

  print_line(level, "Linux LVM volume, version %d",
	     (int)get_le_short(buf + 2));

  /* volume group name */
  memcpy(s, buf + 172, 128);
  s[128] = 0;
  print_line(level + 1, "Volume group name \"%s\"", s);

  /* volume size */
  pe_size = get_le_long(buf + 452);
  pe_count = get_le_long(buf + 456);
  format_size(s, pe_size * pe_count, 512);
  format_size(t, pe_size, 512);
  print_line(level + 1, "Useable size %s (%llu PEs of %s)",
	     s, pe_count, t);

  /* first PE starts after the declared length of the PE tables */
  pe_start = get_le_long(buf + 36) + get_le_long(buf + 40);

  /* try to detect from first PE */
  if (pe_start > 0) {
    rs.source = section->source;
    rs.pos = section->pos + pe_start;
    rs.size = 0;
    detect(&rs, level + 1);
  }
}

/*
 * Linux swap area
 */

void detect_linux_swap(SECTION *section, int level)
{
  int i, en, pagesize;
  unsigned char *buf;
  u4 version, pages;
  char s[256];
  int pagesizes[] = { 4096, 8192, 0 };

  for (i = 0; pagesizes[i]; i++) {
    pagesize = pagesizes[i];

    if (get_buffer(section, pagesize - 512, 512, (void **)&buf) != 512)
      break;  /* assumes page sizes increase through the loop */

    if (memcmp((char *)buf + 512 - 10, "SWAP-SPACE", 10) == 0) {
      print_line(level, "Linux swap, version 1, %dK pages",
		 pagesize >> 10);
    }
    if (memcmp((char *)buf + 512 - 10, "SWAPSPACE2", 10) == 0) {
      if (get_buffer(section, 1024, 512, (void **)&buf) != 512)
	break;  /* really shouldn't happen */

      for (en = 0; en < 2; en++) {
	version = get_ve_long(en, buf);
	if (version >= 1 && version < 10)
	  break;
      }
      if (en < 2) {
	print_line(level, "Linux swap, version 2, subversion %d, %dK pages, %s",
		   (int)version, pagesize >> 10, get_ve_name(en));
	if (version == 1) {
	  pages = get_ve_long(en, buf + 4) - 1;
	  format_size(s, pages, pagesize);
	  print_line(level + 1, "Swap size %s (%lu pages)",
		     s, pages);
	}
      } else {
	print_line(level, "Linux swap, version 2, illegal subversion, %dK pages",
		   pagesize >> 10);
      }
    }
  }
}

/*
 * UFS file system from various BSD strains
 */

void detect_ufs(SECTION *section, int level)
{
  unsigned char *buf;
  int i, at, en, offsets[3] = { 16, -1 };
  u4 magic;

  /* NextStep/OpenStep apparently can move the superblock further into
     the device. Linux uses steps of 8 blocks (of the applicable block
     size) to scan for it. We only scan at the canonical location for now. */
  /* Possible block sizes: 512 (old formats), 1024 (most), 2048 (CD media) */

  for (i = 0; offsets[i] >= 0; i++) {
    at = offsets[i];
    if (get_buffer(section, at * 512, 1536, (void **)&buf) < 1536)
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
      }
    }
  }
}

/*
 * various signatures
 */

void detect_unix_misc(SECTION *section, int level)
{
  int magic, fill, off, en;
  unsigned char *buf;
  char s[256];
  u8 size, blocks, blocksize;

  fill = get_buffer(section, 0, 2048, (void **)&buf);
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
    int version = 0, namesize = 14;
    u8 blocks;

    magic = get_le_short(buf + 1024 + 16);
    if (magic == 0x137F)
      version = 1;
    if (magic == 0x138F) {
      version = 1;
      namesize = 30;
    }
    if (magic == 0x2468)
      version = 2;
    if (magic == 0x2478) {
      version = 2;
      namesize = 30;
    }
    if (version) {
      print_line(level, "Minix file system (v%d, %d chars)",
		 version, namesize);
      if (version == 1)
	blocks = get_le_short(buf + 1024 + 2);
      else
	blocks = get_le_long(buf + 1024 + 20);
      blocks = (blocks - get_le_short(buf + 1024 + 8))
	<< get_le_short(buf + 1024 + 10);
      format_size(s, blocks, 1024);
      print_line(level + 1, "Volume size %s (%llu blocks of 1K)",
		 s, blocks);
    }
  }

  /* Linux romfs */
  if (memcmp(buf, "-rom1fs-", 8) == 0) {
    u8 size = get_be_long(buf + 8);
    print_line(level, "Linux romfs");
    print_line(level+1, "Volume name \"%.300s\"", (char *)(buf + 16));
    format_size(s, size, 1);
    print_line(level+1, "Volume size %s (%llu bytes)", s, size);
  }

  /* Linux cramfs */
  for (off = 0; off <= 512; off += 512) {
    if (fill < off + 512)
      break;
    for (en = 0; en < 2; en++) {
      if (get_ve_long(en, buf + off) == 0x28cd3d45) {
	print_line(level, "Linux cramfs, starts sector %d, %s",
		   off >> 9, get_ve_name(en));

	memcpy(s, buf + off + 48, 16);
	s[16] = 0;
	print_line(level + 1, "Volume name \"%s\"", s);

	size = get_ve_long(en, buf + off + 4);
	blocks = get_ve_long(en, buf + off + 40);
	format_size(s, size, 1);
	print_line(level + 1, "Compressed size %s (%llu bytes)",
		   s, size);
	format_size(s, blocks, 4096);
	print_line(level + 1, "Data size %s (%llu blocks of -assumed- 4K)",
		   s, blocks);
      }
    }
  }

  /* Linux squashfs */
  for (en = 0; en < 2; en++) {
    if (get_ve_long(en, buf) == 0x73717368) {
      int major, minor;

      major = get_ve_short(en, buf + 28);
      minor = get_ve_short(en, buf + 30);
      print_line(level, "Linux squashfs, version %d.%d, %s",
		 major, minor, get_ve_name(en));

      size = get_ve_long(en, buf + 8);
      blocksize = get_ve_short(en, buf + 32);

      format_size(s, size, 1);
      print_line(level + 1, "Compressed size %s (%llu bytes)",
		 s, size);
      format_size(s, blocksize, 1);
      print_line(level + 1, "Block size %s", s);
    }
  }

  /* Linux kernel loader */
  if (fill >= 1024 && memcmp((char *)buf + 512 + 2, "HdrS", 4) == 0) {
    print_line(level, "Linux kernel build-in loader");
  }
}

/* EOF */
