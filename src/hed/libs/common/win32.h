// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_WIN32_H__
#define __ARC_WIN32_H__
#define NOGDI
#define WINVER 0x0501 /* we support XP or higher */
#define WIN32_LEAN_AND_MEAN

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
#ifndef HAVE_MKSTEMP
#ifdef HAVE_MKTEMP
inline int mkstemp(char *pattern) {
   return mktemp(pattern) != '\0';
};
#endif
#endif
//#define mkdir(x, y) mkdir((x))
//#define lstat stat
// no windows functions
#define chown(x, y, z) (0)
#define lchown(x, y, z) (0)
#define fchown(x, y, z) (0)
#define symlink(x, y) (-1)
//#define link(x, y) (-1)
//#define readlink(x, y, z) (-1)
#define getuid() (0)
#define getgid() (0)

inline ssize_t readlink(const char *path, char *buf, size_t bufsiz) {
    return -1;
};

#if defined(__cplusplus)
inline int mkdir(const char *pathname, mode_t mode) {
    return ::mkdir(pathname);
}
#endif

inline int link(const char *oldpath, const char *newpath) {
    return -1;
};

#if defined(__cplusplus)
#include <sys/stat.h>
inline int lstat(const char *path, struct stat *buf) {
    return ::stat(path,buf);
};
#endif

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

#endif
