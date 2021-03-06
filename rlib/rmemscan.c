/* RLIB - Convenience library for useful things
 * Copyright (C) 2016 Haakon Sporsheim <haakon.sporsheim@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 * See the COPYING file at the root of the source repository.
 */

#include "config.h"
#include <rlib/rmem.h>

#include <rlib/charset/rascii.h>

rpointer
r_mem_scan_simple_pattern (rconstpointer mem, rsize size,
    const rchar * pattern, rpointer * end)
{
  rpointer ret;
  RMemScanResult * res = NULL;

  if (r_mem_scan_pattern (mem, size, pattern, &res) == R_MEM_SCAN_RESULT_OK) {
    ret = res->ptr;
    if (end != NULL)
      *end = res->end;
  } else {
    ret = NULL;
  }

  r_free (res);
  return ret;
}

static RMemTokenType
r_mem_scan_pattern_next_token (const rchar * pattern,
    rsize * bytes, const rchar ** end)
{
  RMemTokenType ret;

  while (r_ascii_isspace (*pattern)) pattern++;

  *bytes = 0;

  if (*pattern == 0) {
    ret = R_MEM_TOKEN_NONE;
  } else if (*pattern == '*') {
    pattern++;
    while (r_ascii_isspace (*pattern) || *pattern == '*') pattern++;

    ret = R_MEM_TOKEN_WILDCARD;
  } else if (pattern[0] == '?' && pattern[1] == '?') {
    do {
      pattern += 2;
      (*bytes)++;
      while (r_ascii_isspace (*pattern)) pattern++;
    } while (pattern[0] == '?' && pattern[1] == '?');

    ret = R_MEM_TOKEN_WILDCARD_SIZED;
  } else if (r_ascii_isxdigit (pattern[0]) && r_ascii_isxdigit (pattern[1])) {
    do {
      pattern += 2;
      (*bytes)++;
      while (r_ascii_isspace (*pattern)) pattern++;
    } while (r_ascii_isxdigit (pattern[0]) && r_ascii_isxdigit (pattern[1]));

    ret = R_MEM_TOKEN_BYTES;
  } else {
    ret = R_MEM_TOKEN_NONE;
  }

  while (r_ascii_isspace (*pattern)) pattern++;
  *end = pattern;
  return ret;
}

static rsize
r_mem_scan_validate_pattern (const rchar * pattern)
{
  rsize ret = 0;
  if (R_LIKELY (pattern != NULL)) {
    rsize bytes;

    while (r_mem_scan_pattern_next_token (pattern, &bytes, &pattern) != R_MEM_TOKEN_NONE)
      ret++;

    if (*pattern != 0)
      ret = 0;
  }

  return ret;
}

#if 0
static RMemScanToken *
r_mem_scan_result_longest_token (RMemScanResult * result, RMemTokenType type)
{
  RMemScanToken * ret = NULL;
  rsize i;

  for (i = 0; i < result->tokens; i++) {
    if (result->token[i].type == type) {
      if (ret == NULL || result->token[i].chunk.size > ret->chunk.size)
        ret = &result->token[i];
    }
  }

  return ret;
}
#endif

static rsize
r_mem_scan_result_next_token (RMemScanResult * result, RMemTokenType type, rsize i)
{
  for (; i < result->tokens; i++) {
    if (result->token[i].type == type)
      break;
  }

  return i;
}

static ruint8 *
r_mem_scan_token_bytes (rconstpointer mem, rsize size, const RMemScanToken * token)
{
  ruint8 * ptr, * match = r_alloca (token->chunk.size);
  const rchar * pattern;
  rsize i;

  for (i = 0, ptr = match, pattern = token->pattern; i < token->chunk.size; i++, pattern += 2) {
    while (r_ascii_isspace (*pattern)) pattern++;
    *ptr++ = (ruint8)r_ascii_xdigit_value (pattern[0]) << 4 |
      (ruint8)r_ascii_xdigit_value (pattern[1]);
  }

  return r_mem_scan_data (mem, size, match, token->chunk.size);
}

static rboolean
r_mem_scan_wild_backward (RMemScanResult * result, rsize first, rsize idx,
    const ruint8 * start, const ruint8 * cur)
{
  rsize i;
  for (i = idx; i > first; ) {
    RMemScanToken * token = &result->token[--i];

    switch (token->type) {
      case R_MEM_TOKEN_WILDCARD_SIZED:
        if (start + token->chunk.size > cur)
          return FALSE;
        cur -= token->chunk.size;
        token->chunk.data = (ruint8 *)cur;
        break;
      case R_MEM_TOKEN_WILDCARD:
        if (i == first) {
          token->chunk.data = (ruint8 *)start;
          token->chunk.size = cur - start;
        } else {
          token->chunk.data = (ruint8 *)cur;
          token->chunk.size = 0;
        }
        break;
      default:
        return FALSE;
    }
  }

  return TRUE;
}

static rboolean
r_mem_scan_wild_forward (RMemScanResult * result, rsize first, rsize last,
    const ruint8 * cur, const ruint8 * end)
{
  rsize i;
  for (i = first; i < last; i++) {
    RMemScanToken * token = &result->token[i];

    token->chunk.data = (ruint8 *)cur;
    switch (token->type) {
      case R_MEM_TOKEN_WILDCARD_SIZED:
        if (cur + token->chunk.size > end)
          return FALSE;
        cur += token->chunk.size;
        break;
      case R_MEM_TOKEN_WILDCARD:
        token->chunk.size = (i + 1 == last) ? end - cur : 0;
        break;
      default:
        return FALSE;
    }
  }

  return TRUE;
}

