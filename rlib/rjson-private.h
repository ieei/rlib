/* RLIB - Convenience library for useful things
 * Copyright (C) 2018 Haakon Sporsheim <haakon.sporsheim@gmail.com>
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
#ifndef __R_JSON_PRIV_H__
#define __R_JSON_PRIV_H__

#if !defined(RLIB_COMPILATION)
#error "rjson-private.h should only be used internally in rlib!"
#endif

#include <rlib/rjson.h>

#include <rlib/data/rkvptrarray.h>
#include <rlib/data/rptrarray.h>

R_BEGIN_DECLS

struct _RJsonObject {
  RJsonValue value;
  RKVPtrArray array;
};

struct _RJsonArray  {
  RJsonValue value;
  RPtrArray array;
};

struct _RJsonNumber {
  RJsonValue value;
  rdouble v;
};

struct _RJsonString {
  RJsonValue value;
  const rchar * v;
  rsize len;
};

struct _RJsonTrue {
  RJsonValue value;
};

struct _RJsonFalse {
  RJsonValue value;
};

struct _RJsonNull {
  RJsonValue value;
};

R_END_DECLS

#endif /* __R_JSON_PRIV_H__ */

