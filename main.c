/*
 * main.c
 * Main entry point and global utility functions.
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

#ifdef USE_MACOS_TYPE
#include <CoreServices/CoreServices.h>
#endif

#define PROGNAME "disktype"

/*
 * local functions
 */

static void analyze_file(const char *filename);

#ifdef USE_MACOS_TYPE
static void show_macos_type(const char *filename);
#endif

/*
 * entry point
 */

int main(int argc, char *argv[])
{
  int i;

  /* argument check */
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <device/file>...\n", PROGNAME);
    return 1;
  }

  /* loop over filenames */
  print_line(0, "");
  for (i = 1; i < argc; i++) {
    analyze_file(argv[i]);
  }

  return 0;
}

/*
 * Analyze one file
 */

static void analyze_file(const char *filename)
{
  int fd;
  struct stat sb;
  int8 filesize;
  char *reason;
  SOURCE *s;
  SECTION section;

  print_line(0, "--- %s", filename);

  /* stat check */
  if (stat(filename, &sb) < 0) {
    errore("%.300s", filename);
    return;
  }

  filesize = -1;
  reason = NULL;
  if (S_ISREG(sb.st_mode)) {
    char buf[16];
    filesize = sb.st_size;
    format_size(buf, filesize, 1);
    print_line(0, "Regular file, size %lld bytes (%s)", filesize, buf);
  } else if (S_ISBLK(sb.st_mode)) {
    print_line(0, "Block device");
  } else if (S_ISDIR(sb.st_mode))
    reason = "Is a directory";
  else if (S_ISCHR(sb.st_mode))
    reason = "Is a character device";
  else if (S_ISFIFO(sb.st_mode))
    reason = "Is a FIFO";
  else if (S_ISSOCK(sb.st_mode))
    reason = "Is a socket";
  else
    reason = "Is an unknown kind of special file";
  if (reason != NULL) {
    error("%.300s: %s", filename, reason);
    return;
  }

  /* Mac OS type & creator code (if running on Mac OS) */
#ifdef USE_MACOS_TYPE
  show_macos_type(filename);
#endif

  /* empty files need no further analysis */
  if (filesize == 0)
    return;

  /* open for reading */
  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    errore("%.300s", filename);
    return;
  }

  /* create a source */
  s = init_file_source(fd);

  /* now analyze it */
  section.source = s;
  section.pos = 0;
  section.size = s->size;
  detect(&section, 0);

  /* finish it up */
  print_line(0, "");
  close_source(s);
}

/*
 * Mac OS type & creator code
 */

#ifdef USE_MACOS_TYPE

static void show_macos_type(const char *filename)
{
  int err;
  FSRef ref;
  FSCatalogInfo info;
  FInfo *finfo;

  err = FSPathMakeRef(filename, &ref, NULL);
  if (err == 0) {
    err = FSGetCatalogInfo(&ref, kFSCatInfoFinderInfo,
			   &info, NULL, NULL, NULL);
  }

  if (err == 0) {
    finfo = (FInfo *)(info.finderInfo);
    if (finfo->fdType != 0 || finfo->fdCreator != 0) {
      char typecode[5], creatorcode[5], s1[256], s2[256];

      memcpy(typecode, &finfo->fdType, 4);
      typecode[4] = 0;
      format_ascii(typecode, s1);

      memcpy(creatorcode, &finfo->fdCreator, 4);
      creatorcode[4] = 0;
      format_ascii(creatorcode, s2);

      print_line(0, "Type code \"%s\", creator code \"%s\"",
		 s1, s2);
    } else {
      print_line(0, "No type and creator code");
    }
  }
  if (err) {
    print_line(0, "Type and creator code unknown (error %d)", err);
  }
}

#endif

/*
 * output formatting
 */

static const char *insets[] = {
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
  printf("%s%s\n", insets[level], line_akku);
}

void format_size(char *buf, int8 size, int mult)
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

void format_ascii(char *from, char *to)
{
  unsigned char *p = (unsigned char *)from;
  unsigned char *q = (unsigned char *)to;
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

void format_unicode(char *from, char *to)
{
  unsigned char *p = (unsigned char *)from;
  unsigned char *q = (unsigned char *)to;
  int c;

  for (;;) {
    c = get_be_short(p);
    if (c == 0)
      break;
    p += 2;

    if (c >= 127 || c < 32) {
      *q++ = '<';
      *q++ = "0123456789ABCDEF"[c >> 12];
      *q++ = "0123456789ABCDEF"[(c >> 8) & 15];
      *q++ = "0123456789ABCDEF"[(c >> 4) & 15];
      *q++ = "0123456789ABCDEF"[c & 15];
      *q++ = '>';
    } else {
      *q++ = c;
    }
  }
  *q = 0;
}

/*
 * data access
 */

unsigned int get_be_short(void *from)
{
  unsigned char *p = from;
  return ((unsigned int)(p[0]) << 8) +
    (unsigned int)p[1];
}

unsigned long get_be_long(void *from)
{
  unsigned char *p = from;
  return ((unsigned long)(p[0]) << 24) +
    ((unsigned long)(p[1]) << 16) +
    ((unsigned long)(p[2]) << 8) +
    (unsigned long)p[3];
}

unsigned long long get_be_quad(void *from)
{
  unsigned char *p = from;
  return ((unsigned long long)(p[0]) << 56) +
    ((unsigned long long)(p[1]) << 48) +
    ((unsigned long long)(p[2]) << 40) +
    ((unsigned long long)(p[3]) << 32) +
    ((unsigned long long)(p[4]) << 24) +
    ((unsigned long long)(p[5]) << 16) +
    ((unsigned long long)(p[6]) << 8) +
    (unsigned long long)p[7];
}

unsigned int get_le_short(void *from)
{
  unsigned char *p = from;
  return ((unsigned int)(p[1]) << 8) +
    (unsigned int)p[0];
}

unsigned long get_le_long(void *from)
{
  unsigned char *p = from;
  return ((unsigned long)(p[3]) << 24) +
    ((unsigned long)(p[2]) << 16) +
    ((unsigned long)(p[1]) << 8) +
    (unsigned long)p[0];
}

unsigned long long get_le_quad(void *from)
{
  unsigned char *p = from;
  return ((unsigned long long)(p[7]) << 56) +
    ((unsigned long long)(p[6]) << 48) +
    ((unsigned long long)(p[5]) << 40) +
    ((unsigned long long)(p[4]) << 32) +
    ((unsigned long long)(p[3]) << 24) +
    ((unsigned long long)(p[2]) << 16) +
    ((unsigned long long)(p[1]) << 8) +
    (unsigned long long)p[0];
}

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
