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

/* in apple.c */
void detect_apple_partmap(SECTION *section, int level);
void detect_apple_volume(SECTION *section, int level);

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
void detect_linux_raid(SECTION *section, int level);
void detect_linux_lvm(SECTION *section, int level);
void detect_unix_misc(SECTION *section, int level);

/*
 * general detectors
 */

DETECTOR detectors[] = {
  detect_apple_partmap,
  detect_apple_volume,
  detect_dos_partmap,
  detect_fat,
  detect_ntfs,
  detect_hpfs,
  detect_iso,
  detect_ext23,
  detect_reiser,
  detect_linux_raid,
  detect_linux_lvm,
  detect_unix_misc,
 NULL };

/*
 * main recursive detection function
 */

void detect(SECTION *section, int level)
{
  int i, fill;
  unsigned char *buf;

  fill = get_buffer(section, 0, 4096, (void **)&buf);
  if (fill < 512) {
    print_line(level, "Not enough data for analysis");
    return;
  }

  /* gzip */
  for (i = 0; i < 32 && i < (fill >> 9); i++) {
    int off = i << 9;
    if (buf[off] == 037 && (buf[off+1] == 0213 || buf[off+1] == 0236)) {
      print_line(level, "gzip magic at sector %d", i);
      break;
    }
  }

  /* now run the modularized detectors */
  for (i = 0; detectors[i]; i++)
    (*detectors[i])(section, level);

  /* check size for possible raw CD image */
  if (section->size && (section->size % 2352) == 0) {
    int headlens[] = { 16, 24, -1 };
    SOURCE *s;
    SECTION rs;

    print_line(level, "Size divisible by 2352, interpreting as raw CD image");

    for (i = 0; headlens[i] >= 0; i++) {
      /* create wrapped source */
      s = init_cdimage_source(section->source, section->pos + headlens[i]);

      /* analyze it */
      rs.source = s;
      rs.pos = 0;
      rs.size = s->size;
      detect(&rs, level);

      /* destroy wrapped source */
      close_source(s);
    }
  }
}

/* EOF */
