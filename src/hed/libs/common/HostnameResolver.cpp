#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <list>
#include <iostream>
#include <cstring>

#include <glibmm/miscutils.h>

#include <arc/Run.h>
#include <arc/ArcLocation.h>

#include "hostname_resolver.h"

#include "HostnameResolver.h"

namespace Arc {

  static const int READ_TIMEOUT_MS = 60 * 1000;
  static const int WRITE_TIMEOUT_MS = 60 * 1000;
  static const int CACHE_TTL_S = 5 * 60;
  static const int PENDING_RESOLVER_WAIT_US = 200000;

  enum class ResolveStatus {
    // The caller has received the result and it is good
    Resolved,
    // The caller has received the result and it is a failure
    Failed,
    // Another thread is resolving; please wait and try again
    Pending,
    // This thread should perform resolution and register the result, whether
    // success or failure.
    YoureIt,
  };

  class HostnameResolverCache {
  private:
    struct CacheEntry {
      CacheEntry(const std::string& node, const std::string& service, bool local)
        : node(node)
        , service(service)
        , local(local)
        , expires(0)
        , status(ResolveStatus::Pending)
      {}
      std::string node;
      std::string service;
      bool local;
      std::list<HostnameResolver::SockAddr> addrs;
      time_t expires;
      ResolveStatus status;
    };

  public:
    HostnameResolverCache(int ttl_s);
    ~HostnameResolverCache();

    // Lookup (node,service,local) in the cache.  If found, copy addresses into addrs and return
    // Resolved.  If known not to be found (or if resolver crashed or timed out, or whatever),
    // returns Failed.  If a lookup is pending on another thread, returns Pending, in which case the
    // caller should retry after a while.  Otherwise, returns YoureIt and the thread must perform
    // resolution and must register the result with Resolved(), whether good or bad.
    ResolveStatus Resolve(std::string const& node, std::string const& service, bool local,
                          std::list<HostnameResolver::SockAddr>& addrs);

    // Register the (node,service,local) as resolved with given addresses; res is the result from
    // the DoResolve call (0 => success, !0 => failure).
    void Resolved(int res, std::string const& node, std::string const& service, bool local,
                  std::list<HostnameResolver::SockAddr> const& addrs);

    // Change settings.
    //
    // If not null, the callback will be called for some resolver operations, with these arguments:
    //   "lookup", const char* node, const char* service, const char* localOrRemote, time_t expires
    //      For a lookup operation that missed the cache and was not yet pending
    //
    //   "reap", const char* node, const char* service, const char* localOrRemote, time_t expires
    //      When a node is taken by GC
    //
    //   "gc", unsigned long elements_collected, unsigned long elements_remaining
    //      After a GC is run, even if no elements were reaped
    void testtune(int ttl_s, int gc_interval_s, void (*callback)(const char* action...));

  private:
    CacheEntry* FindLocked(std::string const& node, std::string const& service, bool local);
    void CollectLocked(time_t now);

  private:
    // Immutable after construction (but may be updated by testtune)
    int ttl_s_;
    int gc_interval_s_;
    void (*test_callback_)(const char* action, ...);

    // Lock protects contents_, next_gc_, and every object pointed to from contents_.
    std::mutex lock_;
    std::vector<CacheEntry*> contents_;
    time_t next_gc_;
  };

  static bool relativeResolverPath = false;
  static HostnameResolverContainer hrs_(0,100);
  static HostnameResolverCache rcache_(CACHE_TTL_S);

  static bool sread(Run& r, char* buf, size_t size) {
    while(size > 0) {
      int l = r.ReadStdout(READ_TIMEOUT_MS, buf, size);
      if(l <= 0) {
        return false;
      }
      size -= l;
      buf += l;
    };
    return true;
  }

  static bool swrite(Run& r, const char* buf, size_t size) {
    while(size > 0) {
      int l = r.WriteStdin(WRITE_TIMEOUT_MS, buf, size);
      if(l <= 0) {
        return false;
      }
      size -= l;
      buf += l;
    };
    return true;
  }

  static bool swrite_string(Run& r, const std::string& str) {
    int l = str.length();
    if(!swrite(r, reinterpret_cast<char*>(&l), sizeof(l))) {
      return false;
    }
    if(!swrite(r, str.c_str(), l)) {
      return false;
    }
    return true;
  }

