// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_USER_H__
#define __ARC_USER_H__

#include <string>
#include <arc/Thread.h>

struct passwd;

namespace Arc {

  /// Platform independent representation of system user
  /** \ingroup common
   *  \headerfile User.h arc/User.h */
  class User {
  private:
    // local name, home directory, uid and gid of this user
    std::string name;
    std::string home;
    int uid;
    int gid;
    bool valid;
    bool set(const struct passwd*);

  public:
    /// Construct user from current process owner.
    User();
    /// Construct user from username and optional group name.
    /** If group is not specified it is determined automatically. */
    User(const std::string& name, const std::string& group="");
    /// Construct user from uid and optional gid.
    /** If gid is not specified it is determined automatically. */
    User(int uid, int gid=-1);
    /// Returns true if this is a valid user.
    operator bool() const {
      return valid;
    }
    /// Returns true is this is not a valid user.
    bool operator !() const {
      return !valid;
    }
    /// Returns the name of this user.
    const std::string& Name(void) const {
      return name;
    }
    /// Returns the path to the user's home directory.
    const std::string& Home(void) const {
      return home;
    }
    /// Returns the user's uid.
    int get_uid(void) const {
      return (int)uid;
    }
    /// Returns the user's gid.
    int get_gid(void) const {
      return (int)gid;
    }
    /// Returns true if this User's name is the same as n.
    bool operator==(const std::string& n) {
      return (n == name);
    }
    /// Check if this User has the rights specified by flags on the given path.
    /** \return 0 if User has the rights */
    int check_file_access(const std::string& path, int flags) const;
    /// Change the owner of the current process.
    /** Internally this method calls setuid() and setgid() with this User's
       values. It can be used in the initializer of Arc::Run to switch the
       owner of a child process just after fork(). To temporarily change the
       owner of a thread in a multi-threaded environment UserSwitch should be
       used instead.
       \return true if switch succeeded. */
    bool SwitchUser() const;
  }; // class User

  /// Class for temporary switching of user id.
  /** If this class is created, the user identity is switched to the provided
     uid and gid. Due to an internal lock there will be only one valid
     instance of this class. Any attempt to create another instance 
     will block until the first one is destroyed.
      If uid and gid are set to 0 then the user identity is not switched,
     but the lock is applied anyway.
      The lock has a dual purpose. The first and most important is to
     protect communication with the underlying operating system which may
     depend on user identity. For that it is advisable for code which
     talks to the operating system to acquire a valid instance of this class.
     Care must be taken not to hold that instance too long as
     that may block other code in a multithreaded environment.
      The other purpose of this lock is to provide a workaround for a glibc bug
     in __nptl_setxid. This bug causes lockup of seteuid() function
     if racing with fork. To avoid this problem the lock mentioned above
     is used by the Run class while spawning a new process.
     \ingroup common
     \headerfile User.h arc/User.h  */
  class UserSwitch {
  private:
    static SimpleCondition suid_lock;
    static int suid_count;
    static int suid_uid_orig;
    static int suid_gid_orig;
    bool valid;
  public:
    /// Switch uid and gid.
    UserSwitch(int uid,int gid);
    /// Switch back to old uid and gid and release lock on this class.
    ~UserSwitch(void);
    /// Returns true if switching user succeeded.
    operator bool(void) { return valid; };
  }; // class UserSwitch


} // namespace Arc

#endif
