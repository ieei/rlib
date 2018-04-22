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

#include "config.h"
#include "rjson-private.h"
#include <rlib/rjsonparser.h>

#include <rlib/rmem.h>
#include <rlib/data/rstring.h>

#include <math.h>

static RJsonResult
r_json_str_unescape (rchar * dst, const rchar * src, rssize size)
{
  if (size < 0)
    size = r_strlen (src);

  /* TODO: Implement unescaping... */
  r_memcpy (dst, src, size);
  return R_JSON_OK;
}

static RJsonResult
r_json_str_escape (RString * dst, const rchar * src, rssize size)
{
  if (size < 0)
    size = r_strlen (src);

  /* TODO: Escape string... */
  r_string_append_len (dst, src, size);
  return R_JSON_OK;
}

RJsonValue *
r_json_parse (rconstpointer data, rsize len, RJsonResult * res)
{
  RJsonValue * ret;
  RJsonParser * parser;

  if (R_UNLIKELY (data == NULL || len == 0)) {
    if (res != NULL)
      *res = R_JSON_INVAL;
    return NULL;
  }

  if ((parser = r_json_parser_new (data, len)) != NULL) {
    ret = r_json_parser_parse_all (parser, res);
    r_json_parser_unref (parser);
  } else {
    ret = NULL;
    if (res != NULL)
      *res = R_JSON_OOM;
  }

  return ret;
}

RJsonValue *
r_json_parse_buffer (RBuffer * buf, RJsonResult * res)
{
  RJsonValue * ret;
  RJsonParser * parser;

  if (R_UNLIKELY (buf == NULL)) {
    if (res != NULL)
      *res = R_JSON_INVAL;
    return NULL;
  }

  if ((parser = r_json_parser_new_buffer (buf)) != NULL) {
    ret = r_json_parser_parse_all (parser, res);
    r_json_parser_unref (parser);
  } else {
    ret = NULL;
    if (res != NULL)
      *res = R_JSON_OOM;
  }

  return ret;
}


typedef struct {
  RJsonFlags flags;
  RString * str;

  int indent;
  RJsonResult res;
} RJsonAppendCtx;
#define R_JSON_APPEND_CTX_INIT(flags)   { flags, NULL, 0, R_JSON_OK };

typedef void (*RJsonValueAppendFunc) (RJsonAppendCtx * ctx, const RJsonValue * value);

static void
r_json_append_ctx_newline (RJsonAppendCtx * ctx)
{
  if ((ctx->flags & R_JSON_COMPACT) != R_JSON_COMPACT) {
    if (ctx->indent == 0) {
      r_string_append_c (ctx->str, '\n');
    } else if ((ctx->flags & R_JSON_USE_TABS) == R_JSON_USE_TABS) {
      r_string_append_printf (ctx->str, "\n%*c", ctx->indent, '\t');
    } else {
      r_string_append_printf (ctx->str, "\n%*c", ctx->indent * 2, ' ');
    }
  }
}

static void
r_json_append_ctx_value (RJsonAppendCtx * ctx, const RJsonValue * value);

static void
r_json_append_ctx_object (RJsonAppendCtx * ctx, const RJsonValue * value)
{
  const RJsonObject * o = (const RJsonObject *) value;
  const RJsonValue * k, * v;
  rsize i;

  r_string_append_c (ctx->str, '{');
  ctx->indent++;

  for (i = 0; i < o->array.nsize; i++) {
    if (i > 0)
      r_string_append_c (ctx->str, ',');
    r_json_append_ctx_newline (ctx);

    v = r_kv_ptr_array_get_const (&o->array, i, (rconstpointer *)&k);
    r_json_append_ctx_value (ctx, k);
    r_string_append_c (ctx->str, ':');
    if ((ctx->flags & R_JSON_COMPACT) != R_JSON_COMPACT)
      r_string_append_c (ctx->str, ' ');
    r_json_append_ctx_value (ctx, v);
  }

  ctx->indent--;
  if (o->array.nsize > 0)
    r_json_append_ctx_newline (ctx);
  r_string_append_c (ctx->str, '}');
}

