// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_THREAD_H__
#define __ARC_THREAD_H__

#include <glibmm/thread.h>

namespace Arc {

  class SimpleCondition;
  class SimpleCounter;

  /// This module provides convenient helpers for Glibmm interface for thread management.
  /** So far it takes care of automatic initialization
     of threading environment and creation of simple detached threads.
     Always use it instead of glibmm/thread.h and keep among first
     includes. It safe to use it multiple times and to include it both
     from source files and other include files. */

  /// Defines size of stack assigned to every new thread.
  /* It seems like MacOS has very small stack per thread by default.
     So it's safer to have bigger one defined. If this value is set
     to 0 default value will be used. */
  const size_t thread_stacksize = (16 * 1024 * 1024);

  /// This macro behaves like function which makes thread of class' method.
  /** It accepts class instance and full  name of
     method - like class::method. 'method' should not be static member
     of the class. Result is true if creation of thread succeeded.
     Specified instance must be valid during whole lifetime of
     thread. So probably it is safer to destroy 'instance' in
     'method' just before exiting. */
/*
#define CreateThreadClass(instance, method) \
  { \
    Glib::Thread *thr = NULL; \
    * ThreadLock.lock(); * \
    try { \
      UserSwitch usw(0,0); \
      thr = Glib::Thread::create(sigc::mem_fun((instance), &method), false); \
    } catch (std::exception& e) {}; \
    * ThreadLock.unlock(); * \
    (thr != NULL); \
  }
*/

  /// Helper function to create simple thread.
  /** It takes care of all pecularities of Glib::Thread API.
     As result it runs function 'func' with argument 'arg' in a separate
     thread.
     If count parameter not NULL then corresponding object will be incremented
     before function returns and then decremented then thread finished.
     Returns true on success. */
  bool CreateThreadFunction(void (*func)(void*), void *arg, SimpleCounter* count = NULL);

  /// Helper function to create simple thread.
  /** It takes care of all pecularities of Glib::Thread API.
     As result it runs function 'func' with argument 'arg' in a separate
     thread. The created thread will be joinable.
     Returns true on success.
     This function is currently disable becaueit is not clear if
     joinability is a needed feature */
  //bool CreateThreadFunction(void (*func)(void*), void *arg, Glib::Thread *&thr);

  class ThreadData;

  /// Base class for per-thread object.
  /** Classes inherited from this one are attached to current thread under
     specified key and destroyed only when thread ends or object is replaced by
     another one with same key. */
  class ThreadDataItem {
  friend class ThreadData;
  private:
    ThreadDataItem(const ThreadDataItem& it);
  protected:
    virtual ~ThreadDataItem(void);
  public:
    /** Dummy constructor which does nothing.
      To make object usable one of Attach(...) methods must be used. */
    ThreadDataItem(void);
    /** Creates instance and attaches it to current thread under key.
       If supplied key is empty random one is generated and stored in 
       key variable. */
    ThreadDataItem(std::string& key);
    /** Creates instance and attaches it to current thread under key. */
    ThreadDataItem(const std::string& key);
    /** Attaches object to  current thread under key.
       If supplied key is empty random one is generated and stored in 
       key variable. This method must be used only if object was
       created using dummy constructor. */
    void Attach(std::string& key);
    /** Attaches object to  current thread under key.
       This method must be used only if object was created using 
       dummy constructor. */
    void Attach(const std::string& key);
    /** Retrieves object attached to thread under key.
       Returns if no such obejct. */
    static ThreadDataItem* Get(const std::string& key);
    /** Creates copy of object. 
      This method is called when new thread is created from current thread.
      It is called in new thread, so new object - if created - gets attached
      to new thread. If object is not meant to be inherited by new threads
      then this method should do nothing. */
    virtual void Dup(void);
  };

  /// Simple triggered condition.
  /** Provides condition and semaphor objects in one element. */
  class SimpleCondition {
  private:
    Glib::Cond cond_;
    Glib::Mutex lock_;
    bool flag_;
  public:
    SimpleCondition(void)
      : flag_(false) {}
    ~SimpleCondition(void) {
      /* race condition ? */
      broadcast();
    }
    /** Acquire semaphor */
    void lock(void) {
      lock_.lock();
    }
    /** Release semaphor */
    void unlock(void) {
      lock_.unlock();
    }
    /** Signal about condition */
    void signal(void) {
      lock_.lock();
      flag_ = true;
      cond_.signal();
      lock_.unlock();
    }
    /** Signal about condition without using semaphor */
    void signal_nonblock(void) {
      flag_ = true;
      cond_.signal();
    }
    /** Signal about condition to all waiting threads */
    void broadcast(void) {
      lock_.lock();
      flag_ = true;
      cond_.broadcast();
      lock_.unlock();
    }
    /** Wait for condition */
    void wait(void) {
      lock_.lock();
      while (!flag_)
        cond_.wait(lock_);
      flag_ = false;
      lock_.unlock();
    }
    /** Wait for condition without using semaphor */
    void wait_nonblock(void) {
      while (!flag_)
        cond_.wait(lock_);
      flag_ = false;
    }
    /** Wait for condition no longer than t milliseconds */
    bool wait(int t) {
      lock_.lock();
      Glib::TimeVal etime;
      etime.assign_current_time();
      etime.add_milliseconds(t);
      bool res(true);
      while (!flag_) {
        res = cond_.timed_wait(lock_, etime);
        if (!res)
          break;
      }
      flag_ = false;
      lock_.unlock();
      return res;
    }
    /** Reset object to initial state */
    void reset(void) {
      lock_.lock();
      flag_ = false;
      lock_.unlock();
    }
  };

