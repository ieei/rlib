/* RLIB - Convenience library for useful things
 * Copyright (C) 2015  Haakon Sporsheim <haakon.sporsheim@gmail.com>
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
#include <rlib/rtty.h>
#include <rlib/rstr.h>

static RPrintFunc printfunc         = NULL;
static rpointer   printfuncdata     = NULL;
static RPrintFunc printerrfunc      = NULL;
static rpointer   printerrfuncdata  = NULL;

void
r_override_print_handler (RPrintFunc func, rpointer data,
    RPrintFunc * oldfunc, rpointer * olddata)
{
  if (oldfunc != NULL)
    *oldfunc = printfunc;
  if (olddata != NULL)
    *olddata = printfuncdata;

  printfunc = func;
  printfuncdata = data;
}

void
r_override_printerr_handler (RPrintFunc func, rpointer data,
    RPrintFunc * oldfunc, rpointer * olddata)
{
  if (oldfunc != NULL)
    *oldfunc = printerrfunc;
  if (olddata != NULL)
    *olddata = printerrfuncdata;

  printerrfunc = func;
  printerrfuncdata = data;
}

int
r_print (const rchar * fmt, ...)
{
  va_list args;
  rchar *str = NULL;
  int ret;

  /* TODO: Prevent re-etnry? use TLS */

  va_start (args, fmt);
  ret = r_vasprintf (&str, fmt, args);
  va_end (args);

  if (R_LIKELY (ret > 0 && str != NULL)) {
    RPrintFunc func;
    rpointer data;

    if (R_LIKELY (printfunc == NULL)) {
      func = r_print_default;
      data = NULL;
    } else {
      func = printfunc;
      data = printfuncdata;
    }

    if (R_UNLIKELY (!func (str, (rsize)ret, data)))
      ret = -1;
  }

  r_free (str);
  return ret;
}

int
r_printerr (const rchar * fmt, ...)
{
  va_list args;
  rchar *str = NULL;
  int ret;

  /* TODO: Prevent re-etnry? use TLS */

  va_start (args, fmt);
  ret = r_vasprintf (&str, fmt, args);
  va_end (args);

  if (R_LIKELY (ret > 0 && str != NULL)) {
    RPrintFunc func;
    rpointer data;

    if (R_LIKELY (printerrfunc == NULL)) {
      func = r_printerr_default;
      data = NULL;
    } else {
      func = printerrfunc;
      data = printerrfuncdata;
    }

    if (R_UNLIKELY (!func (str, (rsize)ret, data)))
      ret = -1;
  }

  r_free (str);
  return ret;
}

rboolean
r_print_default (const rchar * str, rsize size, rpointer data)
{
  (void)size;
  (void)data;
  fputs (str, stdout);
  return TRUE;
}

rboolean
r_printerr_default (const rchar * str, rsize size, rpointer data)
{
  (void)size;
  (void)data;
  fputs (str, stderr);
  return TRUE;
}

rchar *
r_tty_clr_to_str (RColorFlags clr, rchar str[R_TTY_MAX_CC])
{
  rchar * pos = str;
  *(pos++) = '\x1B';
  *(pos++) = '[';
  *(pos++) = '0';
  *(pos++) = '0';

  if (clr & R_CLR_FMT_BOLD) {
    *(pos++) = ';';
    *(pos++) = '1';
  }

  if (clr & R_CLR_FMT_UNDERLINE) {
    *(pos++) = ';';
    *(pos++) = '4';
  }

  if (clr & R_CLR_FG_MASK) {
    *(pos++) = ';';
    *(pos++) = '3';
    *(pos++) = (clr & R_CLR_FG_MASK) + '0';
  }

  if (clr & R_CLR_BG_MASK) {
    *(pos++) = ';';
    *(pos++) = '4';
    *(pos++) = ((clr & R_CLR_BG_MASK) >> 4) + '0';
  }

  *(pos++) = 'm';
  *pos = 0;
  return str;
}

