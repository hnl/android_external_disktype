/*
 * buffer.c
 * Manages data reading and caching.
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
  u8 start, end, len;
  void *buf;
  struct readcache *next;
} READCACHE;

/* TODO: use a sorted tree (AVL?) instead */

/*
 * retrieve a piece of the source, entry point for detection
 */

u8 get_buffer(SECTION *section, u8 pos, u8 len, void **buf)
{
  SOURCE *s;

  /* get source info */
  s = section->source;
  pos += section->pos;

  return get_buffer_real(s, pos, len, buf);
}

/*
 * actual retrieval, entry point for layering
 */

u8 get_buffer_real(SOURCE *s, u8 pos, u8 len, void **buf)
{
  READCACHE *trav, *rc;
  u8 end, round_start, round_len, result, available;

  /* sanity check */
  if (len == 0)
    return 0;

  /* get source info */
  if (s->size && pos >= s->size) {
    error("Request for data beyond end of file (pos %llu)", pos);
    return 0;
  }
  end = pos + len;

  /* STEP 1: try to fulfill from cache right away */

  /* search for a block that wholly contains the request */
  for (trav = (READCACHE *)s->cache_head; trav != NULL; trav = trav->next) {
    if (trav->start <= pos && trav->end >= end) {
      /* found, calculate return data */
      if (buf)
	*buf = trav->buf + (pos - trav->start);
      return len;
    }
  }


  /* TODO: special treatment for "sequential" sources */


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

  /* call the read function */
  result = s->read(s, rc->start, rc->len, rc->buf);
  if (result <= 0) {
    errore("Reading %llu bytes at %llu failed", rc->len, rc->start);
    free(rc->buf);
    free(rc);
    return 0;
  }
  if (result < rc->len) {
    errore("Reading %llu bytes at %llu got only %llu bytes",
	   rc->len, rc->start, result);
    rc->len = result;
  }

  /* adjust end marker (len may have changed above) and add to cache */
  rc->end = rc->start + rc->len;
  rc->next = (READCACHE *)s->cache_head;
  s->cache_head = rc;

  /* calculate return data (may be truncated) */
  if (buf)
    *buf = rc->buf + (pos - rc->start);
  if (rc->len <= pos - rc->start)
    len = 0;
  else {
    available = rc->len - (pos - rc->start);
    if (len > available)
      len = available;
  }
  return len;
}

/*
 * dispose of a source
 */

void close_source(SOURCE *s)
{
  READCACHE *trav, *travnext;

  /* drop the cache */
  for (trav = (READCACHE *)s->cache_head; trav != NULL; trav = travnext) {
    travnext = trav->next;
    free(trav->buf);
    free(trav);
  }
  s->cache_head = NULL;

  /* type-specific cleanup */
  if (s->close != NULL)
    (*s->close)(s);

  /* release memory for the structure */
  free(s);
}

/* EOF */
