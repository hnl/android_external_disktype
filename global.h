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

typedef long long int int8;

/* detection stuff */

void detect(void);
void detect_from(int8 pos, int level);

typedef void (*DETECTOR)(int8 pos, int level);

/* buffer stuff */

void init_buffer(int fd, int8 filesize);
int8 get_buffer(int8 pos, int8 len, void **buf);

/* output formatting */

void print_line(int level, const char *fmt, ...);
void start_line(const char *fmt, ...);
void continue_line(const char *fmt, ...);
void finish_line(int level);

void format_size(char *buf, int8 size, int mult);

void format_ascii(char *from, char *to);
void format_unicode(char *from, char *to);

/* data access */

unsigned int       get_be_short(void *from);
unsigned long      get_be_long(void *from);
unsigned long long get_be_quad(void *from);

unsigned int       get_le_short(void *from);
unsigned long      get_le_long(void *from);
unsigned long long get_le_quad(void *from);

void get_pstring(void *from, char *to);

int find_memory(void *haystack, int haystack_len,
		void *needle, int needle_len);

/* error functions */

void error(const char *msg, ...);
void errore(const char *msg, ...);
void bailout(const char *msg, ...);
void bailoute(const char *msg, ...);

/* EOF */
