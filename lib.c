/*
 * lib.c
 * Global utility functions.
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
#include <stdarg.h>

/*
 * output functions
 */

#define LEVELS (8)

static const char *insets[LEVELS] = {
  "",
  "  ",
  "    ",
  "      ",
  "        ",
  "          ",
  "            ",
  "              ",
};
static char line_akku[4096];

void print_line(int level, const char *fmt, ...)
{
  va_list par;

  va_start(par, fmt);
  vsnprintf(line_akku, 4096, fmt, par);
  va_end(par);

  if (level >= LEVELS)
    bailout("Recursion loop caught");
  printf("%s%s\n", insets[level], line_akku);
}

void start_line(const char *fmt, ...)
{
  va_list par;

  va_start(par, fmt);
  vsnprintf(line_akku, 4096, fmt, par);
  va_end(par);
}

void continue_line(const char *fmt, ...)
{
  va_list par;
  int len = strlen(line_akku);

  va_start(par, fmt);
  vsnprintf(line_akku + len, 4096 - len, fmt, par);
  va_end(par);
}

void finish_line(int level)
{
  if (level >= LEVELS)
    bailout("Recursion loop caught");
  printf("%s%s\n", insets[level], line_akku);
}

/*
 * formatting functions
 */

void format_size(char *buf, u8 size, u4 mult)
{
  int card;

  size *= mult;

  if (size < 1000) {
    card = (int)size;
    sprintf(buf, "%dB", card);
  } else if (size < (1000<<10) && (size & 0x3ff) == 0) {
    /* whole kilobytes look funny else... */
    card = (int)(size >> 10);
    sprintf(buf, "%dK", card);
  } else if (size < (10<<10)) {
    card = (int)((size * 100 + 512) / 1024);
    sprintf(buf, "%d.%02dK", card / 100, card % 100);
  } else if (size < (100<<10)) {
    card = (int)((size * 10 + 512) / 1024);
    sprintf(buf, "%d.%01dK", card / 10, card % 10);
  } else if (size < (1000<<10)) {
    card = (int)((size + 512) / 1024);
    sprintf(buf, "%dK", card);
  } else {
    size >>= 10;
    if (size < (10<<10)) {
      card = (int)((size * 100 + 512) / 1024);
      sprintf(buf, "%d.%02dM", card / 100, card % 100);
    } else if (size < (100<<10)) {
      card = (int)((size * 10 + 512) / 1024);
      sprintf(buf, "%d.%01dM", card / 10, card % 10);
    } else if (size < (1000<<10)) {
      card = (int)((size + 512) / 1024);
      sprintf(buf, "%dM", card);
    } else {
      size >>= 10;
      if (size < (10<<10)) {
	card = (int)((size * 100 + 512) / 1024);
	sprintf(buf, "%d.%02dG", card / 100, card % 100);
      } else if (size < (100<<10)) {
	card = (int)((size * 10 + 512) / 1024);
	sprintf(buf, "%d.%01dG", card / 10, card % 10);
      } else {
	card = (int)((size + 512) / 1024);
	sprintf(buf, "%dG", card);
      }
    }
  }
}

void format_ascii(void *from, char *to)
{
  u1 *p = (u1 *)from;
  u1 *q = (u1 *)to;
  int c;

  while ((c = *p++)) {
    if (c >= 127 || c < 32) {
      *q++ = '<';
      *q++ = "0123456789ABCDEF"[c >> 4];
      *q++ = "0123456789ABCDEF"[c & 15];
      *q++ = '>';
    } else {
      *q++ = c;
    }
  }
  *q = 0;
}

void format_unicode(void *from, char *to)
{
  u2 *p = (u2 *)from;
  u1 *q = (u1 *)to;
  u2 c;

  for (;;) {
    c = get_be_short(p);
    if (c == 0)
      break;
    p++;  /* advance 2 bytes */

    if (c >= 127 || c < 32) {
      *q++ = '<';
      *q++ = "0123456789ABCDEF"[c >> 12];
      *q++ = "0123456789ABCDEF"[(c >> 8) & 15];
      *q++ = "0123456789ABCDEF"[(c >> 4) & 15];
      *q++ = "0123456789ABCDEF"[c & 15];
      *q++ = '>';
    } else {
      *q++ = (u1)c;
    }
  }
  *q = 0;
}

