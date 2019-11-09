#ifndef __ARC_HOSTNAMERESOLVER_H__
#define __ARC_HOSTNAMERESOLVER_H__

#include <string>
#include <list>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <glibmm.h>

namespace Arc {

  class Run;

  /// Defines interface for accessing filesystems.
  /** This class performs host name respolution through a proxy executable.
    \ingroup common
    \headerfile HostnameResolver.h arc/HostnameResolver.h
  */
  class HostnameResolver {
  public:


   class SockAddr {
   friend class HostnameResolver;
   public:
     SockAddr();
     SockAddr(SockAddr const& other);
     SockAddr& operator=(SockAddr const& other);
     ~SockAddr();
     int Family() const { return family; }
     sockaddr const* Addr() const { return addr; }
     socklen_t Length() const { return length; }
   private:
     int family;
     socklen_t length;
     sockaddr *addr; 
   };

    /// New HostnameResolver object.
    HostnameResolver(void);
    /// Shuts down any spawned executable.
    ~HostnameResolver(void);
    /// Constructor which takes already existing object from global cache
    static HostnameResolver* Acquire(void);
    /// Destructor which returns object into global cache
    static void Release(HostnameResolver* fa);
    /// Check if communication with proxy works
    bool ping(void);
    /// Performs resolution of provided host name.
    int hr_resolve(std::string const& node, std::string const& service, bool local, std::list<SockAddr>& addrs);
    /// Get errno of last operation. Every operation resets errno.
    int geterrno() { return errno_; };
    /// Returns true if this instance is in useful condition
    operator bool(void) { return (hostname_resolver_ != NULL); };
    /// Returns true if this instance is not in useful condition
    bool operator!(void) { return (hostname_resolver_ == NULL); };
    /// Special method for using in unit tests.
    static void testtune(void);
  private:
    Glib::Mutex lock_;
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
  public:
    /// Creates container with number of stored objects between minval and maxval.
    HostnameResolverContainer(unsigned int minval, unsigned int maxval);
    /// Creates container with number of stored objects between 1 and 10.
    HostnameResolverContainer(void);
    /// Destroys container and all stored objects.
    ~HostnameResolverContainer(void);
    /// Get object from container.
    /** Object either is taken from stored ones or new one created.
        Acquired object looses its connection to container and
        can be safely destroyed or returned into other container. */
    HostnameResolver* Acquire(void);
    /// Returns object into container.
    /** It can be any object - taken from another container or created using
        new. */
    void Release(HostnameResolver* hr);
    /// Adjust minimal number of stored objects.
    void SetMin(unsigned int val);
    /// Adjust maximal number of stored objects.
    void SetMax(unsigned int val);
  private:
    Glib::Mutex lock_;
    unsigned int min_;
    unsigned int max_;
    std::list<HostnameResolver*> hrs_;
    void KeepRange(void);
  };

} // namespace Arc 

#endif // __ARC_HOSTNAMERESOLVER_H__

