#ifndef __ARC_WIN32_H__
#define __ARC_WIN32_H_
#define NOGDI
#define WINVER 0x0501 /* we support XP or higher */

#include <string> 
#include <objbase.h>
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

std::string GetOsErrorMessage(void);
#endif