void format_uuid(void *uuid, char *to)
{
  u1 *from = uuid;
  int i, c, variant, version;

  if (memcmp(uuid, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16) == 0) {
    strcpy(to, "nil");
    return;
  }

  variant = from[8] >> 5;
  version = from[6] >> 4;

  for (i = 0; i < 16; i++) {
    c = *from++;
    *to++ = "0123456789ABCDEF"[c >> 4];
    *to++ = "0123456789ABCDEF"[c & 15];
    if (i == 3 || i == 5 || i == 7 || i == 9)
      *to++ = '-';
  }

  if ((variant & 4) == 0) {         /* 0 x x */
    strcpy(to, " (NCS)");
  } else if ((variant & 2) == 0) {  /* 1 0 x */
    sprintf(to, " (DCE, v%1.1d)", version);
  } else if ((variant & 1) == 0) {  /* 1 1 0 */
    strcpy(to, " (MS GUID)");
  } else {                          /* 1 1 1 */
    strcpy(to, " (Reserved)");
  }
}

/*
 * endian-aware data access
 */

u2 get_be_short(void *from)
{
  u1 *p = from;
  return ((u2)(p[0]) << 8) +
    (u2)p[1];
}

u4 get_be_long(void *from)
{
  u1 *p = from;
  return ((u4)(p[0]) << 24) +
    ((u4)(p[1]) << 16) +
    ((u4)(p[2]) << 8) +
    (u4)p[3];
}

u8 get_be_quad(void *from)
{
  u1 *p = from;
  return ((u8)(p[0]) << 56) +
    ((u8)(p[1]) << 48) +
    ((u8)(p[2]) << 40) +
    ((u8)(p[3]) << 32) +
    ((u8)(p[4]) << 24) +
    ((u8)(p[5]) << 16) +
    ((u8)(p[6]) << 8) +
    (u8)p[7];
}

u2 get_le_short(void *from)
{
  u1 *p = from;
  return ((u2)(p[1]) << 8) +
    (u2)p[0];
}

u4 get_le_long(void *from)
{
  u1 *p = from;
  return ((u4)(p[3]) << 24) +
    ((u4)(p[2]) << 16) +
    ((u4)(p[1]) << 8) +
    (u4)p[0];
}

u8 get_le_quad(void *from)
{
  u1 *p = from;
  return ((u8)(p[7]) << 56) +
    ((u8)(p[6]) << 48) +
    ((u8)(p[5]) << 40) +
    ((u8)(p[4]) << 32) +
    ((u8)(p[3]) << 24) +
    ((u8)(p[2]) << 16) +
    ((u8)(p[1]) << 8) +
    (u8)p[0];
}

u2 get_ve_short(int endianness, void *from)
{
  if (endianness)
    return get_le_short(from);
  else
    return get_be_short(from);
}

u4 get_ve_long(int endianness, void *from)
{
  if (endianness)
    return get_le_long(from);
  else
    return get_be_long(from);
}

u8 get_ve_quad(int endianness, void *from)
{
  if (endianness)
    return get_le_quad(from);
  else
    return get_be_quad(from);
}

const char * get_ve_name(int endianness)
{
  if (endianness)
    return "little-endian";
  else
    return "big-endian";
}

/*
 * more data access
 */

void get_pstring(void *from, char *to)
{
  int len = *(unsigned char *)from;
  memcpy(to, (char *)from + 1, len);
  to[len] = 0;
}

int find_memory(void *haystack, int haystack_len,
                void *needle, int needle_len)
{
  int searchlen = haystack_len - needle_len + 1;
  int pos = 0;
  void *p;

  while (pos < searchlen) {
    p = memchr((char *)haystack + pos, *(unsigned char *)needle,
	       searchlen - pos);
    if (p == NULL)
      return -1;
    pos = (char *)p - (char *)haystack;
    if (memcmp(p, needle, needle_len) == 0)
      return pos;
    pos++;
  }

  return -1;
}

/*
 * error functions
 */

void error(const char *msg, ...)
{
  va_list par;
  char buf[4096];

  va_start(par, msg);
  vsnprintf(buf, 4096, msg, par);
  va_end(par);

  fprintf(stderr, PROGNAME ": %s\n", buf);
}

void errore(const char *msg, ...)
{
  va_list par;
  char buf[4096];

  va_start(par, msg);
  vsnprintf(buf, 4096, msg, par);
  va_end(par);

  fprintf(stderr, PROGNAME ": %s: %s\n", buf, strerror(errno));
}

void bailout(const char *msg, ...)
{
  va_list par;
  char buf[4096];

  va_start(par, msg);
  vsnprintf(buf, 4096, msg, par);
  va_end(par);

  fprintf(stderr, PROGNAME ": %s\n", buf);
  exit(1);
}

void bailoute(const char *msg, ...)
{
  va_list par;
  char buf[4096];

  va_start(par, msg);
  vsnprintf(buf, 4096, msg, par);
  va_end(par);

  fprintf(stderr, PROGNAME ": %s: %s\n", buf, strerror(errno));
  exit(1);
}

/* EOF */