  bool sread_field(HostnameResolver::header_t& header, Run& resolver, char* buf, size_t bufsiz) {
    if (header.size < bufsiz) {
      return false;
    }
    if (!sread(resolver, buf, bufsiz)) {
      return false;
    }
    header.size -= bufsiz;
    return true;
  }

  // Note this can return nullptr.
  static Run* start_resolver_subprocess() {
    std::list<std::string> argv;
    if(!relativeResolverPath) {
      argv.push_back(Arc::ArcLocation::Get()+G_DIR_SEPARATOR_S+PKGLIBSUBDIR+G_DIR_SEPARATOR_S+"arc-hostname-resolver");
    } else {
      argv.push_back(std::string("..")+G_DIR_SEPARATOR_S+"arc-hostname-resolver");
    }
    argv.push_back("0");
    argv.push_back("1");
    Run* hostname_resolver_ = new Run(argv);
    hostname_resolver_->KeepStdin(false);
    hostname_resolver_->KeepStdout(false);
    hostname_resolver_->KeepStderr(true);
    if(!hostname_resolver_->Start()) {
      delete hostname_resolver_;
      hostname_resolver_ = nullptr;
    }
    return hostname_resolver_;
  }

  HostnameResolver::SockAddr::SockAddr()
    : family(0)
    , length(0)
    , addr(nullptr)
  { }

  HostnameResolver::SockAddr::SockAddr(SockAddr const& other)
    : family(other.family)
    , length(other.length)
    , addr(reinterpret_cast<sockaddr*>(::operator new(other.length)))
  {
    std::memcpy(addr, other.addr, other.length);
  }

  HostnameResolver::SockAddr::~SockAddr() {
    ::operator delete(addr);
  }

  HostnameResolver::HostnameResolver()
    : hostname_resolver_(nullptr)
    , errno_(0)
  {
    hostname_resolver_ = start_resolver_subprocess();
  }

  HostnameResolver::~HostnameResolver() {
    delete hostname_resolver_;
    hostname_resolver_ = nullptr;
  }

  void HostnameResolverCache::testtune(int ttl_s, int gc_interval_s, void (*callback)(const char* ...)) {
    std::unique_lock<std::mutex> l(lock_);
    ttl_s_ = ttl_s;
    gc_interval_s_ = gc_interval_s;
    test_callback_ = callback;
    next_gc_ = 0;
    CollectLocked(::time(nullptr));
  }


  /*static*/
  bool HostnameResolver::Resolve(std::string const& node, std::string const& service, bool local,
                                 std::list<SockAddr>& addrs) {
    ResolveStatus s;
    while ((s = rcache_.Resolve(node, service, local, addrs)) == ResolveStatus::Pending) {
      usleep(PENDING_RESOLVER_WAIT_US);
    }
    if (s == ResolveStatus::Resolved) {
      return true;
    }
    if (s == ResolveStatus::Failed) {
      return false;
    }
    // assert(s == ResolveStatus::YoureIt)
    // We have to perform the lookup here.
    HostnameResolver* hr = hrs_.Acquire();
    int r = hr->DoResolve(node, service, local, addrs);
    hrs_.Release(hr);
    rcache_.Resolved(r, node, service, local, addrs);
    return r == 0;
  }

  void HostnameResolver::CheckRunningLocked() {
    // If it's dead then remove it.
    if (hostname_resolver_ != nullptr && !hostname_resolver_->Running()) {
      delete hostname_resolver_;
      hostname_resolver_ = nullptr;
    }
  }

  // The meaning of Ping() is that if it returns true then it has a working resolver subprocess, and
  // if it returns false then the subprocess will have been killed and removed, and errno_ will have
  // been set.  Ping() will not create a new process - its caller handles that logic.
  bool HostnameResolver::Ping() {
    std::unique_lock<std::mutex> mlock(lock_);
    errno_ = 0;

    CheckRunningLocked();
    if (hostname_resolver_ == nullptr) {
      goto failed;
    }

    {
      header_t cmd_header;
      cmd_header.cmd = CMD_PING;
      cmd_header.size = 0;
      if(!swrite(*hostname_resolver_, reinterpret_cast<char*>(&cmd_header), sizeof(cmd_header))) {
        goto failed;
      }
    }
    {
      header_t resp_header;
      if (!sread(*hostname_resolver_, reinterpret_cast<char*>(&resp_header), sizeof(resp_header))) {
        goto failed;
      }
      if (resp_header.cmd != CMD_PING || resp_header.size != 0) {
        goto failed;
      }
    }
    return true;

  failed:
    // It's unresponsive, so kill it and remove it.
    delete hostname_resolver_;
    hostname_resolver_ = nullptr;
    errno_ = -1;
    return false;
  }

