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
void detect_apple_partmap(int8 pos, int level);
void detect_apple_volume(int8 pos, int level);

/* in dos.c */
void detect_dos_partmap(int8 pos, int level);
void detect_fat(int8 pos, int level);

/* in cdrom.c */
void detect_iso(int8 pos, int level);

/* in unix.c */
void detect_ext23(int8 pos, int level);
void detect_unix_misc(int8 pos, int level);

/*
 * general detectors
 */

DETECTOR detectors[] = {
  detect_apple_partmap,
  detect_apple_volume,
  detect_dos_partmap,
  detect_fat,
  detect_iso,
  detect_ext23,
  detect_unix_misc,
 NULL };

/*
 * main routine detection
 */

void detect(void)
{
  detect_from(0, 0);
}

/*
 * main recursive detection function
 */

void detect_from(int8 pos, int level)
{
  int i, fill;
  unsigned char *buf;

  fill = get_buffer(pos, 4096, (void **)&buf);
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
    (*detectors[i])(pos, level);
}

/* EOF */
