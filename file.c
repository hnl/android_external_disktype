/*
 * file.c
 * Data source for files and block devices.
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
 * types
 */

typedef struct file_source {
  SOURCE c;
  int fd;
} FILE_SOURCE;

/*
 * helper functions
 */

static u8 read_file(SOURCE *s, u8 pos, u8 len, void *buf);
static void close_file(SOURCE *s);

/*
 * initialize the file source
 */

SOURCE *init_file_source(int fd)
{
  FILE_SOURCE *fs;

  fs = (FILE_SOURCE *)malloc(sizeof(FILE_SOURCE));
  if (fs == NULL)
    bailout("Out of memory");
  memset(fs, 0, sizeof(FILE_SOURCE));

  fs->c.sequential = 0;
  fs->c.read = read_file;
  fs->c.close = close_file;
  fs->fd = fd;

  fs->c.size = lseek(fd, 0, SEEK_END);

  return (SOURCE *)fs;
}

/*
 * raw read
 */

static u8 read_file(SOURCE *s, u8 pos, u8 len, void *buf)
{
  off_t result_seek;
  ssize_t result_read;
  char *p;
  u8 got;
  int fd = ((FILE_SOURCE *)s)->fd;

  /* seek to the requested position */
  result_seek = lseek(fd, pos, SEEK_SET);
  if (result_seek != pos) {
    errore("Seek to %llu returned %lld", pos, result_seek);
    return 0;
  }

  /* read from there */
  p = (char *)buf;
  got = 0;
  while (len > 0) {
    result_read = read(fd, p, len);
    if (result_read < 0) {
      if (errno == EINTR || errno == EAGAIN)
	continue;
      errore("On file read");
      break;
    } else if (result_read == 0) {
      break;
    } else {
      len -= result_read;
      got += result_read;
      p += result_read;
    }
  }

  return got;
}

/*
 * dispose of everything
 */

static void close_file(SOURCE *s)
{
  int fd = ((FILE_SOURCE *)s)->fd;

  if (fd >= 0)
    close(fd);
}

/* EOF */
