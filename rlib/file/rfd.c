/* RLIB - Convenience library for useful things
 * Copyright (C) 2015-2018 Haakon Sporsheim <haakon.sporsheim@gmail.com>
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
#include "rfile-private.h"
#include <rlib/file/rfd.h>

#include <rlib/file/rfs.h>
#include <rlib/rstr.h>

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined (R_OS_WIN32)
#include <rlib/charset/runicode.h>
#define WIN32_LEAN_AND_MEAN 1
#include <windows.h>

#ifdef fstat
#undef fstat
#endif
#ifdef stat
#undef stat
#endif
#define fstat(a,b) _fstati64(a,b)
#define stat _stati64
#endif

#if defined (__APPLE__) && defined (__MACH__)
#define lseek64 lseek
#endif

#include <errno.h>

int
r_fd_open (const rchar * file, int flags, int mode)
{
  int fd = -1;
  if (R_UNLIKELY (file == NULL || *file == 0))
    return fd;

#ifdef RLIB_HAVE_FILES
#if defined (R_OS_WIN32)
  wchar_t * utf16file = r_utf8_to_utf16_dup (file, -1, NULL, NULL, NULL);
  fd = _wopen (utf16file, flags, mode);
  r_free (utf16file);
#elif defined (R_OS_UNIX)
  do {
    fd = open (file, flags, mode);
  } while (R_UNLIKELY (fd == -1 && errno == EINTR));
#endif
#else
  (void) flags;
  (void) mode;
#endif

  return fd;
}

int
r_fd_open_tmp (const rchar * dir, const rchar * pre, rchar ** path)
{
  return r_fd_open_tmp_full (dir, pre, O_WRONLY, 0600, path);
}

int
r_fd_open_tmp_full (const rchar * dir, const rchar * fileprefix,
    int flags, int mode, rchar ** path)
{
  rchar * file;
  int fd = -1;

  if ((file = r_fs_path_new_tmpname_full (dir, fileprefix, "")) != NULL) {
    fd = r_fd_open (file, flags | O_CREAT | O_EXCL, mode);

    if (path != NULL && fd >= 0)
      *path = file;
    else
      r_free (file);
  }

  return fd;
}

rboolean
r_fd_close (int fd)
{
  int res;
#ifdef RLIB_HAVE_FILES
#if defined (R_OS_WIN32)
  res = _close (fd);
#elif defined (R_OS_UNIX)
  res = close (fd);
#endif
#else
  (void) fd;
  res = -1;
#endif
  return res == 0;
}

rssize
r_fd_write (int fd, rconstpointer buf, rsize size)
{
  rssize res;
#ifdef RLIB_HAVE_FILES
#if defined (R_OS_WIN32)
  res = (rssize)_write (fd, buf, (unsigned int)size);
#elif defined (R_OS_UNIX)
  res = write (fd, buf, size);
#endif
#else
  (void) fd;
  (void) buf;
  (void) size;
  res = -1;
#endif
  return res;
}

rssize
r_fd_read (int fd, rpointer buf, rsize size)
{
  rssize res;
#ifdef RLIB_HAVE_FILES
#if defined (R_OS_WIN32)
  res = (rssize)_read (fd, buf, (unsigned int)size);
#elif defined (R_OS_UNIX)
  res = read (fd, buf, size);
#endif
#else
  (void) fd;
  (void) buf;
  (void) size;
  res = -1;
#endif
  return res;
}

roffset
r_fd_tell (int fd)
{
  roffset res;
#ifdef RLIB_HAVE_FILES
#if defined (R_OS_WIN32)
  res = (roffset)_telli64 (fd);
#elif defined (HAVE_LSEEK64)
  res = (roffset)lseek64 (fd, 0, SEEK_CUR);
#elif defined (HAVE_LSEEK)
  res = (roffset)lseek (fd, 0, SEEK_CUR);
#endif
#else
  (void) fd;
  res = -1;
#endif
  return res;
}

roffset
r_fd_seek (int fd, roffset offset, RSeekMode mode)
{
  roffset ret;
#ifdef RLIB_HAVE_FILES
#if defined (R_OS_WIN32)
  ret = (roffset)_lseeki64 (fd, offset, r_file_seek_mode_to_whence (mode));
#elif defined (HAVE_LSEEK64)
  ret = (roffset)lseek64 (fd, offset, r_file_seek_mode_to_whence (mode));
#elif defined (HAVE_LSEEK)
  ret = (roffset)lseek (fd, (off_t)offset, r_file_seek_mode_to_whence (mode));
#else
  (void) fd;
  (void) offset;
  (void) mode;
  ret = -1;
#endif
#else
  (void) fd;
  (void) offset;
  (void) mode;
  ret = -1;
#endif

  return ret;
}

ruint64
r_fd_get_filesize (int fd)
{
  ruint64 ret = 0;

#ifdef RLIB_HAVE_FILES
#if defined (HAVE_FSTAT)
  struct stat st;
  if (fstat (fd, &st) == 0)
    ret = (ruint64)st.st_size;
#else
  roffset res, cur;
  cur = r_fd_tell (fd);
  if ((res = r_fd_seek (fd, 0, R_SEEK_MODE_END)) >= 0)
    ret = (ruint64)res;
  r_fd_seek (fd, cur, R_SEEK_MODE_SET)
#endif
#endif

  return ret;
}

rboolean
r_fd_truncate (int fd, rsize size)
{
  int res;
#ifdef RLIB_HAVE_FILES
#if defined (R_OS_WIN32)
  res = (int)_chsize_s (fd, (__int64)size);
#elif defined (R_OS_UNIX)
  res = ftruncate (fd, size);
#endif
#else
  (void) fd;
  (void) size;
  res = -1;
#endif

  return res == 0;
}

#ifdef RLIB_HAVE_FILES
#if defined (R_OS_WIN32)
static int
fsync (int fd)
{
  HANDLE h = (HANDLE) _get_osfhandle (fd);

  if (h != INVALID_HANDLE_VALUE) {
    if (FlushFileBuffers (h)) {
      return 0;
    } else {
      switch (GetLastError ()) {
        case ERROR_ACCESS_DENIED: /* This is OK */
          return 0;
        case ERROR_INVALID_HANDLE:
          errno = EINVAL;
          break;
        default:
          errno = EIO;
        }
    }
  } else {
    errno = EBADF;
  }

  return -1;
}
#endif
#endif

