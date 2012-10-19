// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_USER_H__
#define __ARC_USER_H__

#include <string>

struct passwd;

namespace Arc {

  /// Platform independent representation of system user
  class User {
  private:
    /* local name, home directory, uid and gid of this user */
    std::string name;
    std::string home;
    int uid;
    int gid;
    bool valid;
    bool set(const struct passwd*);

  public:
    /// Construct user from current process owner
    User();
    /// Construct user from username and optional group name. If group is not
    /// specified it is determined automatically.
    User(const std::string& name, const std::string& group="");
    /// Construct user from uid and optional gid. If gid is not specified it
    /// is determined automatically.
    User(int uid, int gid=-1);
    /// Returns true if this is a valid user
    operator bool() const {
      return valid;
    }
    /// Returns true is this is not a valid user
    bool operator !() const {
      return !valid;
    }
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
    bool operator==(const std::string& n) {
      return (n == name);
    }
    int check_file_access(const std::string& path, int flags) const;
    /* Run command as behalf of this user */
    bool RunAs(std::string cmd);
    /// Switch user in single-threaded environment or right after fork. Calls
    /// setuid() and setgid() with this User's values. This can be used in the
    /// initializer of Arc::Run to switch the owner of a child process just
    /// after fork(). In a multi-threaded environment UserSwitch must be used
    /// instead. Returns true if switch succeeded.
    bool SwitchUser() const;
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
