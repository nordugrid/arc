#ifndef __ARC_HOSTNAMERESOLVER_H__
#define __ARC_HOSTNAMERESOLVER_H__

#include <string>
#include <list>
#include <mutex>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace Arc {

  class Run;

  /// Defines interface for accessing filesystems.
  /** This class performs host name respolution through a proxy executable.
    \ingroup common
    \headerfile HostnameResolver.h arc/HostnameResolver.h
  */
  class HostnameResolver {
    friend class HostnameResolverContainer;
  public:

   class SockAddr {
   friend class HostnameResolver;
   public:
     SockAddr();
     SockAddr(SockAddr const& other);
     SockAddr& operator=(SockAddr const& other) = delete;
     ~SockAddr();
     int Family() const { return family; }
     const sockaddr* Addr() const { return addr; }
     socklen_t Length() const { return length; }
   private:
     int family;
     socklen_t length;
     sockaddr *addr;
   };

    /// Resolve provides host name using resolver proxy (if valid), returning true for success.
    static bool Resolve(std::string const& node, std::string const& service, bool local, std::list<SockAddr>& addrs);
    /// Special method for using in unit tests.
    static void testtune();

  private:
    /// New HostnameResolver object.
    HostnameResolver();
    /// Shuts down any spawned executable.
    ~HostnameResolver();
    /// Check that the resolver is still valid and that communication with resolver proxy works.
    bool Ping();
    /// Make sure it's running or delete it and make it null.
    void CheckRunningLocked();
    /// Workhorse for name resolution.
    int DoResolve(std::string const& node, std::string const& service, bool local, std::list<SockAddr>& addrs);
    /// Get error code of last operation on resolver proxy.  Every proxy operation resets errno.
    int Errno() { return errno_; };

  private:
    // The lock protects hostname_resolver_ and errno_.  hostname_resolver_ is initialized
    // by the constructor but may be set to null (and its value recycled) at any failure.
    std::mutex lock_;
    Run* hostname_resolver_;
    int errno_;

  public:
    /// Internal struct used for communication between processes.
    typedef struct {
      unsigned int size;
      unsigned int cmd;
    } header_t;
  };

  /// Container for shared HostnameResolver objects.
  /** HostnameResolverContainer maintains a pool of executables and can be used to
      reduce the overhead in creating and destroying executables when using
      HostnameResolver.
      \ingroup common
      \headerfile HostnameResolver.h arc/HostnameResolver.h */
  class HostnameResolverContainer {
    friend class HostnameResolver;

  public:
    /// Creates container with number of stored objects between minval and maxval.
    HostnameResolverContainer(unsigned int minval, unsigned int maxval);
    /// Destroys container and all stored objects.
    ~HostnameResolverContainer();
    /// Get object from container.
    /** Object either is taken from stored ones or new one created.
        Acquired object looses its connection to container and
        can be safely destroyed or returned into other container. */

  private:
    HostnameResolver* Acquire();
    /// Returns object into container.
    /** It can be any object - taken from another container or created using
        new. */
    void Release(HostnameResolver* hr);

  private:
    HostnameResolver* MaybePopFront();

    // The lock protects min_, max_, and hrs_.
    std::mutex lock_;
    unsigned int min_;
    unsigned int max_;
    std::list<HostnameResolver*> hrs_;
    void KeepRangeLocked();
  };

} // namespace Arc

#endif // __ARC_HOSTNAMERESOLVER_H__