rboolean
r_fd_flush (int fd)
{
#if defined (RLIB_HAVE_FILES)
  return fsync (fd) == 0;
#else
  (void) fd;
  return FALSE;
#endif
}

#ifdef R_OS_UNIX
rboolean
r_fd_unix_set_nonblocking (int fd, rboolean set)
{
  int res;

#ifdef HAVE_IOCTL
  do {
    res = ioctl (fd, FIONBIO, &set);
  } while (res < 0 && errno == EINTR);
  if (res == 0)
    return TRUE;
#endif

#ifdef HAVE_FCNTL
  do {
    res = fcntl (fd, F_GETFL);
  } while (res < 0 && errno == EINTR);

  if (!!(res & O_NONBLOCK) != !!set)
    return 0;

  if (set)
    res |= O_NONBLOCK;
  else
    res &= ~O_NONBLOCK;

  do {
    res = fcntl (fd, F_SETFL, res);
  } while (res < 0 && errno == EINTR);
  if (res == 0)
    return TRUE;
#endif

  return FALSE;
}

rboolean
r_fd_unix_set_cloexec (int fd, rboolean set)
{
  int res;

#ifdef HAVE_IOCTL
  do {
    res = ioctl (fd, set ? FIOCLEX : FIONCLEX);
  } while (res < 0 && errno == EINTR);
  if (res == 0)
    return TRUE;
#endif

#ifdef HAVE_FCNTL
  do {
    res = fcntl (fd, F_GETFL);
  } while (res < 0 && errno == EINTR);

  if (!!(res & FD_CLOEXEC) != !!set)
    return 0;

  if (set)
    res |= FD_CLOEXEC;
  else
    res &= ~FD_CLOEXEC;

  do {
    res = fcntl (fd, F_SETFL, res);
  } while (res < 0 && errno == EINTR);
  if (res == 0)
    return TRUE;
#endif

  return FALSE;
}
#endif

