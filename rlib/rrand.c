/* RLIB - Convenience library for useful things
 * Copyright (C) 2015-2016  Haakon Sporsheim <haakon.sporsheim@gmail.com>
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
#include <rlib/rprng-private.h>
#include <rlib/rmem.h>

RPrng *
r_prng_new (RPrngGetFunc func, rsize size)
{
  RPrng * ret;

  if ((ret = r_malloc (sizeof (RPrng) + size)) != NULL) {
    r_ref_init (ret, r_free);
    ret->get = func;
  }

  return ret;
}

ruint64
r_rand_prng_get (RPrng * prng)
{
  return prng->get (prng);
}

