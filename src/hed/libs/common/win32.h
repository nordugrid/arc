#ifndef __ARC_WIN32_H__
#define __ARC_WIN32_H_
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
#define sleep(x) Sleep((x)*1000)
#define mkdir(x,y) mkdir((x))
// no windows functions
#define chown(x,y,z) (0)
#define	lchown(x,y,z) (0) 
#define fchown(x,y,z) (0)
#define chmod(x,y) (0)
#define symlink(x,y) (0)

// pwd.h does not exist on windows
struct passwd{
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
