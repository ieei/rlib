/* RLIB - Convenience library for useful things
 * Copyright (C) 2016  Haakon Sporsheim <haakon.sporsheim@gmail.com>
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
#include <rlib/rref.h>

rpointer
r_ref_ref (rpointer ref)
{
  RRef * self = ref;
  r_atomic_uint_fetch_add (&self->refcount, 1);
  return ref;
}

void
r_ref_unref (rpointer ref)
{
  RRef * self = ref;
  if (r_atomic_uint_fetch_sub (&self->refcount, 1) == 1) {
    if (R_LIKELY (self->notify != NULL))
      self->notify (ref);
  }
}