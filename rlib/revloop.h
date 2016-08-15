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
#ifndef __R_EV_LOOP_H__
#define __R_EV_LOOP_H__

#if !defined(__RLIB_H_INCLUDE_GUARD__) && !defined(RLIB_COMPILATION)
#error "#include <rlib.h> only pelase."
#endif

#include <rlib/rtypes.h>
#include <rlib/rref.h>
#include <rlib/rclock.h>

R_BEGIN_DECLS

typedef enum {
  R_EV_IO_READABLE    = (1 << 0),
  /*R_EV_IO_PRI         = (1 << 1),*/
  R_EV_IO_WRITABLE    = (1 << 2),
  R_EV_IO_ERROR       = (1 << 3),
  R_EV_IO_HANGUP      = (1 << 4),
} REvIOEvent;
typedef ruint REvIOEvents;

#if defined (R_OS_WIN32)
typedef HANDLE REvHandle;
#define R_EV_HANDLE_FMT       "p"
#define R_EV_HANDLE_INVALID   INVALID_HANDLE_VALUE
#elif defined (R_OS_UNIX)
typedef int REvHandle;
#define R_EV_HANDLE_FMT       "i"
#define R_EV_HANDLE_INVALID   -1
#endif

typedef struct _REvIO REvIO;
typedef void (*REvIOFunc) (rpointer data, REvIO * evio);
typedef void (*REvIOCB) (rpointer data, REvIOEvents events, REvIO * evio);

#define R_EV_IO_FORMAT        "%p [%"R_EV_HANDLE_FMT"]"
#define R_EV_IO_ARGS(evio)    evio, evio->handle

typedef enum {
  R_EV_LOOP_RUN_LOOP,
  R_EV_LOOP_RUN_ONCE,
  R_EV_LOOP_RUN_NOWAIT,
} REvLoopRunMode;

typedef struct _REvLoop REvLoop;
typedef void (*REvFunc) (rpointer data, REvLoop * loop);
typedef rboolean (*REvFuncReturn) (rpointer data, REvLoop * loop);

#define r_ev_loop_new() r_ev_loop_new_full (NULL)
R_API REvLoop * r_ev_loop_new_full (RClock * clock) R_ATTR_MALLOC;
R_API REvLoop * r_ev_loop_default (void);
#define r_ev_loop_ref r_ref_ref
#define r_ev_loop_unref r_ref_unref

R_API ruint r_ev_loop_run (REvLoop * loop, REvLoopRunMode mode);
R_API void r_ev_loop_stop (REvLoop * loop);

R_API rboolean r_ev_loop_add_prepare (REvLoop * loop, REvFuncReturn prepare_cb,
    rpointer data, RDestroyNotify datanotify);
R_API rboolean r_ev_loop_add_idle (REvLoop * loop, REvFuncReturn idle_cb,
    rpointer data, RDestroyNotify datanotify);
R_API rboolean r_ev_loop_add_callback (REvLoop * loop, rboolean pri,
    REvFunc cb, rpointer data, RDestroyNotify datanotify);

R_API rboolean r_ev_loop_add_callback_at (REvLoop * loop, RClockEntry ** entry,
    RClockTime deadline, REvFunc cb, rpointer data, RDestroyNotify datanotify);
R_API rboolean r_ev_loop_add_callback_later (REvLoop * loop, RClockEntry ** entry,
    RClockTimeDiff delay, REvFunc cb, rpointer data, RDestroyNotify datanotify);
R_API rboolean r_ev_loop_cancel_timer (REvLoop * loop, RClockEntry * entry);

R_API REvIO * r_ev_loop_init_handle (REvLoop * loop, REvHandle handle);
#define r_ev_io_ref r_ref_ref
#define r_ev_io_unref r_ref_unref

R_API rboolean r_ev_io_start (REvIO * evio, REvIOEvents events, REvIOCB io_cb,
    rpointer data, RDestroyNotify datanotify);
R_API rboolean r_ev_io_stop (REvIO * evio);
R_API rboolean r_ev_io_close (REvIO * evio, REvIOFunc close_cb,
    rpointer data, RDestroyNotify datanotify);

rboolean r_ev_handle_close (REvHandle handle);
#ifdef R_OS_UNIX
rboolean r_ev_handle_set_nonblocking (REvHandle handle, rboolean set);
rboolean r_ev_handle_set_cloexec (REvHandle handle, rboolean set);
#endif

R_END_DECLS

#endif /* __R_EV_LOOP_H__ */
