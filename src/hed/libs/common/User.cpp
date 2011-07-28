// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#ifndef WIN32
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#else // WIN32
#include <glibmm/miscutils.h>
#endif

#include <arc/Thread.h>
#include <arc/StringConv.h>
#include <arc/Utils.h>
#include "User.h"

namespace Arc {

static Glib::Mutex suid_lock;

#ifndef WIN32
  static uid_t get_user_id(void) {
    uid_t user_id = getuid();
    if (user_id != 0)
      return user_id;
    std::string user_s = GetEnv("USER_ID");
    if (user_s.empty())
      return 0;
    stringto(user_s, user_id);
    return user_id;
  }

  static uid_t get_group_id(void) {
    return getgid();
  }

  void User::set(const struct passwd *pwd_p) {
    if (pwd_p == NULL)
      return;
    home = GetEnv("HOME");
    name = pwd_p->pw_name;
    if (home.empty())
      home = pwd_p->pw_dir;
    uid = pwd_p->pw_uid;
    gid = pwd_p->pw_gid;
  }

  User::User(void) {
    uid = get_user_id();
    gid = get_group_id();
    struct passwd pwd;
    char pwdbuf[2048];
    struct passwd *pwd_p;
    getpwuid_r(uid, &pwd, pwdbuf, sizeof(pwdbuf), &pwd_p);
    set(pwd_p);
  }

  // Unix implementation
  User::User(const std::string& name) {
    this->name = name;
    struct passwd pwd;
    char pwdbuf[2048];
    struct passwd *pwd_p;
    getpwnam_r(name.c_str(), &pwd, pwdbuf, sizeof(pwdbuf), &pwd_p);
    set(pwd_p);
  }

  User::User(int uid) {
    this->uid = uid;
    this->gid = -1;
    struct passwd pwd;
    char pwdbuf[2048];
    struct passwd *pwd_p;
    getpwuid_r(uid, &pwd, pwdbuf, sizeof(pwdbuf), &pwd_p);
    set(pwd_p);
  }

  bool User::RunAs(std::string) {
    // XXX NOP
    return false;
  }