  int HostnameResolver::DoResolve(std::string const& node, std::string const& service, bool local,
                                  std::list<SockAddr>& addrs) {
    std::unique_lock<std::mutex> mlock(lock_);
    errno_ = 0;

    unsigned command = local ? CMD_RESOLVE_TCP_LOCAL : CMD_RESOLVE_TCP_REMOTE;

    // Check if it's running or make sure it's null.
    CheckRunningLocked();

    // If it's missing then create it.
    if (hostname_resolver_ == nullptr) {
      hostname_resolver_ = start_resolver_subprocess();
      if (hostname_resolver_ == nullptr) {
        return false;
      }
    }

    {
      header_t cmd_header;
      cmd_header.cmd = command;
      cmd_header.size = 2 * sizeof(int) + node.length() + service.length(); // see swrite_string
      if(!swrite(*hostname_resolver_, reinterpret_cast<char*>(&cmd_header), sizeof(cmd_header))) {
        goto failed;
      }
      if(!swrite_string(*hostname_resolver_, node)) {
        goto failed;
      }
      if(!swrite_string(*hostname_resolver_, service)) {
        goto failed;
      }
    }

    {
      header_t resp_header;
      int res = 0;
      if (!sread(*hostname_resolver_, reinterpret_cast<char*>(&resp_header), sizeof(resp_header))) {
        goto failed;
      }
      if (resp_header.cmd != command) {
        goto failed;
      }
      if (!sread_field(resp_header, *hostname_resolver_, reinterpret_cast<char*>(&res), sizeof(res))) {
        goto failed;
      }
      if (!sread_field(resp_header, *hostname_resolver_, reinterpret_cast<char*>(&errno_), sizeof(errno_))) {
        goto failed;
      }
      while(resp_header.size > 0) {
        SockAddr addr;
        if (!sread_field(resp_header, *hostname_resolver_, reinterpret_cast<char*>(&addr.family), sizeof(addr.family))) {
          goto failed;
        }
        if (!sread_field(resp_header, *hostname_resolver_, reinterpret_cast<char*>(&addr.length), sizeof(addr.length))) {
          goto failed;
        }
        addr.addr = reinterpret_cast<sockaddr*>(::operator new(addr.length));
        if (!sread_field(resp_header, *hostname_resolver_, reinterpret_cast<char*>(addr.addr), addr.length)) {
          goto failed;
        }
        addrs.push_back(addr);
      };
      return res;
    }

  failed:
    delete hostname_resolver_;
    hostname_resolver_ = nullptr;
    errno_ = -1;
    return -1;
  }

  void HostnameResolver::testtune(void (*callback)(const char* action, ...)) {
    relativeResolverPath = true;
    rcache_.testtune(10, 5, callback);
  }

  HostnameResolverContainer::HostnameResolverContainer(unsigned int minval, unsigned int maxval)
    : min_(minval)
    , max_(maxval)
  {
    if (min_ > max_) {
      abort();
    }
    std::unique_lock<std::mutex> lock(lock_);
    KeepRangeLocked();
  }

  HostnameResolverContainer::~HostnameResolverContainer() {
    std::unique_lock<std::mutex> lock(lock_);
    for (auto& hr : hrs_) {
      delete hr;
    }
  }

  HostnameResolver* HostnameResolverContainer::Acquire() {
    HostnameResolver* r = nullptr;
    for (;;) {
      // Obtain resolver from cache if possible
      r = MaybePopFront();
      if (r == nullptr) {
        break;
      }

      // If cached resolver is still functional, take it
      if(r->Ping()) {
        break;
      }

      // Broken resolver - kill it
      delete r;
      r = nullptr;
    }

    // Make new resolver if necessary - assume it will be functional
    if (r == nullptr) {
      r = new HostnameResolver;
    }

    // Backfill in case it's below the low watermark
    {
      std::unique_lock<std::mutex> lock(lock_);
      KeepRangeLocked();
    }

    // Never null
    return r;
  }

  HostnameResolver* HostnameResolverContainer::MaybePopFront() {
    std::unique_lock<std::mutex> lock(lock_);
    if (hrs_.size() == 0) {
      return nullptr;
    }
    HostnameResolver* r = hrs_.front();
    hrs_.pop_front();
    // Do not call KeepRangeLocked(), client must do that when it's done
    return r;
  }