static void
r_json_append_ctx_array (RJsonAppendCtx * ctx, const RJsonValue * value)
{
  const RJsonArray * a = (const RJsonArray *) value;
  rsize i;

  r_string_append_c (ctx->str, '[');
  ctx->indent++;

  for (i = 0; i < a->array.nsize; i++) {
    if (i > 0)
      r_string_append_c (ctx->str, ',');
    r_json_append_ctx_newline (ctx);
    r_json_append_ctx_value (ctx, r_ptr_array_get_const (&a->array, i));
  }

  ctx->indent--;
  if (a->array.nsize > 0)
    r_json_append_ctx_newline (ctx);
  r_string_append_c (ctx->str, ']');
}

static void
r_json_append_ctx_number (RJsonAppendCtx * ctx, const RJsonValue * value)
{
  const RJsonNumber * num = (const RJsonNumber *)value;
  rdouble intpart;

  if (modf (num->v, &intpart) == 0.0)
    r_string_append_printf (ctx->str, "%d", (int)num->v);
  else
    r_string_append_printf (ctx->str, "%f", num->v);
}

static void
r_json_append_ctx_string (RJsonAppendCtx * ctx, const RJsonValue * value)
{
  r_string_append_c (ctx->str, '"');
  r_json_str_escape (ctx->str, ((const RJsonString *)value)->v, -1);
  r_string_append_c (ctx->str, '"');
}

static void
r_json_append_ctx_true (RJsonAppendCtx * ctx, const RJsonValue * value)
{
  (void) value;
  r_string_append (ctx->str, "true");
}

static void
r_json_append_ctx_false (RJsonAppendCtx * ctx, const RJsonValue * value)
{
  (void) value;
  r_string_append (ctx->str, "false");
}

static void
r_json_append_ctx_null (RJsonAppendCtx * ctx, const RJsonValue * value)
{
  (void) value;
  r_string_append (ctx->str, "null");
}


static void
r_json_append_ctx_value (RJsonAppendCtx * ctx, const RJsonValue * value)
{
  RJsonValueAppendFunc funcs[] = {
    r_json_append_ctx_object,
    r_json_append_ctx_array,
    r_json_append_ctx_number,
    r_json_append_ctx_string,
    r_json_append_ctx_true,
    r_json_append_ctx_false,
    r_json_append_ctx_null,
  };

  funcs[value->type] (ctx, value);
}

static RJsonResult
r_json_value_append_output (const RJsonValue * value, RJsonAppendCtx * ctx)
{
  r_json_append_ctx_value (ctx, value);
  return ctx->res;
}

RBuffer *
r_json_value_to_buffer (const RJsonValue * value,
    RJsonFlags flags, RJsonResult * res)
{
  RBuffer * ret;
  RJsonResult r = R_JSON_OOM;

  if (R_UNLIKELY (value == NULL)) {
    ret = NULL;
    r = R_JSON_INVAL;
  } else if ((ret = r_buffer_new ()) != NULL) {
    RJsonAppendCtx ctx = R_JSON_APPEND_CTX_INIT (flags);

    if ((ctx.str = r_string_new_sized (4096)) != NULL &&
        (r = r_json_value_append_output (value, &ctx)) == R_JSON_OK) {
      rsize alloc_size = r_string_alloc_size (ctx.str);
      rsize len = r_string_length (ctx.str);
      rchar * data = r_string_free_keep (ctx.str);
      RMem * mem = r_mem_new_take (R_MEM_FLAG_NONE, data, alloc_size, len, 0);

      if (R_UNLIKELY (mem == NULL)) {
        r_buffer_unref (ret);
        ret = NULL;
        r = R_JSON_OOM;
      } else if (r_buffer_mem_append (ret, mem)) {
        r_mem_unref (mem);
      } else {
        r_mem_unref (mem);
        r_buffer_unref (ret);
        ret = NULL;
        r = R_JSON_MAP_FAILED;
      }
    } else {
      r_string_free (ctx.str);
      r_buffer_unref (ret);
      ret = NULL;
    }
  }

  if (res != NULL)
    *res = r;
  return ret;
}

static rboolean
r_json_value_equal (rconstpointer a, rconstpointer b)
{
  const RJsonValue * va = a, * vb = b;

  if (va->type == vb->type) {
    switch (va->type) {
      case R_JSON_TYPE_OBJECT:
      case R_JSON_TYPE_ARRAY:
        /* FIXME: Implement?? */
        return a == b;
      case R_JSON_TYPE_NUMBER:
        return ((const RJsonNumber *)va)->v == ((const RJsonNumber *)vb)->v;
      case R_JSON_TYPE_STRING:
        return r_str_equals (((const RJsonString *)va)->v, ((const RJsonString *)vb)->v);
      case R_JSON_TYPE_TRUE:
      case R_JSON_TYPE_FALSE:
      case R_JSON_TYPE_NULL:
        return TRUE;
      default:
        break;
    }
  }

  return FALSE;
}

