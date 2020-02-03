// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_THREAD_H__
#define __ARC_THREAD_H__

#include <map>

#include <glibmm/thread.h>

namespace Arc {

  /** \addtogroup common
   *  @{ */

  class SimpleCondition;
  class SimpleCounter;

  // This module provides convenient helpers for Glibmm interface for thread
  // management. So far it takes care of automatic initialization
  // of threading environment and creation of simple detached threads.
  // Always use it instead of glibmm/thread.h and keep among first
  // includes. It safe to use it multiple times and to include it both
  // from source files and other include files.

  /** \cond Defines size of stack assigned to every new thread.
     It seems like MacOS has very small stack per thread by default.
     So it's safer to have bigger one defined. If this value is set
     to 0 default value will be used. */
  const size_t thread_stacksize = (16 * 1024 * 1024);
  /** \endcond */

  /// Helper function to create simple thread.
  /** It takes care of all the peculiarities of the Glib::Thread API. It
      runs function 'func' with argument 'arg' in a separate thread. If count
      parameter is not NULL then count will be incremented before this function
      returns and then decremented when thread finishes.
      \return true on success. */
  bool CreateThreadFunction(void (*func)(void*), void *arg, SimpleCounter* count = NULL);

  /** \cond Internal class used to map glib thread ids (pointer addresses) to
     an incremental counter, for easier debugging. */
  class ThreadId {
  private:
    Glib::Mutex mutex;
    std::map<unsigned long int, unsigned long int> thread_ids;
    unsigned long int thread_no;
    ThreadId();
   public:
    static ThreadId& getInstance();
    /// Called at beginning of ThreadArgument.thread() to add thread id to map
    void add();
    /// Called at end of ThreadArgument.thread() to remove thread id from map
    void remove();
    /// Called by logger to get id of current thread
    unsigned long int get();
  };
  /** \endcond */

  class ThreadData;

  /// Base class for per-thread object.
  /** Classes inherited from this one are attached to current thread under
     specified key and destroyed only when thread ends or object is replaced by
     another one with same key.
     \headerfile Thread.h arc/Thread.h */
  class ThreadDataItem {
  friend class ThreadData;
  private:
    ThreadDataItem(const ThreadDataItem& it);
  protected:
    virtual ~ThreadDataItem(void);
  public:
    /// Dummy constructor which does nothing.
    /** To make object usable one of the Attach(...) methods must be used. */
    ThreadDataItem(void);
    /// Creates instance and attaches it to current thread under key.
    /** If supplied key is empty random one is generated and stored in
        key variable. */
    ThreadDataItem(std::string& key);
    /// Creates instance and attaches it to current thread under key.
    ThreadDataItem(const std::string& key);
    /// Attaches object to  current thread under key.
    /** If supplied key is empty random one is generated and stored in
        key variable. This method must be used only if object was
        created using dummy constructor. */
    void Attach(std::string& key);
    /// Attaches object to  current thread under key.
    /** This method must be used only if object was created using
        dummy constructor. */
    void Attach(const std::string& key);
    /// Retrieves object attached to thread under key.
    /** \return NULL if no such obejct. */
    static ThreadDataItem* Get(const std::string& key);
    /// Creates copy of object.
    /** This method is called when a new thread is created from the current
      thread. It is called in the new thread, so the new object - if created -
      gets attached to the new thread. If the object is not meant to be
      inherited by new threads then this method should do nothing. */
    virtual void Dup(void);
  };

  class ThreadArgument;

  /// Base class for simple object associated thread.
  class Thread {
   friend class ThreadArgument;
   public:
    /// Start thread 
    /** This method provides functionality similar to CreateThreadFunction
       but runs thread() method instead of specified function. */
    bool start(SimpleCounter* count = NULL);

    virtual ~Thread(void) {};

   protected:
    /// Implement this method and put thread functionality into it
    virtual void thread(void) = 0;

  };

  template<typename T> class AutoLock {
  public:
    AutoLock(T& olock, bool olocked = true) : lock_(olock), locked_(0) {
      if(olocked) lock();
    }

    ~AutoLock() {
      if(locked_ != 0) lock_.unlock();
      locked_ = 0;
    }

    void lock() {
      if(locked_ == 0) lock_.lock();
      ++locked_;
    }

    void unlock() {
      if(locked_ == 1) lock_.unlock();
      --locked_;
    }

  private:
    T& lock_;
    int locked_;
  };