  void HostnameResolverContainer::Release(HostnameResolver* hr) {
    if (hr != nullptr) {
      std::unique_lock<std::mutex> lock(lock_);
      hrs_.push_back(hr);
      KeepRangeLocked();
    }
  }

  void HostnameResolverContainer::KeepRangeLocked() {
    while (hrs_.size() > max_) {
      HostnameResolver* fa = hrs_.front();
      hrs_.pop_front();
      delete fa;
    }
    while (hrs_.size() < min_) {
      hrs_.push_back(new HostnameResolver);
    }
  }

  // Very simple list-based cache implementation.  A hash table would be better for large
  // populations, but do we expect large populations?  The ttl is normally smallish, maybe on the
  // order of a few minutes; this will help keep the list short.

  HostnameResolverCache::HostnameResolverCache(int ttl_s)
    : ttl_s_(ttl_s)
    , gc_interval_s_(ttl_s_)
    , test_callback_(nullptr)
    , next_gc_(::time(nullptr) + gc_interval_s_)
  { }

  HostnameResolverCache::~HostnameResolverCache() {
    std::unique_lock<std::mutex> l(lock_);
    for (auto* it : contents_) {
      delete it;
    }
  }

  ResolveStatus
  HostnameResolverCache::Resolve(std::string const& node, std::string const& service, bool local,
                                 std::list<HostnameResolver::SockAddr>& addrs) {
    std::unique_lock<std::mutex> l(lock_);
    bool newNode = false;
    CacheEntry* probe = FindLocked(node, service, local);
    if (probe == nullptr) {
      probe = new CacheEntry(node, service, local);
      contents_.push_back(probe);
      newNode = true;
    }
    if (probe->status == ResolveStatus::Pending) {
      if (newNode) {
        return ResolveStatus::YoureIt;
      }
      return ResolveStatus::Pending;
    }
    if (probe->status == ResolveStatus::Resolved) {
      for (auto const& a : probe->addrs) {
        addrs.emplace_back(a);
      }
    }
    return probe->status;
  }

  void HostnameResolverCache::Resolved(int res, std::string const& node, std::string const& service,
                                       bool local, std::list<HostnameResolver::SockAddr> const& addrs) {
    time_t now = ::time(nullptr);
    std::unique_lock<std::mutex> l(lock_);
    CacheEntry* it = FindLocked(node, service, local);
    // It should never be null because we only come here after Resolve() has found a pending node,
    // and pending nodes are not garbage collected.
    // assert(it != nullptr)
    // assert(it->status == ResolveStatus::Pending)
    if (res == 0) {
      for (auto const& a : addrs) {
        it->addrs.emplace_back(a);
      }
      it->status = ResolveStatus::Resolved;
    } else {
      it->status = ResolveStatus::Failed;
    }
    it->expires = now + ttl_s_;
    if (test_callback_ != nullptr) {
      test_callback_("lookup", node.c_str(), service.c_str(), (local ? "local" : "remote"), it->expires);
    }
  }

  HostnameResolverCache::CacheEntry*
  HostnameResolverCache::FindLocked(std::string const& node, std::string const& service, bool local) {
    time_t now = ::time(nullptr);

    // If it's been a while since we removed expired items, compact the list first.
    if (now > next_gc_) {
      CollectLocked(now);
    }

    // Scan the list looking for the item.  There are no dead items since we just ran GC.
    for (auto const& it : contents_) {
      if (it->node == node && it->service == service && it->local == local) {
        return it;
      }
    }

    return nullptr;
  }

  void HostnameResolverCache::CollectLocked(time_t now) {
    size_t next_free = 0;
    size_t nelem = contents_.size();
    for (auto const& it : contents_) {
      bool dead = it->expires < now && it->status != ResolveStatus::Pending;
      if (!dead) {
        contents_[next_free] = it;
        next_free++;
      } else {
        if (test_callback_ != nullptr) {
          test_callback_("reap", it->node.c_str(), it->service.c_str(), (it->local ? "local" : "remote"),
                       it->expires);
        }
        delete it;
      }
    }
    contents_.resize(next_free);
    if (test_callback_ != nullptr) {
      test_callback_("gc", (unsigned long)(nelem - next_free), (unsigned long)next_free);
    }
    next_gc_ = now + gc_interval_s_;
  }
}