static void
r_json_object_free (RJsonObject * object)
{
  r_kv_ptr_array_clear (&object->array);
  r_free (object);
}

RJsonValue *
r_json_object_new (void)
{
  RJsonObject * ret;

  if ((ret = r_mem_new0 (RJsonObject)) != NULL) {
    r_ref_init (ret, r_json_object_free);
    ret->value.type = R_JSON_TYPE_OBJECT;
    r_kv_ptr_array_init (&ret->array, r_json_value_equal);
  }

  return &ret->value;
}

static void
r_json_array_free (RJsonArray * array)
{
  r_ptr_array_clear (&array->array);
  r_free (array);
}

RJsonValue *
r_json_array_new (void)
{
  RJsonArray * ret;

  if ((ret = r_mem_new0 (RJsonArray)) != NULL) {
    r_ref_init (ret, r_json_array_free);
    ret->value.type = R_JSON_TYPE_ARRAY;
    r_ptr_array_init (&ret->array);
  }

  return &ret->value;
}

RJsonValue *
r_json_number_new_double (rdouble value)
{
  RJsonNumber * ret;

  if ((ret = r_mem_new (RJsonNumber)) != NULL) {
    r_ref_init (ret, r_free);
    ret->value.type = R_JSON_TYPE_NUMBER;
    ret->v = value;
  }

  return &ret->value;
}

RJsonValue *
r_json_string_new (const rchar * value, rssize size)
{
  RJsonString * ret;

  if (size < 0)
    size = r_strlen (value);

  if ((ret = r_malloc (sizeof (RJsonString) + size + 1)) != NULL) {
    rchar * data = (rchar *) (ret + 1);
    r_ref_init (ret, r_free);
    ret->value.type = R_JSON_TYPE_STRING;
    r_json_str_unescape (data, value, size);
    data[size] = 0;
    ret->v = data;
  }

  return &ret->value;
}

RJsonValue *
r_json_string_new_unescaped (const rchar * value, rssize size)
{
  RJsonString * ret;

  if (size < 0)
    size = r_strlen (value);

  if ((ret = r_malloc (sizeof (RJsonString) + size + 1)) != NULL) {
    rchar * data = (rchar *) (ret + 1);
    r_ref_init (ret, r_free);
    ret->value.type = R_JSON_TYPE_STRING;
    r_memcpy (data, value, size);
    data[size] = 0;
    ret->v = data;
  }

  return &ret->value;
}

RJsonValue *
r_json_string_new_static (const rchar * value)
{
  RJsonString * ret;

  if ((ret = r_mem_new (RJsonString)) != NULL) {
    r_ref_init (ret, r_free);
    ret->value.type = R_JSON_TYPE_STRING;
    ret->v = value;
  }

  return &ret->value;
}

RJsonValue *
r_json_value_new (RJsonType type)
{
  RJsonValue * ret;

  if ((ret = r_mem_new (RJsonValue)) != NULL) {
    r_ref_init (ret, r_free);
    ret->type = type;
  }

  return ret;
}


RJsonResult
r_json_object_add_field (RJsonValue * obj, RJsonValue * key, RJsonValue * value)
{
  RJsonObject * o = (RJsonObject *)obj;

  if (R_UNLIKELY (obj == NULL || key == NULL || value == NULL))
    return R_JSON_INVAL;
  if (R_UNLIKELY (obj->type != R_JSON_TYPE_OBJECT || key->type != R_JSON_TYPE_STRING))
    return R_JSON_WRONG_TYPE;

  r_kv_ptr_array_add (&o->array, r_json_value_ref (key), r_json_value_unref,
      r_json_value_ref (value), r_json_value_unref);
  return R_JSON_OK;
}

RJsonResult
r_json_array_add_value (RJsonValue * array, RJsonValue * value)
{
  RJsonArray * a = (RJsonArray *)array;

  if (R_UNLIKELY (array == NULL || value == NULL))
    return R_JSON_INVAL;
  if (R_UNLIKELY (array->type != R_JSON_TYPE_ARRAY))
    return R_JSON_WRONG_TYPE;

  r_ptr_array_add (&a->array, r_json_value_ref (value), r_json_value_unref);
  return R_JSON_OK;
}


