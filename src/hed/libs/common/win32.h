// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_WIN32_H__
#define __ARC_WIN32_H__
#define NOGDI
#define WINVER 0x0501 /* we support XP or higher */
#define WIN32_LEAN_AND_MEAN

#include <string>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#undef USE_WINSOCK
#define USE_WINSOCK 2
#include <io.h>
#include <winsock2.h>

#define SIGPIPE 13
#define SIGTTIN 21
#define SIGTTOU 22
#define sleep(x) Sleep((x) * 1000)
#define mkdir(x, y) mkdir((x))
// no windows functions
#define chown(x, y, z) (0)
#define lchown(x, y, z) (0)
#define fchown(x, y, z) (0)
#define chmod(x, y) (0)
#define symlink(x, y) (0)

#define _FOPEN    (-1)     /* from sys/file.h, kernel use only */
#define _FREAD    0x0001  /* read enabled */
#define _FWRITE   0x0002  /* write enabled */
#define _FAPPEND  0x0008  /* append (writes guaranteed at the end) */
#define _FMARK    0x0010  /* internal; mark during gc() */
#define _FDEFER   0x0020  /* internal; defer for next gc pass */
#define _FASYNC   0x0040  /* signal pgrp when data ready */
#define _FSHLOCK  0x0080  /* BSD flock() shared lock present */
#define _FEXLOCK  0x0100  /* BSD flock() exclusive lock present */
#define _FCREAT   0x0200  /* open with file create */
#define _FTRUNC   0x0400  /* open with truncation */
#define _FEXCL    0x0800  /* error on open if file exists */
#define _FNBIO    0x1000  /* non blocking I/O (sys5 style) */
#define _FSYNC    0x2000  /* do all writes synchronously */
#define _FNONBLOCK 0x4000 /* non blocking I/O (POSIX style) */
#define _FNDELAY  _FNONBLOCK /* non blocking I/O (4.2 style) */
#define _FNOCTTY  0x8000   /* don't assign a ctty on this open */
 
#define O_ACCMODE (O_RDONLY|O_WRONLY|O_RDWR)

/*
 * Flag values for open(2) and fcntl(2)
 * The kernel adds 1 to the open modes to turn it into some
 * combination of FREAD and FWRITE.
 *
 * These macros are from msys: sys\fcntl.h
 */

#define O_RDONLY  0   /* +1 == FREAD */
#define O_WRONLY  1   /* +1 == FWRITE */
#define O_RDWR    2   /* +1 == FREAD|FWRITE */
#define O_APPEND  _FAPPEND
#define O_CREAT   _FCREAT
#define O_TRUNC   _FTRUNC
#define O_EXCL    _FEXCL
/*..O_SYNC    _FSYNC    not posix, defined below */
/*  O_NDELAY  _FNDELAY  set in include/fcntl.h */
/*  O_NDELAY  _FNBIO    set in 5include/fcntl.h */
#define O_NONBLOCK _FNONBLOCK
#define O_NOCTTY   _FNOCTTY
/* For machines which care - */

#define _FBINARY        0x10000
#define _FTEXT          0x20000
#define _FNOINHERIT     0x40000

#define O_BINARY    _FBINARY
#define O_TEXT      _FTEXT
#define O_NOINHERIT _FNOINHERIT


// pwd.h does not exist on windows
struct passwd {
  char *pw_name;
  char *pw_passwd;
  int pw_uid;
  int pw_gid;
  char *pw_age;
  char *pw_comment;
  char *pw_gecos;
  char *pw_dir;
  char *pw_shell;
};

std::string GetOsErrorMessage(void);
#endif