  /// Simple triggered condition.
  /** Provides condition and semaphor objects in one element.
      \headerfile Thread.h arc/Thread.h  */
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
    /// Acquire semaphor.
    void lock(void) {
      lock_.lock();
    }
    /// Release semaphor.
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
    /// Signal about condition without using semaphor.
    /** Call it *only* with lock acquired. */
    void signal_nonblock(void) {
      flag_ = 1;
      cond_.signal();
    }
    /// Signal about condition to all waiting threads.
    /** If there are no waiting threads, it works like signal(). */
    void broadcast(void) {
      lock_.lock();
      flag_ = waiting_?waiting_:1;
      cond_.broadcast();
      lock_.unlock();
    }
    /// Wait for condition.
    void wait(void) {
      lock_.lock();
      ++waiting_;
      while (!flag_) cond_.wait(lock_);
      --waiting_;
      --flag_;
      lock_.unlock();
    }
    /// Wait for condition without using semaphor.
    /** Call it *only* with lock acquired. */
    void wait_nonblock(void) {
      ++waiting_;
      while (!flag_) cond_.wait(lock_);
      --waiting_;
      --flag_;
    }
    /// Wait for condition no longer than t milliseconds.
    /** \return false if timeout occurred */
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
    /// Reset object to initial state.
    void reset(void) {
      lock_.lock();
      flag_ = 0;
      lock_.unlock();
    }
    /// This method is meant to be used only after fork.
    /** It resets state of all internal locks and variables. */
    void forceReset(void) {
      // This function is deprecated and its body removed because
      // there is no safe way to reset locks after call to fork().
    }
  };

  /// Thread-safe counter with capability to wait for zero value.
  /** It is extendible through re-implementation of virtual methods.
      \headerfile Thread.h arc/Thread.h */
  class SimpleCounter {
  private:
    Glib::Cond cond_;
    Glib::Mutex lock_;
    int count_;
  public:
    SimpleCounter(void) : count_(0) {}
    virtual ~SimpleCounter(void);
    /// Increment value of counter.
    /** \return new value. */
    virtual int inc(void);
    /// Decrement value of counter.
    /** \return new value. Does not go below 0 value. */
    virtual int dec(void);
    /// \return current value of counter.
    virtual int get(void) const;
    /// Set value of counter.
    /** \return new value. */
    virtual int set(int v);
    /// Wait for zero condition.
    virtual void wait(void) const;
    /// Wait for zero condition no longer than t milliseconds.
    /** If t is negative - wait forever.
        \return false if timeout occurred. */
    virtual bool wait(int t) const;
    /// This method is meant to be used only after fork.
    /** It resets state of all internal locks and variables. */
    virtual void forceReset(void) {
      // This function is deprecated and its body removed because
      // there is no safe way to reset locks after call to fork().
    }
  };

  /// Mutex which allows a timeout on locking.
  /** \headerfile Thread.h arc/Thread.h */
  class TimedMutex {
  private:
    Glib::Cond cond_;
    Glib::Mutex lock_;
    bool locked_;
  public:
    TimedMutex(void):locked_(false) { };
    ~TimedMutex(void) { };
    /// Lock mutex, but wait no longer than t milliseconds.
    /** \return false if timeout occurred. */
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
    /// Returns true if mutex is currently locked, but does not attempt to acquire lock.
    bool trylock(void) {
      return lock(0);
    };
    /// Release mutex.
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
    /// This method is meant to be used only after fork.
    /** It resets state of all internal locks and variables. */
    void forceReset(void) {
      // This function is deprecated and its body removed because
      // there is no safe way to reset locks after call to fork().
    }
  };

  /// Mutex which allows shared and exclusive locking.
  /** \headerfile Thread.h arc/Thread.h */
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
    /// Acquire a shared lock. Blocks until exclusive lock is released.
    void lockShared(void);
    /// Release a shared lock.
    void unlockShared(void);
    /// Returns true if at least one shared lock is held.
    bool isLockShared(void) {
      return (shared_.size() > 0); // Is it safe?
    };
    /// Acquire an exclusive lock. Blocks until all shared and exclusive locks are released.
    void lockExclusive(void);
    /// Release exclusive lock.
    void unlockExclusive(void);
    /// Returns true if the exclusive lock is held.
    bool isLockExclusive(void) {
      return (exclusive_ > 0);
    };
    /// This method is meant to be used only after fork.
    /** It resets state of all internal locks and variables. */
    void forceReset(void) {
      // This function is deprecated and its body removed because
      // there is no safe way to reset locks after call to fork().
    };
  };

  /** \cond Helper class for ThreadedPointer.
     \headerfile Thread.h arc/Thread.h */
  class ThreadedPointerBase {
  private:
    Glib::Mutex lock_;
    Glib::Cond cond_;
    unsigned int cnt_;
    void *ptr_;
    bool released_;
    ThreadedPointerBase(ThreadedPointerBase&);
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
  /** \endcond */

  /// Wrapper for pointer with automatic destruction and multiple references.
  /** See for CountedPointer for description. Differently from CountedPointer
     this class provides thread safe destruction of referred object. But the
     instance of ThreadedPointer itself is not thread safe. Hence it is
     advisable to use different instances in different threads.
     \headerfile Thread.h arc/Thread.h */
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
    /// Assign a new ThreadedPointer from a pointer to an object.
    ThreadedPointer<T>& operator=(T *p) {
      if (p != object_->ptr()) {
        delete((T*)(object_->rem()));
        object_ = new ThreadedPointerBase(p);
      }
      return *this;
    }
    /// Assign a new ThreadedPointer from another ThreadedPointer.
    ThreadedPointer<T>& operator=(const ThreadedPointer<T>& p) {
      if (p.object_->ptr() != object_->ptr()) {
        delete((T*)(object_->rem()));
        object_ = p.object_->add();
      }
      return *this;
    }
    /// For referring to wrapped object
    T& operator*(void) const {
      return *(T*)(object_->ptr());
    }
    /// For referring to wrapped object
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
    /// Returns true if pointers are equal
    bool operator==(const ThreadedPointer& p) const {
      return ((T*)(object_->ptr()) == (T*)(p.object_->ptr()));
    }
    /// Returns true if pointers are not equal
    bool operator!=(const ThreadedPointer& p) const {
      return ((T*)(object_->ptr()) != (T*)(p.object_->ptr()));
    }
    /// Comparison operator
    bool operator<(const ThreadedPointer& p) const {
      return ((T*)(object_->ptr()) < (T*)(p.object_->ptr()));
    }
    /// Cast to original pointer
    T* Ptr(void) const {
      return (T*)(object_->ptr());
    }
    /// Release referred object so that it can be passed to other container
    /** After Release() is called referred object is will not be destroyed
       automatically anymore. */
    T* Release(void) {
      T* tmp = (T*)(object_->ptr());
      object_->rel();
      return tmp;
    }
    /// Returns number of ThreadedPointer instances referring to underlying object
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

  /// A set of conditions, mutexes, etc. conveniently exposed to monitor running child threads and to wait till they exit.
  /** There are no protections against race conditions, so use it carefully.
      \headerfile Thread.h arc/Thread.h */
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
    /** \return true if cancel request received. */
    bool WaitOrCancel(int timeout);
    /// Wait for registered threads to exit.
    /** Leave after timeout milliseconds if failed.
        \return true if all registered threads reported their exit. */
    bool WaitForExit(int timeout = -1);
    /// Send cancel request to registered threads
    void RequestCancel(void);
    /// This method is meant to be used only after fork.
    /** It resets state of all internal locks and variables. */
    void forceReset(void) {
      // This function is deprecated and its body removed because
      // there is no safe way to reset locks after call to fork().
    }
  };

  /** \cond Internal function to initialize Glib thread system. Use
      ThreadInitializer instead. */
  void GlibThreadInitialize(void);
  /** \endcond */

  /// This class initializes the glibmm thread system.
  class ThreadInitializer {
  public:
    /// Initialise the thread system
    ThreadInitializer(void) {
      GlibThreadInitialize();
    }
    /// This method is meant to be used only after fork.
    /** It resets state of all internal locks and variables.
       This method is deprecated. */
    void forceReset(void);
    /// Wait for all known threads to exit.
    /** It can be used before exiting application to make
        sure no concurrent threads are running during cleanup. */
    void waitExit(void);
  };

  // This is done intentionally to make sure glibmm is
  // properly initialized before every module starts
  // using threads functionality. To make it work this
  // header must be included before defining any 
  // variable/class instance using static threads-related
  // elements. The simplest way to do that is to use
  // this header instead of glibmm/thread.h
  static ThreadInitializer _local_thread_initializer;

  /** @} */

} // namespace Arc

#endif /* __ARC_THREAD_H__ */
