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

/* Windows redefines CreateDirectory in winbase.h */
#ifdef CreateDirectory
#undef CreateDirectory
#endif

#define SIGPIPE 13
#define SIGTTIN 21
#define SIGTTOU 22
#define sleep(x) Sleep((x) * 1000)
inline int usleep(int x) { Sleep((x + 999) / 1000); return 0; }
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

// Socket errors are prefixed WSA in winsock2.h
#define EWOULDBLOCK     WSAEWOULDBLOCK     /* Operation would block */
#define EINPROGRESS     WSAEINPROGRESS     /* Operation now in progress */
#define EALREADY        WSAEALREADY        /* Operation already in progress */
#define ENOTSOCK        WSAENOTSOCK        /* Socket operation on non-socket */
#define EDESTADDRREQ    WSAEDESTADDRREQ    /* Destination address required */
#define EMSGSIZE        WSAEMSGSIZE        /* Message too long */
#define EPROTOTYPE      WSAEPROTOTYPE      /* Protocol wrong type for socket */
#define ENOPROTOOPT     WSAENOPROTOOPT     /* Protocol not available */
#define EPROTONOSUPPORT WSAEPROTONOSUPPORT /* Protocol not supported */
#define ESOCKTNOSUPPORT WSAESOCKTNOSUPPORT /* Socket type not supported */
#define EOPNOTSUPP      WSAEOPNOTSUPP      /* Operation not supported on transport endpoint */
#define EPFNOSUPPORT    WSAEPFNOSUPPORT    /* Protocol family not supported */
#define EAFNOSUPPORT    WSAEAFNOSUPPORT    /* Address family not supported by protocol */
#define EADDRINUSE      WSAEADDRINUSE      /* Address already in use */
#define EADDRNOTAVAIL   WSAEADDRNOTAVAIL   /* Cannot assign requested address */
#define ENETDOWN        WSAENETDOWN        /* Network is down */
#define ENETUNREACH     WSAENETUNREACH     /* Network is unreachable */
#define ENETRESET       WSAENETRESET       /* Network dropped connection because of reset */
#define ECONNABORTED    WSAECONNABORTED    /* Software caused connection abort */
#define ECONNRESET      WSAECONNRESET      /* Connection reset by peer */
#define ENOBUFS         WSAENOBUFS         /* No buffer space available */
#define EISCONN         WSAEISCONN         /* Transport endpoint is already connected */
#define ENOTCONN        WSAENOTCONN        /* Transport endpoint is not connected */
#define ESHUTDOWN       WSAESHUTDOWN       /* Cannot send after transport endpoint shutdown */
#define ETOOMANYREFS    WSAETOOMANYREFS    /* Too many references: cannot splice */
#define ETIMEDOUT       WSAETIMEDOUT       /* Connection timed out */
#define ECONNREFUSED    WSAECONNREFUSED    /* Connection refused */
#define ELOOP           WSAELOOP           /* Too many symbolic links encountered */
#define ENAMETOOLONG    WSAENAMETOOLONG    /* File name too long */
#define EHOSTDOWN       WSAEHOSTDOWN       /* Host is down */
#define EHOSTUNREACH    WSAEHOSTUNREACH    /* No route to host */
#define EUSERS          WSAEUSERS          /* Too many users */
#define EDQUOT          WSAEDQUOT          /* Quota exceeded */
#define ESTALE          WSAESTALE          /* Stale NFS file handle */
#define EREMOTE         WSAEREMOTE         /* Object is remote */

inline ssize_t readlink(const char *path, char *buf, size_t bufsiz) {
    return -1;
};

#if defined(__cplusplus)
#include <sys/stat.h>
#include <sys/types.h>
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
