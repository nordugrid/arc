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

  static bool do_tests = false;
  static HostnameResolverContainer hrs_(0,100);

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
    if(!do_tests) {
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

  /*static*/
  bool HostnameResolver::Resolve(std::string const& node, std::string const& service, bool local,
                                 std::list<SockAddr>& addrs) {
    HostnameResolver* hr = hrs_.Acquire();
    int r = hr->DoResolve(node, service, local, addrs);
    hrs_.Release(hr);
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

  void HostnameResolver::testtune() {
    do_tests = true;
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

}