rsize
r_json_value_get_object_field_count (const RJsonValue * value)
{
  if (value != NULL && value->type == R_JSON_TYPE_OBJECT) {
    const RJsonObject * object = (const RJsonObject *) value;
    return r_kv_ptr_array_size (&object->array);
  }

  return 0;
}

const rchar *
r_json_value_get_object_field_name (RJsonValue * value, rsize idx)
{
  if (value != NULL && value->type == R_JSON_TYPE_OBJECT) {
    RJsonObject * object = (RJsonObject *) value;
    const RJsonValue * key = r_kv_ptr_array_get_key (&object->array, idx);

    return r_json_value_get_string (key);
  }

  return NULL;
}

RJsonValue *
r_json_value_get_object_field_value (RJsonValue * value, rsize idx)
{
  RJsonValue * ret;

  if (value != NULL && value->type == R_JSON_TYPE_OBJECT) {
    RJsonObject * object = (RJsonObject *) value;

    if ((ret = r_kv_ptr_array_get_val (&object->array, idx)) != NULL)
      r_json_value_ref (ret);
  } else {
    ret = NULL;
  }

  return ret;
}

RJsonValue *
r_json_value_get_object_field (RJsonValue * value, const rchar * key)
{
  RJsonValue * ret = NULL;

  if (value != NULL && value->type == R_JSON_TYPE_OBJECT) {
    RJsonObject * object = (RJsonObject *) value;
    RJsonValue * cmp = r_json_string_new_static (key);

    if ((ret = r_kv_ptr_array_get_val (&object->array,
            r_kv_ptr_array_find (&object->array, cmp))) != NULL) {
      r_json_value_ref (ret);
    }
    r_json_value_unref (cmp);
  }

  return ret;
}

void
r_json_value_foreach_object_field (RJsonValue * value, RKeyValueFunc func, rpointer user)
{
  if (value != NULL && value->type == R_JSON_TYPE_OBJECT) {
    RJsonObject * object = (RJsonObject *) value;

    r_kv_ptr_array_foreach (&object->array, func, user);
  }
}

rsize
r_json_value_get_array_size (const RJsonValue * value)
{
  if (value != NULL && value->type == R_JSON_TYPE_ARRAY) {
    const RJsonArray * array = (const RJsonArray *) value;
    return r_ptr_array_size (&array->array);
  }

  return 0;
}

RJsonValue *
r_json_value_get_array_value (RJsonValue * value, rsize idx)
{
  RJsonValue * ret;

  if (value != NULL && value->type == R_JSON_TYPE_ARRAY) {
    RJsonArray * array = (RJsonArray *) value;

    if ((ret = r_ptr_array_get (&array->array, idx)) != NULL)
      r_json_value_ref (ret);
  } else {
    ret = NULL;
  }

  return ret;
}

void
r_json_value_foreach_array_value (RJsonValue * value, RFunc func, rpointer user)
{
  if (value != NULL && value->type == R_JSON_TYPE_ARRAY) {
    RJsonArray * array = (RJsonArray *) value;

    r_ptr_array_foreach (&array->array, func, user);
  }
}

int
r_json_value_get_number_int (const RJsonValue * value)
{
  if (value != NULL && value->type == R_JSON_TYPE_NUMBER) {
    const RJsonNumber * number = (const RJsonNumber *)value;
    return (int)number->v;
  }

  return 0;
}

rdouble
r_json_value_get_number_double (const RJsonValue * value)
{
  if (value != NULL && value->type == R_JSON_TYPE_NUMBER) {
    const RJsonNumber * number = (const RJsonNumber *)value;
    return number->v;
  }

  return RDOUBLE_NAN;
}

const rchar *
r_json_value_get_string (const RJsonValue * value)
{
  if (value != NULL && value->type == R_JSON_TYPE_STRING) {
    const RJsonString * string = (const RJsonString *)value;
    return string->v;
  }

  return NULL;
}

rboolean
r_json_value_is_true (const RJsonValue * value)
{
  return value != NULL && value->type == R_JSON_TYPE_TRUE;
}

rboolean
r_json_value_is_false (const RJsonValue * value)
{
  return value != NULL && value->type == R_JSON_TYPE_FALSE;
}

rboolean
r_json_value_is_null (const RJsonValue * value)
{
  return value != NULL && value->type == R_JSON_TYPE_NULL;
}

