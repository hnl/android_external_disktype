/*
 * buffer.c
 * Reads data from disk and maintains a cache.
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
 * constants
 */

#define ROUNDSIZE (4096)
#define ROUNDMASK (ROUNDSIZE-1)

/*
 * types
 */

typedef struct readcache {
  int8 start, end, len;
  void *buf;
  struct readcache *next;
} READCACHE;

/* TODO: use a sorted tree (AVL?) instead */

/*
 * module-global vars
 */

static int fd;
static int8 limit;

static READCACHE *cache = NULL;

/*
 * helper functions
 */

static int8 read_safely(int fd, void *buf, int8 len);

/*
 * initialize the system
 */

void init_buffer(int the_fd, int8 filesize)
{
  READCACHE *trav, *travnext;

  /* drop any earlier cache */
  for (trav = cache; trav != NULL; trav = travnext) {
    travnext = trav->next;
    free(trav->buf);
    free(trav);
  }
  cache = NULL;

  /* remember the descriptor */
  fd = the_fd;
  limit = filesize;
  if (limit < 0)
    limit = 0;

  /* put beginning of file into cache */
  get_buffer(0, 32768, NULL);
}

/*
 * retrieve a piece of the file
 */

int8 get_buffer(int8 pos, int8 len, void **buf)
{
  READCACHE *trav, *rc;
  int8 end = pos + len;
  int8 round_start, round_len;
  int8 available;
  int8 result;

  /* sanity check */
  if (len <= 0 || pos < 0)
    return 0;

  /* TODO: keep a deduced "size limit" around and check it */


  /* STEP 1: try to fulfill from chache right away */

  /* search for a block that wholly contains the request */
  for (trav = cache; trav != NULL; trav = trav->next) {
    if (trav->start <= pos && trav->end >= end) {
      /* found, calculate return data */
      if (buf)
	*buf = trav->buf + (pos - trav->start);
      return len;
    }
  }


  /* STEP 2: try to fulfill by concatenating existing blocks */

  /* round position and length to chunk size */
  round_start = pos;
  round_len = len;
  if (round_start & ROUNDMASK) {
    round_len += (round_start & ROUNDMASK);
    round_start &= ~ROUNDMASK;
  }
  if (round_len & ROUNDMASK)
    round_len += ROUNDSIZE - (round_len & ROUNDMASK);

  /* allocate cache block with buffer */
  rc = (READCACHE *)malloc(sizeof(READCACHE));
  if (rc == NULL)
    bailout("Out of memory");
  rc->start = round_start;
  rc->len = round_len;
  rc->buf = malloc(rc->len);
  if (rc->buf == NULL)
    bailout("Out of memory");

  /* TODO: fill with parts from existing chunks (real func of step 2) */


  /* STEP 3: just read the whole block from disk in the req'd size */

  /* seek and read, freeing the new block on error */
  result = lseek(fd, rc->start, SEEK_SET);
  if (result != rc->start) {
    errore("Seek to %lld returned %lld", rc->start, result);
    free(rc->buf);
    free(rc);
    return 0;
  }
  result = read_safely(fd, rc->buf, rc->len);
  if (result <= 0) {
    errore("Reading %lld bytes at %lld failed", rc->len, rc->start);
    free(rc->buf);
    free(rc);
    return 0;
  }
  if (result < rc->len) {
    errore("Reading %lld bytes at %lld got only %lld bytes",
	   rc->len, rc->start, result);
    rc->len = result;
    /* TODO: deduct a new size limit when not all was read */
  }

  /* adjust end marker (len may have changed above) and add to cache */
  rc->end = rc->start + rc->len;
  rc->next = cache;
  cache = rc;

  /* calculate return data (may be truncated) */
  if (buf)
    *buf = rc->buf + (pos - rc->start);
  available = rc->len - (pos - rc->start);
  if (available < 0)
    available = 0;
  if (len > available)
    len = available;
  return len;
}

/*
 * helper: a safe variant of read(2)
 *
 * note the slightly different error return semantics
 */

static int8 read_safely(int fd, void *buf, int8 len)
{
  int8 got, have = 0;

  while (len > 0) {
    got = read(fd, buf, len);
    if (got < 0) {
      if (errno == EINTR || errno == EAGAIN)
	continue;
      errore("On cache read");
      break;
    } else if (got == 0) {
      break;
    } else {
      have += got;
      len -= got;
      buf = ((char *)buf + got);
    }
  }
  return have;
}

/* EOF */