static rboolean
r_mem_scan_wild_fill (RMemScanResult * result, rsize first, rsize last,
    const ruint8 * start, const ruint8 * end)
{
  rsize idx;

restart:
  while (first < last && result->token[first].type == R_MEM_TOKEN_WILDCARD_SIZED) {
    if (start + result->token[first].chunk.size > end)
      return FALSE;
    result->token[first].chunk.data = (ruint8 *)start;
    start += result->token[first].chunk.size;
    first++;
  }

  while (first < last && result->token[last - 1].type == R_MEM_TOKEN_WILDCARD_SIZED) {
    last--;
    if (end - result->token[last].chunk.size < start)
      return FALSE;
    end -= result->token[last].chunk.size;
    result->token[last].chunk.data = (ruint8 *)end;
  }

  if (first < last) {
    if ((idx = r_mem_scan_result_next_token (result, R_MEM_TOKEN_WILDCARD_SIZED, first)) < last) {
      while (first < last && result->token[first].type == R_MEM_TOKEN_WILDCARD) {
        result->token[first].chunk.data = (ruint8 *)start;
        result->token[first].chunk.size = 0;
        first++;
      }
      while (first < last && result->token[last - 1].type == R_MEM_TOKEN_WILDCARD) {
        last--;
        result->token[last].chunk.data = (ruint8 *)end;
        result->token[last].chunk.size = 0;
      }

      goto restart;
    } else {
      rsize i, dsize = (end - start) / (last - first);

      for (i = first; i < last; i++) {
        result->token[i].chunk.data = (ruint8 *)start + (i - first) * dsize;
        result->token[i].chunk.size = dsize;
      }
      result->token[i - 1].chunk.size = end - (const ruint8 *)result->token[i - 1].chunk.data;
    }
  }

  return TRUE;
}

RMemScanResultType
r_mem_scan_pattern (rconstpointer mem, rsize size,
    const rchar * pattern, RMemScanResult ** result)
{
  RMemScanResultType ret;
  rsize tokens;

  if (R_UNLIKELY (mem == NULL)) return R_MEM_SCAN_RESULT_INVAL;
  if (R_UNLIKELY (pattern == NULL)) return R_MEM_SCAN_RESULT_INVAL;
  if (R_UNLIKELY (result == NULL)) return R_MEM_SCAN_RESULT_INVAL;

  tokens = r_mem_scan_validate_pattern (pattern);

  if (R_UNLIKELY (tokens == 0))
    return R_MEM_SCAN_RESULT_INVALID_PATTERN;

  if (R_LIKELY ((*result = r_malloc (sizeof (RMemScanResult) + tokens * sizeof (RMemScanToken))) != NULL)) {
    RMemScanToken * t;
    rsize first, i;

    (*result)->tokens = tokens;

    /* Setup tokens based on pattern */
    for (i = 0; i < tokens && *pattern != 0; i++) {
      t = &(*result)->token[i];
      t->pattern = pattern;
      t->chunk.data = NULL;
      t->type = r_mem_scan_pattern_next_token (pattern, &t->chunk.size, &pattern);
    }

    if ((first = r_mem_scan_result_next_token (*result, R_MEM_TOKEN_BYTES, 0)) < tokens) {
      const ruint8 * wrkmem = mem;
      rsize wsize = size;

      /* We have some fixed bytes to find first! */
      ret = R_MEM_SCAN_RESULT_NOT_FOUND;
      do {
        ruint8 * ptr;
        rsize cur, next;

        if ((ptr = r_mem_scan_token_bytes (wrkmem, wsize, &(*result)->token[first])) == NULL)
          goto beach;

        (*result)->token[first].chunk.data = ptr;
        wrkmem = ptr + 1;
        wsize = size - (wrkmem - (const ruint8 *)mem);

        /* Check preamble */
        if (!r_mem_scan_wild_backward (*result, 0, first, mem, ptr))
          continue;

        ptr += (*result)->token[first].chunk.size;
        cur = first + 1;
        while ((next = r_mem_scan_result_next_token (*result, R_MEM_TOKEN_BYTES, cur)) < tokens) {
          while (TRUE) {
            ruint8 * nptr;
            if ((nptr = r_mem_scan_token_bytes (ptr, wsize - (ptr - wrkmem), &(*result)->token[next])) == NULL)
              goto beach;

            (*result)->token[next].chunk.data = nptr;
            if (r_mem_scan_wild_fill (*result, cur, next, ptr, nptr)) {
              ptr = nptr + (*result)->token[next].chunk.size;
              break;
            }

            ptr = nptr + 1;
          }
          cur = next + 1;
        }

        if (r_mem_scan_wild_forward (*result, cur, tokens, ptr, wrkmem + wsize))
          ret = R_MEM_SCAN_RESULT_OK;
      } while (wsize > 0 && ret == R_MEM_SCAN_RESULT_NOT_FOUND);
    } else {
      if (r_mem_scan_wild_fill (*result, 0, tokens, mem, (ruint8 *)mem + size))
        ret = R_MEM_SCAN_RESULT_OK;
      else
        ret = R_MEM_SCAN_RESULT_NOT_FOUND;
    }

    if (ret == R_MEM_SCAN_RESULT_OK) {
      (*result)->ptr = (*result)->token[0].chunk.data;
      (*result)->end = RSIZE_TO_POINTER (
          RPOINTER_TO_SIZE ((*result)->token[tokens-1].chunk.data) +
          (*result)->token[tokens-1].chunk.size);
    } else {
      r_free (*result);
      *result = NULL;
    }
  } else {
    ret = R_MEM_SCAN_RESULT_OOM;
  }

beach:
  return ret;
}