  class SimpleCounter {
  private:
    Glib::Cond cond_;
    Glib::Mutex lock_;
    int count_;
  public:
   SimpleCounter(void)
      : count_(0) {}
    ~SimpleCounter(void) {
      /* race condition ? */
      lock_.lock();
      count_ = 0;
      cond_.broadcast();
      lock_.unlock();
    }
    int inc(void) {
      lock_.lock();
      ++count_;
      int r = count_;
      lock_.unlock();
      return r;
    }
    int dec(void) {
      lock_.lock();
      if(count_ > 0) --count_;
      if(count_ <= 0) cond_.signal();
      int r = count_;
      lock_.unlock();
      return r;
    }
    int get(void) {
      lock_.lock();
      int r = count_;
      lock_.unlock();
      return r;
    }
    void wait(void) {
      lock_.lock();
      while (count_ > 0) cond_.wait(lock_);
      lock_.unlock();
    }
    /** Wait for condition no longer than t milliseconds */
    bool wait(int t) {
      lock_.lock();
      Glib::TimeVal etime;
      etime.assign_current_time();
      etime.add_milliseconds(t);
      bool res(true);
      while (count_ > 0) {
        res = cond_.timed_wait(lock_, etime);
        if (!res)
          break;
      }
      lock_.unlock();
      return res;
    }
  };

  class TimedMutex {
  private:
    Glib::Cond cond_;
    Glib::Mutex lock_;
    bool locked_;
  public:
    TimedMutex(void):locked_(false) { };
    ~TimedMutex(void) { };
    bool lock(int t = -1) {
      lock_.lock();
      if(t < 0) { // infinite
        while(locked_) {
          cond_.wait(lock_);
        };
      } else if(t > 0) { // timed
        Glib::TimeVal etime;
        etime.assign_current_time();
        etime.add_milliseconds(t);
        while(locked_) {
          if(!cond_.timed_wait(lock_, etime)) break;
        };
      };
      bool res = !locked_;
      locked_=true;
      lock_.unlock();
      return res;
    };
    bool trylock(void) {
      return lock(0);
    };
    bool unlock(void) {
      lock_.lock();
      bool res = locked_;
      if(res) {
        locked_ = false;
        cond_.signal();
      };
      lock_.unlock();
      return true;
    };
  };

  class SharedMutex {
  private:
    Glib::Cond cond_;
    Glib::Mutex lock_;
    bool exclusive_;
    int shared_;
  public:
    SharedMutex(void):exclusive_(false),shared_(0) { };
    ~SharedMutex(void) { };
    void lockShared(void) {
      lock_.lock();
      while(exclusive_) {
        cond_.wait(lock_);
      };
      ++shared_;
      lock_.unlock();
    };
    void unlockShared(void) {
      lock_.lock();
      if(shared_) --shared_;
      cond_.broadcast();
      lock_.unlock();
    };
    void lockExclusive(void) {
      lock_.lock();
      while(exclusive_ || shared_) {
        cond_.wait(lock_);
      };
      exclusive_ = true;
      lock_.unlock();
    };
    void unlockExclusive(void) {
      lock_.lock();
      exclusive_ = false;
      cond_.broadcast();
      lock_.unlock();
    };
  };

  /// This class is a set of conditions, mutexes, etc. conveniently
  /// exposed to monitor running child threads and to wait till
  /// they exit. There are no protections against race conditions.
  /// So use it carefully.
  class ThreadRegistry {
  private:
    int counter_;
    bool cancel_;
    Glib::Cond cond_;
    Glib::Mutex lock_;
  public:
    ThreadRegistry(void);
    ~ThreadRegistry(void);
    /// Register thread as started/starting into this instance
    void RegisterThread(void);
    /// Report thread as exited
    void UnregisterThread(void);
    /// Wait for timeout milliseconds or cancel request.
    /// Returns true if cancel request received.
    bool WaitOrCancel(int timeout);
    /// Wait for registered threads to exit.
    /// Leave after timeout miliseconds if failed. Returns true if
    /// all registered threads reported their exit.
    bool WaitForExit(int timeout = -1);
    // Send cancel request to registered threads
    void RequestCancel(void);
  };

  void GlibThreadInitialize(void);

  // This class initializes glibmm thread system
  class ThreadInitializer {
  public:
    ThreadInitializer(void) {
      GlibThreadInitialize();
    }
  };

  // This is done intentionally to make sure glibmm is
  // properly initialized before every module starts
  // using threads functionality. To make it work this
  // header must be included before defining any 
  // variable/class instance using static threads-related
  // elements. The simplest way to do that is to use
  // this header instead of glibmm/thread.h
  static ThreadInitializer _local_thread_initializer;

} // namespace Arc

#endif /* __ARC_THREAD_H__ */
