/*
 * detect.c
 * Detection dispatching
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
 * external detection functions
 */

/* in amiga.c */
void detect_amiga_partmap(SECTION *section, int level);
void detect_amiga_fs(SECTION *section, int level);

/* in apple.c */
void detect_apple_partmap(SECTION *section, int level);
void detect_apple_volume(SECTION *section, int level);

/* in atari.c */
void detect_atari_partmap(SECTION *section, int level);

/* in dos.c */
void detect_dos_partmap(SECTION *section, int level);
void detect_fat(SECTION *section, int level);
void detect_ntfs(SECTION *section, int level);
void detect_hpfs(SECTION *section, int level);

/* in cdrom.c */
void detect_iso(SECTION *section, int level);

/* in unix.c */
void detect_ext23(SECTION *section, int level);
void detect_reiser(SECTION *section, int level);
void detect_jfs(SECTION *section, int level);
void detect_xfs(SECTION *section, int level);
void detect_linux_raid(SECTION *section, int level);
void detect_linux_lvm(SECTION *section, int level);
void detect_linux_swap(SECTION *section, int level);
void detect_ufs(SECTION *section, int level);
void detect_unix_misc(SECTION *section, int level);

/* in compressed.c */
void detect_compressed(SECTION *section, int level);

/* in cdimage.c */
void detect_cdimage(SECTION *section, int level);

/* in archives.c */
void detect_archive(SECTION *section, int level);

/*
 * general detectors
 */

DETECTOR detectors[] = {
  detect_amiga_partmap,
  detect_amiga_fs,
  detect_apple_partmap,
  detect_apple_volume,
  detect_atari_partmap,
  detect_dos_partmap,
  detect_fat,
  detect_ntfs,
  detect_hpfs,
  detect_iso,
  detect_ext23,
  detect_reiser,
  detect_jfs,
  detect_xfs,
  detect_linux_raid,
  detect_linux_lvm,
  detect_linux_swap,
  detect_ufs,
  detect_unix_misc,
  detect_archive,
  detect_compressed,
  detect_cdimage,
 NULL };

/*
 * main recursive detection function
 */

void detect(SECTION *section, int level)
{
  int i;

  /* run the modularized detectors */
  for (i = 0; detectors[i]; i++)
    (*detectors[i])(section, level);
}

/* EOF */
