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
#ifndef __R_BITSET_H__
#define __R_BITSET_H__

#if !defined(__RLIB_H_INCLUDE_GUARD__) && !defined(RLIB_COMPILATION)
#error "#include <rlib.h> only pelase."
#endif

#include <rlib/rtypes.h>
#include <rlib/rmem.h>

R_BEGIN_DECLS

typedef ruint64 rbsword;
typedef void (*RBitsetFunc) (rsize bit, rpointer user);

typedef struct {
  rsize   bits;
  rsize   words;
  rbsword data[0];
} RBitset;

#define r_bitset_init_stack(BS, BITS)                                         \
    (((BS) = r_alloca0 (sizeof (RBitset) + sizeof (rbsword) * (1 + (BITS - 1) / (sizeof (rbsword) * 8)))) != NULL && \
    ((BS)->bits = (BITS)) > 0 &&                                              \
    ((BS)->words = (1 + (BITS - 1) / (sizeof (rbsword) * 8))) > 0)
#define r_bitset_init_heap(BS, BITS)                                          \
    (((BS) = r_malloc0 (sizeof (RBitset) + sizeof (rbsword) * (1 + (BITS - 1) / (sizeof (rbsword) * 8)))) != NULL && \
    ((BS)->bits = (BITS)) > 0 &&                                              \
    ((BS)->words = (1 + (BITS - 1) / (sizeof (rbsword) * 8))) > 0)

R_API RBitset * r_bitset_new_from_binary (rconstpointer data, rsize size);
R_API rboolean r_bitset_copy (RBitset * dest, const RBitset * src);
R_API rboolean r_bitset_set_bit (RBitset * bitset, rsize bit, rboolean set);
R_API rboolean r_bitset_set_bits (RBitset * bitset,
    const rsize * bits, rsize count, rboolean set);
#define r_bitset_clear(bs)  r_bitset_set_all (bs, FALSE)
R_API rboolean r_bitset_set_all (RBitset * bitset, rboolean set);
R_API rboolean r_bitset_set_n_bits_at (RBitset * bitset, rsize n, rsize bit, rboolean set);
R_API rboolean r_bitset_set_u8_at (RBitset * bitset, ruint8 u8, rsize bit);
R_API rboolean r_bitset_set_u16_at (RBitset * bitset, ruint16 u16, rsize bit);
R_API rboolean r_bitset_set_u32_at (RBitset * bitset, ruint32 u32, rsize bit);
R_API rboolean r_bitset_set_u64_at (RBitset * bitset, ruint64 u64, rsize bit);
R_API rboolean r_bitset_set_from_human_readable (RBitset * bitset, const rchar * str, rsize * bits);
R_API rboolean r_bitset_set_from_human_readable_file (RBitset * bitset, const rchar * file, rsize * bits);
#define r_bitset_inv(bs)  r_bitset_not (bs, bs)
R_API rboolean r_bitset_shr (RBitset * a, ruint count);
R_API rboolean r_bitset_shl (RBitset * a, ruint count);

R_API ruint8  r_bitset_get_u8_at  (const RBitset * bitset, rsize bit);
R_API ruint16 r_bitset_get_u16_at (const RBitset * bitset, rsize bit);
R_API ruint32 r_bitset_get_u32_at (const RBitset * bitset, rsize bit);
R_API ruint64 r_bitset_get_u64_at (const RBitset * bitset, rsize bit);
R_API rchar * r_bitset_to_human_readable (const RBitset * bitset);

R_API rboolean r_bitset_is_bit_set (const RBitset * bitset, rsize bit);
R_API rsize r_bitset_popcount (const RBitset * bitset);
R_API rsize r_bitset_clz (const RBitset * bitset);
R_API rsize r_bitset_ctz (const RBitset * bitset);

R_API void r_bitset_foreach (const RBitset * bitset, rboolean set,
    RBitsetFunc func, rpointer user);

R_API rboolean r_bitset_not (RBitset * dest, const RBitset * src);
R_API rboolean r_bitset_or (RBitset * dest, const RBitset * a, const RBitset * b);
R_API rboolean r_bitset_xor (RBitset * dest, const RBitset * a, const RBitset * b);
R_API rboolean r_bitset_and (RBitset * dest, const RBitset * a, const RBitset * b);

R_END_DECLS

#endif /* __R_BITSET_H__ */

