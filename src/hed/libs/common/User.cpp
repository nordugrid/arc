// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <arc/StringConv.h>
#include <arc/Utils.h>
#include "User.h"

namespace Arc {

SimpleCondition UserSwitch::suid_lock;
int UserSwitch::suid_count = 0;
int UserSwitch::suid_uid_orig = -1;
int UserSwitch::suid_gid_orig = -1;

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

  bool User::set(const struct passwd *pwd_p) {
    if (pwd_p == NULL)
      return false;
    name = pwd_p->pw_name;
    uid = pwd_p->pw_uid;
    gid = pwd_p->pw_gid;
    home = GetEnv("HOME");
    // don't use HOME if user is different from current user
    if (home.empty() || uid != get_user_id()) home = pwd_p->pw_dir;
    return true;
  }

  User::User(void): valid(false) {
    uid = get_user_id();
    gid = get_group_id();
    struct passwd pwd;
    char pwdbuf[2048];
    struct passwd *pwd_p;
    if (getpwuid_r(uid, &pwd, pwdbuf, sizeof(pwdbuf), &pwd_p) != 0) return;
    if (!set(pwd_p)) return;
    valid = true;
  }

  // Unix implementation
  User::User(const std::string& name, const std::string& group): valid(false) {
    this->name = name;
    uid = -1;
    gid = -1;
    struct passwd pwd;
    char pwdbuf[2048];
    struct passwd *pwd_p;
    if (getpwnam_r(name.c_str(), &pwd, pwdbuf, sizeof(pwdbuf), &pwd_p) != 0) return;
    if (!set(pwd_p)) return;
    if (!group.empty()) {
      // override gid with given group
      struct group* gr = getgrnam(group.c_str());
      if (!gr) return;
      gid = gr->gr_gid;
    }
    valid = true;
  }

  User::User(int uid, int gid): valid(false) {
    this->uid = uid;
    this->gid = gid;
    struct passwd pwd;
    char pwdbuf[2048];
    struct passwd *pwd_p;
    if (getpwuid_r(uid, &pwd, pwdbuf, sizeof(pwdbuf), &pwd_p) != 0) return;
    if (!set(pwd_p)) return;
    // override what is found in passwd with given gid
    if (gid != -1) this->gid = gid;
    valid = true;
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

  bool User::SwitchUser() const {
    // set proper umask
    umask(0077);
    if (getuid() != 0 && uid != 0 && getuid() != uid) return false;
    if (uid != 0) {
      setgid(gid); // this is not an error if group failed, not a big deal
      if (setuid(uid) != 0) return false;
    }
    return true;
  }

  UserSwitch::UserSwitch(int uid,int gid):valid(false) {
    suid_lock.lock(); // locking while analyzing situation
    while(suid_count > 0) {
      // Already locked
      // Check if request is compatible with current situation
      bool need_switch = false;
      if(uid) {
        if(uid != geteuid()) need_switch = true;
      } else {
        if(suid_uid_orig != geteuid()) need_switch = true;
      };
      if(gid) {
        if(gid != getegid()) need_switch = true;
      } else {
        if(suid_gid_orig != getegid()) need_switch = true;
      };
      if(!need_switch) {
        // can join existing switch
        ++suid_count;
        valid = true;
        suid_lock.unlock();
        return;
      };
      // must wait till released
      suid_lock.wait_nonblock();
      // and then re-check
    };
    // First request - apply
    // Make sure current state is properly set
    if(suid_uid_orig == -1) {
      suid_uid_orig = geteuid();
      suid_gid_orig = getegid();
    };
    // Try to apply new state
    if(gid && (suid_gid_orig != gid)) {
      if(setegid(gid) == -1) {
        suid_lock.unlock();
        return;
      };
    };
    if(uid && (suid_uid_orig != uid)) {
      if(seteuid(uid) == -1) {
        if(suid_gid_orig != gid) setegid(suid_gid_orig);
        suid_lock.unlock();
        return;
      };
    };
    // switch done - record that
    ++suid_count;
    valid = true;
    suid_lock.unlock();
  } 

  UserSwitch::~UserSwitch(void) {
    if(valid) {
      suid_lock.lock();
      --suid_count;
      if(suid_count <= 0) {
        if(suid_uid_orig != geteuid()) seteuid(suid_uid_orig);
        if(suid_gid_orig != getegid()) setegid(suid_gid_orig);
        suid_lock.signal_nonblock();
      };
      suid_lock.unlock();
    };
  }

  void UserSwitch::resetPostFork(void) {
    valid = false;
  }

} // namespace Arc
