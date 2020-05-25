// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <cstdio>
// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <unistd.h>
#include <signal.h>

#include <iostream>
#include <set>

#include <glibmm.h>

#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/User.h>
#include <arc/Utils.h>

#include "Watchdog.h"


namespace Arc {


#define WATCHDOG_TEST_INTERVAL (60)
#define WATCHDOG_KICK_INTERVAL (10)

  class Watchdog {
  friend class WatchdogListener;
  friend class WatchdogChannel;
  private:
    class Channel {
    public:
      int timeout;
      time_t next;
      Channel(void):timeout(-1),next(0) {};
    };
    int lpipe[2];
    Glib::Thread *timer_;
    static Glib::Mutex instance_lock_;
    std::vector<Channel> channels_;
    static Watchdog *instance_;
    static unsigned int mark_;
#define WatchdogMagic (0x1E84FC05)
    static Watchdog& Instance(void);
    int Open(int timeout);
    void Kick(int channel);
    void Close(int channel);
    int Listen(void);
    void Timer(void);
  public:
    Watchdog(void);
    ~Watchdog(void);
  };

  Glib::Mutex Watchdog::instance_lock_;
  Watchdog* Watchdog::instance_ = NULL;
  unsigned int Watchdog::mark_ = ~WatchdogMagic;

  Watchdog& Watchdog::Instance(void) {
    instance_lock_.lock();
    if ((instance_ == NULL) || (mark_ != WatchdogMagic)) {
      instance_ = new Watchdog();
      mark_ = WatchdogMagic;
    }
    instance_lock_.unlock();
    return *instance_;
  }

  Watchdog::Watchdog(void):timer_(NULL) {
    lpipe[0] = -1; lpipe[1] = -1;
    ::pipe(lpipe);
    if(lpipe[1] != -1) fcntl(lpipe[1], F_SETFL, fcntl(lpipe[1], F_GETFL) | O_NONBLOCK);
  }

  Watchdog::~Watchdog(void) {
    // TODO: stop timer thread
    if(lpipe[0] != -1) { ::close(lpipe[0]); lpipe[0] = -1; }
    if(lpipe[1] != -1) { ::close(lpipe[1]); lpipe[1] = -1; }
  }

  void Watchdog::Timer(void) {
    while(true) {
      // TODO: Implement thread exit
      char c = '\0';
      bool is_timeout = false;
      time_t now = ::time(NULL);
      {
        Glib::Mutex::Lock lock(instance_lock_);
        for(int n = 0; n < channels_.size(); ++n) {
          if(channels_[n].timeout < 0) continue; // channel not active
          if(((int)(now - channels_[n].next)) > 0) { is_timeout = true; break; } // timeout
        }
      }
      if(!is_timeout) {
        if(lpipe[1] != -1) write(lpipe[1],&c,1); // report watchdog is ok
      }
      ::sleep(WATCHDOG_KICK_INTERVAL);
    }
  }

  int Watchdog::Open(int timeout) {
    if(timeout <= 0) return -1;
    Glib::Mutex::Lock lock(instance_lock_);
    if(!timer_) {
      // start timer thread
      try {
        timer_ = Glib::Thread::create(sigc::mem_fun(*this, &Watchdog::Timer), false);
      } catch (Glib::Exception& e) {} catch (std::exception& e) {};
    }
    int n = 0;
    for(; n < channels_.size(); ++n) {
      if(channels_[n].timeout < 0) {
        channels_[n].timeout = timeout;
        channels_[n].next = ::time(NULL) + timeout;
        return n;
      }
    }
    channels_.resize(n+1);
    channels_[n].timeout = timeout;
    channels_[n].next = ::time(NULL) + timeout;
    return n;
  }

  void Watchdog::Kick(int channel) {
    Glib::Mutex::Lock lock(instance_lock_);
    if((channel < 0) || (channel >= channels_.size())) return;
    if(channels_[channel].timeout < 0) return;
    channels_[channel].next = ::time(NULL) + channels_[channel].timeout;
  }

  void Watchdog::Close(int channel) {
    Glib::Mutex::Lock lock(instance_lock_);
    if((channel < 0) || (channel >= channels_.size())) return;
    channels_[channel].timeout = -1;
    // resize?
  }

  int Watchdog::Listen(void) {
    return lpipe[0];
  }

  WatchdogChannel::WatchdogChannel(int timeout) {
    id_ = Watchdog::Instance().Open(timeout);
  }

  WatchdogChannel::~WatchdogChannel(void) {
    Watchdog::Instance().Close(id_);
  }
 
  void WatchdogChannel::Kick(void) {
    Watchdog::Instance().Kick(id_);
  }

  WatchdogListener::WatchdogListener(void):
             instance_(Watchdog::Instance()),last((time_t)(-1)) {
  }

  bool WatchdogListener::Listen(int limit, bool& error) {
    error = false;
    int h = instance_.Listen();
    if(h == -1) return !(error = true);
    time_t out = (time_t)(-1); // when to leave
    if(limit >= 0) out = ::time(NULL) + limit;
    int to = 0; // initailly just check if something already arrived
    for(;;) {
      pollfd fd;
      fd.fd = h; fd.events = POLLIN; fd.revents = 0;
      int err = ::poll(&fd, 1, to);
      // process errors
      if((err < 0) && (errno != EINTR)) break; // unrecoverable error
      if(err > 0) { // unexpected results
        if(err != 1) break;
        if(!(fd.revents & POLLIN)) break;
      };
      time_t now = ::time(NULL);
      time_t next = (time_t)(-1); // when to timeout
      if(err == 1) {
        // something arrived
        char c;
        ::read(fd.fd,&c,1);
        last = now; next = now + WATCHDOG_TEST_INTERVAL;
      } else {
        // check timeout
        if(last != (time_t)(-1)) next = last + WATCHDOG_TEST_INTERVAL;
        if((next != (time_t)(-1)) && (((int)(next-now)) <= 0)) return true;
      }
      // check for time limit
      if((limit >= 0) && (((int)(out-now)) <= 0)) return false;
      // prepare timeout for poll
      to = WATCHDOG_TEST_INTERVAL;
      if(next != (time_t)(-1)) {
        int tto = next-now;
        if(tto < to) to = tto;
      }
      if(limit >= 0) {
        int tto = out-now;
        if(tto < to) to = tto;
      }
      if(to < 0) to = 0;
    }
    // communication failure
    error = true;
    return false;
  }

  bool WatchdogListener::Listen(void) {
    bool error;
    return Listen(-1,error);
  }


}

