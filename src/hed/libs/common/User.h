// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_USER_H__
#define __ARC_USER_H__
/** Platform independent represnetation of system user */
#include <string>

struct passwd;

namespace Arc {

  class User {
  private:
    /* local name, home directory, uid and gid of this user */
    std::string name;
    std::string home;
    int uid;
    int gid;
    void set(struct passwd*);

  public:
    // get current user
    User();
    User(const std::string name);
    User(int uid);
    const std::string& Name(void) const {
      return name;
    }
    const std::string& Home(void) const {
      return home;
    }
    int get_uid(void) const {
      return (int)uid;
    }
    int get_gid(void) const {
      return (int)gid;
    }
    bool operator==(const std::string n) {
      return (n == name);
    }
    int check_file_access(const std::string& path, int flags);
    /* Run command as behalf of this user */
    bool RunAs(std::string cmd);
  }; // class User

  // For quick switching of user id.
  /** If this class is created user identity is switched to provided
     uid and gid. Due to internal lock there will be only one valid
     instance of this class. Any attempt to create another instance 
     will block till first one is destroyed.
      If uid and gid are set to 0 then user identity is not switched.
     But lock is applied anyway.
      The lock has dual purpose. First and most important is to 
     protect communication with underlying operating system which may
     depend on user identity. For that it is advisable for code which
     talks to operating system to acquire valid instance of this class.
     Care must be taken for not to hold that instance too long cause
     that may block other code in multithreaded envoronment. 
      Other purpose of this lock is to provide workaround for glibc bug
     in __nptl_setxid. That bug causes lockup of seteuid() function
     if racing with fork. To avoid this problem the lock mentioned above
     is used by Run class while spawning new process. */
  class UserSwitch {
  private:
    int old_uid;
    int old_gid;
    bool valid;
  public:
    UserSwitch(int uid,int gid);
    ~UserSwitch(void);
    operator bool(void) { return valid; };
  }; // class UserSwitch


} // namespace Arc

#endif
