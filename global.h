/*
 * global.h
 * Common header file.
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

#define PROGNAME "disktype"


/* global includes */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/* types */

typedef signed char s1;
typedef unsigned char u1;
typedef short int s2;
typedef unsigned short int u2;
typedef long int s4;
typedef unsigned long int u4;
typedef long long int s8;
typedef unsigned long long int u8;

typedef struct source {
  u8 size;
  void *cache_head;

  int sequential;
  u8 seq_pos;
  struct source *foundation;

  u8 (*read)(struct source *s, u8 pos, u8 len, void *buf);
  void (*close)(struct source *s);

  /* private data may follow */
} SOURCE;

typedef struct section {
  u8 pos, size;
  SOURCE *source;
} SECTION;

typedef void (*DETECTOR)(SECTION *section, int level);

/* for compatibility only */
typedef s8 int8;


/* detection dispatching function */

void detect(SECTION *section, int level);

/* source and buffer functions */

SOURCE *init_file_source(int fd);

u8 get_buffer(SECTION *section, u8 pos, u8 len, void **buf);
u8 get_buffer_real(SOURCE *s, u8 pos, u8 len, void **buf);
void close_source(SOURCE *s);

/* output functions */

void print_line(int level, const char *fmt, ...);
void start_line(const char *fmt, ...);
void continue_line(const char *fmt, ...);
void finish_line(int level);

/* formatting functions */

void format_size(char *buf, u8 size, int mult);

void format_ascii(char *from, char *to);
void format_unicode(char *from, char *to);

void format_uuid(void *uuid, char *to);

/* endian-aware data access */

u2 get_be_short(void *from);
u4 get_be_long(void *from);
u8 get_be_quad(void *from);

u2 get_le_short(void *from);
u4 get_le_long(void *from);
u8 get_le_quad(void *from);

u2 get_ve_short(int endianness, void *from);
u4 get_ve_long(int endianness, void *from);
u8 get_ve_quad(int endianness, void *from);

const char * get_ve_name(int endianness);

/* more data access */

void get_pstring(void *from, char *to);

int find_memory(void *haystack, int haystack_len,
		void *needle, int needle_len);

/* error functions */

void error(const char *msg, ...);
void errore(const char *msg, ...);
void bailout(const char *msg, ...);
void bailoute(const char *msg, ...);

/* EOF */
