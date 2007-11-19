#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef WIN32
#include <pwd.h>
#include <sys/types.h>
#endif

#include "User.h"

namespace Arc
{

#ifndef WIN32
// Unix implementation
User::User(std::string name)
{
    struct passwd *pwd = getpwnam(name.c_str());
    if (pwd == NULL) {
        return;
    }
    name = name;
    home = pwd->pw_dir;
    uid = pwd->pw_uid;
    gid = pwd->pw_gid;
}

User::User(int uid)
{
    struct passwd *pwd = getpwuid((uid_t)uid);
    if (pwd == NULL) {
        return;
    }
    name = pwd->pw_name;
    home = pwd->pw_dir;
    uid = uid;
    gid = pwd->pw_gid;
}

bool User::RunAs(std::string cmd)
{
    // XXX NOP
    return false;
}

#else

// Win32 implementation

User::User(std::string name)
{
    // XXX NOP
}

User::User(int uid)
{
    // XXX NOP
}

bool User::RunAs(std::string cmd)
{
    // XXX NOP
    return false;
}

#endif
}; // namespace Arc
