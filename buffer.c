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

#define CHUNKBITS (12)
#define CHUNKSIZE (1<<CHUNKBITS)
#define CHUNKMASK (CHUNKSIZE-1)

#define HASHSIZE (13)
#define HASHFUNC(start) (((start)>>CHUNKBITS) % HASHSIZE)

/*
 * types
 */

typedef struct chunk {
  u8 start, end, len;
  void *buf;
  struct chunk *next, *prev;
} CHUNK;

typedef struct cache {
  CHUNK *hashtab[HASHSIZE];
  void *tempbuf;
} CACHE;

/*
 * helper functions
 */

static CHUNK * ensure_chunk(SOURCE *s, CACHE *cache, u8 start);
static CHUNK * get_chunk_alloc(CACHE *cache, u8 start);

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
  CACHE *cache;
  CHUNK *c;
  u8 end, first_chunk, last_chunk, curr_chunk, available;

  /* sanity check */
  if (len == 0)
    return 0;

  /* get source info */
  if (s->size && pos >= s->size) {
    /* error("Request for data beyond end of file (pos %llu)", pos); */
    return 0;
  }
  end = pos + len;
  if (s->size && end > s->size) {
    /* truncate to known end of file */
    end = s->size;
    len = end - pos;
  }

  /* get cache head, free old temp buffer if present */
  cache = (CACHE *)s->cache_head;
  if (cache == NULL) {
    cache = (CACHE *)malloc(sizeof(CACHE));
    if (cache == NULL)
      bailout("Out of memory");
    memset(cache, 0, sizeof(CACHE));
    s->cache_head = (void *)cache;
  }
  if (cache->tempbuf != NULL) {
    free(cache->tempbuf);
    cache->tempbuf = NULL;
  }

  /* calculate involved chunks */
  first_chunk = pos & ~CHUNKMASK;
  last_chunk = (end - 1) & ~CHUNKMASK;

  if (last_chunk == first_chunk) {
    /* just get the matching chunk */
    c = ensure_chunk(s, cache, first_chunk);

    /* calculate return data */
    if (buf)
      *buf = c->buf + (pos - c->start);
    if (c->len <= pos - c->start)
      len = 0;
    else {
      available = c->len - (pos - c->start);
      if (len > available)
	len = available;
    }
    return len;

  } else {

    /* allocate temp buffer */
    cache->tempbuf = malloc(last_chunk - first_chunk + CHUNKSIZE);
    if (cache->tempbuf == NULL) {
      error("Out of memory, still going");
      return 0;
    }

    /* ensure all chunks are present and filled (to the extent possible) */
    available = 0;
    for (curr_chunk = first_chunk; curr_chunk <= last_chunk;
	 curr_chunk += CHUNKSIZE) {
      /* get that chunk */
      c = ensure_chunk(s, cache, curr_chunk);

      /* copy all available data */
      if (c->len) {
	memcpy(cache->tempbuf + available, c->buf, c->len);
	available += c->len;
      }

      /* stop after an incomplete chunk (possibly-okay for the last one,
	 not-so-nice for earlier ones, but treated all the same) */
      if (c->len < CHUNKSIZE)
	break;
    }

    /* calculate return data */
    if (buf)
      *buf = cache->tempbuf + (pos - first_chunk);
    if (available <= pos - first_chunk)
      len = 0;
    else {
      available -= pos - first_chunk;
      if (len > available)
	len = available;
    }
    return len;

  }
}

static CHUNK * ensure_chunk(SOURCE *s, CACHE *cache, u8 start)
{
  CHUNK *c;
  u8 result, curr_chunk;

  c = get_chunk_alloc(cache, start);
  if (c->len >= CHUNKSIZE)
    return c;  /* all is well */

  if (s->sequential) {
    if (s->seq_pos < start) {
      /* try to read in-between data */
      curr_chunk = s->seq_pos & ~CHUNKMASK;
      while (curr_chunk < start) {  /* runs at least once, due to the if()
				       and the formula of curr_chunk */
	ensure_chunk(s, cache, curr_chunk);
	curr_chunk += CHUNKSIZE;
	if (s->seq_pos < curr_chunk)
	  break;  /* it didn't work out... */
      }
    }

    if (s->seq_pos != c->end)
      return c;  /* we're not in a sane state, give up */
  }

  /* try to read the missing piece */
  result = s->read(s, c->start + c->len, CHUNKSIZE - c->len,
		   c->buf + c->len);
  if (result > 0) {
    /* adjust offsets */
    c->len += result;
    c->end = c->start + c->len;
    if (s->sequential)
      s->seq_pos += result;
  }  /* NOTE: errors are not reported here */

  return c;
}

static CHUNK * get_chunk_alloc(CACHE *cache, u8 start)
{
  int hpos;
  CHUNK *chain, *trav, *c;

  hpos = HASHFUNC(start);
  chain = cache->hashtab[hpos];

  if (chain == NULL) {
    c = (CHUNK *)malloc(sizeof(CHUNK));
    if (c == NULL)
      bailout("Out of memory");
    c->buf = malloc(CHUNKSIZE);
    if (c->buf == NULL)
      bailout("Out of memory");
    c->start = start;
    c->end = start;
    c->len = 0;
    /* create a new ring list */
    c->prev = c->next = c;
    cache->hashtab[hpos] = c;
    return c;
  }

  trav = chain;
  do {
    if (trav->start == start)
      return trav;
    trav = trav->next;
  } while (trav != chain);

  c = (CHUNK *)malloc(sizeof(CHUNK));
  if (c == NULL)
    bailout("Out of memory");
  c->buf = malloc(CHUNKSIZE);
  if (c->buf == NULL)
    bailout("Out of memory");
  c->start = start;
  c->end = start;
  c->len = 0;
  /* add to ring list before chain, becomes new head */
  c->prev = chain->prev;
  c->next = chain;
  c->prev->next = c;
  c->next->prev = c;
  cache->hashtab[hpos] = c;
  return c;
}

/*
 * dispose of a source
 */

void close_source(SOURCE *s)
{
  CACHE *cache;
  int hpos;
  CHUNK *chain, *trav, *nexttrav;

  /* drop the cache */
  cache = (CACHE *)s->cache_head;
  if (cache != NULL) {
    if (cache->tempbuf != NULL)
      free(cache->tempbuf);
    for (hpos = 0; hpos < HASHSIZE; hpos++) {
      chain = cache->hashtab[hpos];
      if (chain != NULL) {
	trav = chain;
	do {
	  nexttrav = trav->next;
	  free(trav->buf);
	  free(trav);
	  trav = nexttrav;
	} while (trav != chain);
      }
    }
  }

  /* type-specific cleanup */
  if (s->close != NULL)
    (*s->close)(s);

  /* release memory for the structure */
  free(s);
}

/* EOF */
