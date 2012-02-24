// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_THREAD_H__
#define __ARC_THREAD_H__

#include <map>

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
       Returns NULL if no such obejct. */
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
    unsigned int flag_;
    unsigned int waiting_;
  public:
    SimpleCondition(void)
      : flag_(0), waiting_(0) {}
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
    /** Signal about condition. This overrides broadcast(). */
    void signal(void) {
      lock_.lock();
      flag_ = 1;
      cond_.signal();
      lock_.unlock();
    }
    /** Signal about condition without using semaphor.
       Call it *only* with lock acquired. */
    void signal_nonblock(void) {
      flag_ = 1;
      cond_.signal();
    }
    /** Signal about condition to all waiting threads.
       If there are no waiting threads, it works like signal(). */
    void broadcast(void) {
      lock_.lock();
      flag_ = waiting_?waiting_:1;
      cond_.broadcast();
      lock_.unlock();
    }
    /** Wait for condition */
    void wait(void) {
      lock_.lock();
      ++waiting_;
      while (!flag_) cond_.wait(lock_);
      --waiting_;
      --flag_;
      lock_.unlock();
    }
    /** Wait for condition without using semaphor.
       Call it *only* with lock acquired. */
    void wait_nonblock(void) {
      ++waiting_;
      while (!flag_) cond_.wait(lock_);
      --waiting_;
      --flag_;
    }
    /** Wait for condition no longer than t milliseconds */
    bool wait(int t) {
      lock_.lock();
      Glib::TimeVal etime;
      etime.assign_current_time();
      etime.add_milliseconds(t);
      bool res(true);
      ++waiting_;
      while (!flag_) {
        res = cond_.timed_wait(lock_, etime);
        if (!res) break;
      }
      --waiting_;
      if(res) --flag_;
      lock_.unlock();
      return res;
    }
    /** Reset object to initial state */
    void reset(void) {
      lock_.lock();
      flag_ = 0;
      lock_.unlock();
    }
    // This method is meant to be used only after fork.
    // It resets state of all internal locks and variables.
    void forceReset(void) {
      flag_ = 0;
      waiting_ = 0;
      lock_.unlock();
    }
  };

  /// Thread-safe counter with capability to wait for zero value.
  /// It is extendable through re-implementation of virtual methods.
  class SimpleCounter {
  private:
    Glib::Cond cond_;
    Glib::Mutex lock_;
    int count_;
  public:
    SimpleCounter(void) : count_(0) {}
    virtual ~SimpleCounter(void);
    /// Increment value of counter.
    /** Returns new value. */
    virtual int inc(void);
    /// Decrement value of counter.
    /** Returns new value. Does not go below 0 value. */
    virtual int dec(void);
    /// Returns current value of counter.
    virtual int get(void) const;
    /// Set value of counter.
    /** Returns new value. */
    virtual int set(int v);
    /// Wait for zero condition.
    virtual void wait(void) const;
    /// Wait for zero condition no longer than t milliseconds.
    /** If t is negative - wait forever. */
    virtual bool wait(int t) const;
    /// This method is meant to be used only after fork.
    /** It resets state of all internal locks and variables. */
    virtual void forceReset(void) {
      lock_.unlock();
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
    // This method is meant to be used only after fork.
    // It resets state of all internal locks and variables.
    void forceReset(void) {
      locked_ = false;
      lock_.unlock();
    }
  };

  class SharedMutex {
  private:
    Glib::Cond cond_;
    Glib::Mutex lock_;
    unsigned int exclusive_;
    Glib::Thread* thread_;
    typedef std::map<Glib::Thread*,unsigned int> shared_list;
    shared_list shared_;
    void add_shared_lock(void);
    void remove_shared_lock(void);
    bool have_shared_lock(void);
    inline bool have_exclusive_lock(void) {
      if(!exclusive_) return false;
      if(thread_ == Glib::Thread::self()) return false;
      return true;
    };
  public:
    SharedMutex(void):exclusive_(0),thread_(NULL) { };
    ~SharedMutex(void) { };
    void lockShared(void);
    void unlockShared(void);
    bool isLockShared(void) {
      return (shared_.size() > 0); // Is it safe?
    };
    void lockExclusive(void);
    void unlockExclusive(void);
    bool isLockExclusive(void) {
      return (exclusive_ > 0);
    };
    // This method is meant to be used only after fork.
    // It resets state of all internal locks and variables.
    void forceReset(void) {
      exclusive_ = 0;
      thread_ = NULL;
      shared_.clear();
      lock_.unlock();
    };
  };

  /// Helper class for ThreadedPointer
  class ThreadedPointerBase {
  private:
    Glib::Mutex lock_;
    Glib::Cond cond_;
    unsigned int cnt_;
    void *ptr_;
    bool released_;
    ThreadedPointerBase(ThreadedPointerBase&) {};
    ~ThreadedPointerBase(void);
  public:
    ThreadedPointerBase(void *p);
    ThreadedPointerBase* add(void);
    void* rem(void);
    void* ptr(void) const { return ptr_; };
    void rel(void) { released_ = true; };
    unsigned int cnt(void) const { return cnt_; };
    void lock(void) { lock_.lock(); };
    void unlock(void) { lock_.unlock(); };
    void wait(void) { cond_.wait(lock_); };
    bool wait(Glib::TimeVal etime) {
      return cond_.timed_wait(lock_,etime);
    };
  };

  /// Wrapper for pointer with automatic destruction and mutiple references
  /** See for CountedPointer for description. Differently from CountedPointer
     this class provides thread safe destruction of refered object. But the
     instance of ThreadedPointer itself is not thread safe. Hence it is
     advisable to use different instances in different threads. */
  template<typename T>
  class ThreadedPointer {
  private:
    ThreadedPointerBase *object_;
  public:
    ThreadedPointer(T *p)
      : object_(new ThreadedPointerBase(p)) {}
    ThreadedPointer(const ThreadedPointer<T>& p)
      : object_(p.object_->add()) {}
    ThreadedPointer(void)
      : object_(new ThreadedPointerBase(NULL)) {}
    ~ThreadedPointer(void) {
      delete((T*)(object_->rem()));
    }
    ThreadedPointer& operator=(T *p) {
      if (p != object_->ptr()) {
        delete((T*)(object_->rem()));
        object_ = new ThreadedPointerBase(p);
      }
      return *this;
    }
    ThreadedPointer& operator=(ThreadedPointer& p) {
      if (p.object_->ptr() != object_->ptr()) {
        delete((T*)(object_->rem()));
        object_ = p.object_->add();
      }
      return *this;
    }
    /// For refering wrapped object
    T& operator*(void) const {
      return *(T*)(object_->ptr());
    }
    /// For refering wrapped object
    T* operator->(void) const {
      return (T*)(object_->ptr());
    }
    /// Returns false if pointer is NULL and true otherwise.
    operator bool(void) const {
      return ((object_->ptr()) != NULL);
    }
    /// Returns true if pointer is NULL and false otherwise.
    bool operator!(void) const {
      return ((object_->ptr()) == NULL);
    }
    /// Comparison operator
    bool operator<(const ThreadedPointer& p) const {
      return ((T*)(object_->ptr()) < (T*)(p.object_->ptr()));
    }
    /// Cast to original pointer
    T* Ptr(void) const {
      return (T*)(object_->ptr());
    }
    /// Release refered object so that it can be passed to other container
    /** After Release() is called refered object is will not be destroyed
       automatically anymore. */
    T* Release(void) {
      T* tmp = (T*)(object_->ptr());
      object_->rel();
      return tmp;
    }
    /// Returns number of ThreadedPointer instances refering to underlying object
    unsigned int Holders(void) {
      return object_->cnt();
    }
    /// Waits till number of ThreadedPointer instances <= minThr or >= maxThr
    /* Returns current number of instances. */
    unsigned int WaitOutRange(unsigned int minThr, unsigned int maxThr) {
      unsigned int r = 0;
      object_->lock();
      for(;;) {
        r = object_->cnt();
        if(r <= minThr) break;
        if(r >= maxThr) break;
        object_->wait();
      };
      object_->unlock();
      return r;
    }
    /// Waits till number of ThreadedPointer instances <= minThr or >= maxThr
    /** Waits no longer than timeout milliseconds. If timeout is negative -
       wait forever. Returns current number of instances. */
    unsigned int WaitOutRange(unsigned int minThr, unsigned int maxThr, int timeout) {
      if(timeout < 0) return WaitOutRange(minThr, maxThr);
      unsigned int r = 0;
      object_->lock();
      Glib::TimeVal etime;
      etime.assign_current_time();
      etime.add_milliseconds(timeout);
      for(;;) {
        r = object_->cnt();
        if(r <= minThr) break;
        if(r >= maxThr) break;
        if(!object_->wait(etime)) break;
      };
      object_->unlock();
      return r;
    }
    /// Waits till number of ThreadedPointer instances >= minThr and <= maxThr
    /* Returns current number of instances. */
    unsigned int WaitInRange(unsigned int minThr, unsigned int maxThr) {
      unsigned int r = 0;
      object_->lock();
      for(;;) {
        r = object_->cnt();
        if((r >= minThr) && (r <= maxThr)) break;
        object_->wait();
      };
      object_->unlock();
      return r;
    }
    /// Waits till number of ThreadedPointer instances >= minThr and <= maxThr
    /** Waits no longer than timeout milliseconds. If timeout is negative -
       wait forever. Returns current number of instances. */
    unsigned int WaitInRange(unsigned int minThr, unsigned int maxThr, int timeout) {
      if(timeout < 0) return WaitInRange(minThr, maxThr);
      unsigned int r = 0;
      object_->lock();
      Glib::TimeVal etime;
      etime.assign_current_time();
      etime.add_milliseconds(timeout);
      for(;;) {
        r = object_->cnt();
        if((r >= minThr) && (r <= maxThr)) break;
        if(!object_->wait(etime)) break;
      };
      object_->unlock();
      return r;
    }

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
    // This method is meant to be used only after fork.
    // It resets state of all internal locks and variables.
    void forceReset(void) {
      counter_ = 0;
      cancel_ = false;
      lock_.unlock();
    }
  };

  void GlibThreadInitialize(void);

  // This class initializes glibmm thread system
  class ThreadInitializer {
  public:
    ThreadInitializer(void) {
      GlibThreadInitialize();
    }
    // This method is meant to be used only after fork.
    // It resets state of all internal locks and variables.
    void forceReset(void);
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
