/*
 * cdimage.c
 * Layered data source for CD images in raw mode.
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

typedef struct cdimage_source {
  SOURCE c;
  u8 off;
} CDIMAGE_SOURCE;

/*
 * helper functions
 */

static SOURCE *init_cdimage_source(SOURCE *foundation, u8 offset);
static u8 read_cdimage(SOURCE *s, u8 pos, u8 len, void *buf);

/*
 * cd image detection
 */

static unsigned char syncbytes[12] =
  { 0, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 0 };

void detect_cdimage(SECTION *section, int level)
{
  int mode, off;
  unsigned char *buf;
  SOURCE *s;
  SECTION rs;

  if (get_buffer(section, 0, 2352, (void **)&buf) < 2352)
    return;

  /* check sync bytes as signature */
  if (memcmp(buf, syncbytes, 12) != 0)
    return;

  /* get mode of the track -- this determines sector layout */
  mode = buf[15];
  if (mode == 1) {
    /* standard data track */
    print_line(level, "Raw CD image, Mode 1");
    off = 16;
  } else if (mode == 2) {
    /* free-form track, assume XA form 1 */
    print_line(level, "Raw CD image, Mode 2, assuming Form 1");
    off = 24;
  } else
    return;

  /* create wrapped source */
  s = init_cdimage_source(section->source, section->pos + off);

  /* analyze it */
  rs.source = s;
  rs.pos = 0;
  rs.size = s->size;
  detect(&rs, level);

  /* destroy wrapped source */
  close_source(s);
}

/*
 * initialize the cd image source
 */

static SOURCE *init_cdimage_source(SOURCE *foundation, u8 offset)
{
  CDIMAGE_SOURCE *src;

  src = (CDIMAGE_SOURCE *)malloc(sizeof(CDIMAGE_SOURCE));
  if (src == NULL)
    bailout("Out of memory");
  memset(src, 0, sizeof(CDIMAGE_SOURCE));

  src->c.sequential = 0;
  src->c.foundation = foundation;
  src->c.read = read_cdimage;
  src->c.close = NULL;
  src->off = offset;

  src->c.size = ((foundation->size - offset + 304) / 2352) * 2048;

  return (SOURCE *)src;
}

/*
 * raw read
 */

static u8 read_cdimage(SOURCE *s, u8 pos, u8 len, void *buf)
{
  SOURCE *fs = s->foundation;
  int skip, askfor;
  u8 filepos, fill, got;
  char *p, *filebuf;
  u8 off = ((CDIMAGE_SOURCE *)s)->off;

  p = (char *)buf;
  got = 0;

  /* translate position into image sectors */
  filepos = (pos / 2048) * 2352 + off;
  skip = (int)(pos & 2047);

  /* special case if we're not aligned (unlikely, but still...) */
  if (len > 0 && skip > 0) {
    /* read from lower layer */
    askfor = 2048;
    if (len < 2048)
      askfor = len;
    fill = get_buffer_real(fs, filepos, askfor, (void **)&filebuf);

    /* copy to our buffer */
    if (fill <= skip)
      return got;
    memcpy(p, filebuf + skip, fill - skip);

    /* adjust state for next iteration */
    len -= (fill - skip);
    got += (fill - skip);
    p += (fill - skip);
    filepos += 2352;

    if (fill < askfor)
      return got;
  }

  while (len > 0) {
    /* read from lower layer */
    askfor = 2048;
    if (len < 2048)
      askfor = len;
    fill = get_buffer_real(fs, filepos, askfor, (void **)&filebuf);

    /* copy to our buffer */
    if (fill <= 0)
      break;
    memcpy(p, filebuf, fill);

    /* adjust state for next iteration */
    len -= fill;
    got += fill;
    p += fill;
    filepos += 2352;

    if (fill < askfor)
      break;
  }

  return got;
}

/* EOF */