  int User::check_file_access(const std::string& path, int flags) const {
    struct stat st;
    mode_t m;
    char **grmem;

    flags &= O_RDWR | O_RDONLY | O_WRONLY;
    if ((flags != O_RDWR) && (flags != O_RDONLY) && (flags != O_WRONLY))
      return -1;
    if (getuid() != 0) { /* not root - just try to open */
      int h;
      if ((h = open(path.c_str(), flags)) == -1)
        return -1;
      close(h);
      return 0;
    }
    if (uid == 0)
      return 0;
    /* check for file */
    if (stat(path.c_str(), &st) != 0)
      return -1;
    if (!S_ISREG(st.st_mode))
      return -1;
    m = 0;
    if (st.st_uid == uid)
      m |= st.st_mode & (S_IRUSR | S_IWUSR);
    if (st.st_gid == gid)
      m |= st.st_mode & (S_IRGRP | S_IWGRP);
    else {
      char grbuf[2048];
      struct group grp;
      struct group *grp_p = NULL;
      char pwdbuf[2048];
      struct passwd pwd;
      struct passwd *pwd_p = NULL;
      getpwuid_r(uid, &pwd, pwdbuf, sizeof(pwdbuf), &pwd_p);
      getgrgid_r(st.st_gid, &grp, grbuf, sizeof(grbuf), &grp_p);
      if ((grp_p != NULL) && (pwd_p != NULL))
        for (grmem = grp_p->gr_mem; (*grmem) != NULL; grmem++)
          if (strcmp(*grmem, pwd_p->pw_name) == 0) {
            m |= st.st_mode & (S_IRGRP | S_IWGRP);
            break;
          }
    }
    m |= st.st_mode & (S_IROTH | S_IWOTH);
    if (flags == O_RDWR) {
      if (((m & (S_IRUSR | S_IRGRP | S_IROTH)) == 0) ||
          ((m & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0))
        return 1;
    }
    else if (flags == O_RDONLY) {
      if ((m & (S_IRUSR | S_IRGRP | S_IROTH)) == 0)
        return 1;
    }
    else if (flags == O_WRONLY) {
      if ((m & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0)
        return 1;
    }
    else
      return -1; /* check if all directories allow to read - not implemented yet */

    return 0;
  }


#else
  // Win32 implementation

  static uid_t get_user_id(void) {
    return 0; // TODO: The user id is not used on windows for file permissions etc.
  }

  static uid_t get_group_id(void) {
    return 0; // TODO: The user id is not used on windows for file permissions etc.
  }

  void User::set(const struct passwd *pwd_p) {
    if (pwd_p == NULL)
      return;
    name = pwd_p->pw_name;
    home = pwd_p->pw_dir;
    uid = pwd_p->pw_uid;
    gid = pwd_p->pw_gid;
  }

  User::User(void) {
    int uid = get_user_id();
    int gid = get_group_id();
    bool found;

    struct passwd pwd_p;

    std::string name = Glib::getenv("USERNAME", found);
    if (!found)
      name = "";
    std::string home = g_get_user_config_dir();

    pwd_p.pw_name = const_cast<char*>(name.c_str());
    pwd_p.pw_uid = uid;
    pwd_p.pw_gid = gid;
    pwd_p.pw_dir = const_cast<char*>(home.c_str());

    set(&pwd_p);
  }

  User::User(const std::string& name) {
    this->name = name;
    int uid = get_user_id();
    int gid = get_group_id();

    struct passwd pwd_p;

    std::string home = g_get_user_config_dir();

    pwd_p.pw_name = const_cast<char*>(name.c_str());
    pwd_p.pw_uid = uid;
    pwd_p.pw_gid = gid;
    pwd_p.pw_dir = const_cast<char*>(home.c_str());

    set(&pwd_p);
  }

  User::User(int uid) {
    this->uid = uid;
    this->gid = 0;

    bool found;

    struct passwd pwd_p;

    std::string name = Glib::getenv("USERNAME", found);
    if (!found)
      name = "";
    std::string home = g_get_user_config_dir();

    pwd_p.pw_name = const_cast<char*>(name.c_str());
    pwd_p.pw_uid = uid;
    pwd_p.pw_gid = gid;
    pwd_p.pw_dir = const_cast<char*>(home.c_str());

    set(&pwd_p);
  }
  bool User::RunAs(std::string cmd) {
    // XXX NOP
    return false;
  }

  int User::check_file_access(const std::string& path, int flags) const {
    // XXX NOP
    return 0;
  }
#endif

#ifndef WIN32
  UserSwitch::UserSwitch(int uid,int gid):old_uid(0),old_gid(0),valid(false) {
    suid_lock.lock();
    old_gid = getegid();
    old_uid = geteuid();
    if((uid == 0) && (gid == 0)) {
      valid=true;
      return;
    };
    if(gid) {
      if(old_gid != gid) {
        if(setegid(gid) == -1) {
          suid_lock.unlock();
          return;
        };
      };
    };
    if(uid) {
      if(old_uid != uid) {
        if(seteuid(uid) == -1) {
          if(old_gid != gid) setegid(old_gid);
          suid_lock.unlock();
          return;
        };
      };
    };
    valid=true;
  }

  UserSwitch::~UserSwitch(void) {
    if(valid) {
      if(old_uid != geteuid()) seteuid(old_uid);
      if(old_gid != getegid()) setegid(old_gid);
      suid_lock.unlock();
    };
  }
#else
  UserSwitch::UserSwitch(int uid,int gid):old_uid(0),old_gid(0),valid(false) {
  }

  UserSwitch::~UserSwitch(void) {
  }
#endif

} // namespace Arc
