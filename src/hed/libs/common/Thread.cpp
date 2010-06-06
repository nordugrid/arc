// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#define USE_THREAD_POOL

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <glibmm/init.h>

#ifdef USE_THREAD_POOL
#include <sys/time.h>
#ifndef WIN32
#include <sys/resource.h>
#endif
#endif

#include "Thread.h"
#include "Logger.h"
#include "User.h"


namespace Arc {

  static Logger threadLogger(Logger::getRootLogger(), "Thread");

#ifdef USE_THREAD_POOL
  class ThreadInc {
   public:
    ThreadInc(void);
    ~ThreadInc(void);
  };
#endif

  class ThreadArgument {
  public:
    typedef void (*func_t)(void*);
    void *arg;
    func_t func;
    SimpleCounter* count;
#ifdef USE_THREAD_POOL
    ThreadInc* resource;
    ThreadArgument& acquire(void) {
      resource = new ThreadInc;
      return *this;
    }
    void release(void) {
      if(resource) delete resource;
      resource = NULL;
    }
#endif
    ThreadArgument(func_t f, void *a, SimpleCounter *c)
      : arg(a),
        func(f),
        count(c)
#ifdef USE_THREAD_POOL
        ,resource(NULL)
#endif
    {}
    ~ThreadArgument(void) { }
    void thread(void);
  };


#ifdef USE_THREAD_POOL
  // This is not a real pool. It is just a queue of requests for
  // new threads. Hopefuly it will be converted to pool later for
  // better performance.
  class ThreadPool {
   public:
   friend class ThreadInc;
   private:
    int max_count;
    int count;
    Glib::Mutex count_lock;
    Glib::Mutex queue_lock;
    std::list<ThreadArgument*> queue;
    int CheckQueue(void);
    ~ThreadPool(void) { };
   public:
    ThreadPool(void);
    void PushQueue(ThreadArgument* arg);
  };

  ThreadPool::ThreadPool(void):max_count(0),count(0) {
    // Estimating amount of available memory 
    uint64_t n = 0;
#ifndef WIN32
    struct rlimit rl;
    if(getrlimit(RLIMIT_AS,&rl) == 0) {
      if(rl.rlim_cur != RLIM_INFINITY) {
        // Dividing by 2 assuming each thread will equally use 
        // stack and heap
        n = rl.rlim_cur/thread_stacksize/2;
        if(n == 0) {
          // What else can we do. Application will fail on first thread
          n=1;
        }
      } else if(rl.rlim_max != RLIM_INFINITY) {
        n = rl.rlim_max/thread_stacksize/2;
        if(n == 0) {
          n=1;
        }
      }
    }
#endif
    if(n == 0) {
      // Very rough estimation of number of threads which can be run
      n = (((uint64_t)1)<<(8*sizeof(int*) - 2))/thread_stacksize;
    }
    if(n > INT_MAX) n = INT_MAX;
    max_count = (int)n;
    // TODO: can't use logger here because it will try to initilize pool
    //threadLogger.msg(DEBUG, "Maximum number of threads is %i",max_count);
  }
  int ThreadPool::CheckQueue(void) {
    Glib::Mutex::Lock lock(queue_lock, Glib::TRY_LOCK);
    if(!lock.locked()) return -1;
    int size = queue.size();
    while((count < max_count) && (size > 0)) {
      ThreadArgument* argument = *(queue.begin());
      argument->acquire();
      try {
        UserSwitch usw(0,0);
        Glib::Thread::create(sigc::mem_fun(*argument,
                                           &ThreadArgument::thread),
                             thread_stacksize, false, false,
                             Glib::THREAD_PRIORITY_NORMAL);
        queue.erase(queue.begin());
      } catch (Glib::Error& e) {
        threadLogger.msg(ERROR, e.what());
        argument->release();
      } catch (std::exception& e) {
        threadLogger.msg(ERROR, e.what());
        argument->release();
      };
      size = queue.size();
    }
    return size;
  }

  void ThreadPool::PushQueue(ThreadArgument* arg) {
    Glib::Mutex::Lock lock(queue_lock);
    queue.push_back(arg);
    lock.release();
    if(CheckQueue() > 0)
      threadLogger.msg(INFO, "Maximum number of threads running - puting new request into queue");
  }

  static ThreadPool* pool = NULL;

  ThreadInc::ThreadInc(void) {
    if(!pool) return;
    pool->count_lock.lock();
    ++(pool->count);
    pool->count_lock.unlock();
  }

  ThreadInc::~ThreadInc(void) {
    if(!pool) return;
    pool->count_lock.lock();
    --(pool->count);
    pool->count_lock.unlock();
    pool->CheckQueue();
  }
#endif

  void ThreadArgument::thread(void) {
#ifdef USE_THREAD_POOL
    ThreadInc resource_;
    release();
#endif
    func_t f_temp = func;
    void *a_temp = arg;
    SimpleCounter *c_temp = count;
    delete this;
    (*f_temp)(a_temp);
    if(c_temp) c_temp->dec();
  }

  bool CreateThreadFunction(void (*func)(void*), void *arg, SimpleCounter* count
) {
#ifdef USE_THREAD_POOL
    if(!pool) return false;
    ThreadArgument *argument = new ThreadArgument(func, arg, count);
    if(count) count->inc();
    pool->PushQueue(argument);
#else
    ThreadArgument *argument = new ThreadArgument(func, arg, count);
    if(count) count->inc();
    try {
      UserSwitch usw(0,0);
      Glib::Thread::create(sigc::mem_fun(*argument, &ThreadArgument::thread),
                           thread_stacksize, false, false,
                           Glib::THREAD_PRIORITY_NORMAL);
    } catch (std::exception& e) {
      threadLogger.msg(ERROR, e.what());
      if(count) count->dec();
      delete argument;
      return false;
    };
#endif
    return true;
  }

/*
  bool CreateThreadFunction(void (*func)(void*), void *arg, Glib::Thread *&thr) {
    ThreadArgument *argument = new ThreadArgument(func, arg);
    Glib::Thread *thread;
    try {
      UserSwitch usw(0,0);
      thread = Glib::Thread::create(sigc::mem_fun(*argument, &ThreadArgument::thread),             
	                  thread_stacksize,
                                  true,  // thread joinable
                                 false,
          Glib::THREAD_PRIORITY_NORMAL);
    } catch (std::exception& e) {
      threadLogger.msg(ERROR, e.what());
      delete argument;
      return false;
    };    
    thr = thread;
    return true;
  }
*/

  /*
     Example of how to use CreateThreadClass macro

     class testclass {
      public:
          int a;
          testclass(int v) { a=v; };
          void run(void) { a=0; };
     };

     void test(void) {
     testclass tc(1);
     CreateThreadClass(tc,testclass::run);
     }
   */

  void GlibThreadInitialize(void) {
    Glib::init();
    if (!Glib::thread_supported())
      Glib::thread_init();
#ifdef USE_THREAD_POOL
    if (!pool)
      pool = new ThreadPool;
#endif
  }

  ThreadRegistry::ThreadRegistry(void):counter_(0),cancel_(false) {
  }

  ThreadRegistry::~ThreadRegistry(void) {
  }

  void ThreadRegistry::RegisterThread(void) {
    lock_.lock();
    ++counter_;
    lock_.unlock();
  }

  void ThreadRegistry::UnregisterThread(void) {
    lock_.lock();
    --counter_;
    cond_.broadcast();
    lock_.unlock();
  }

  bool ThreadRegistry::WaitOrCancel(int timeout) {
    bool v = false;
    lock_.lock();
    Glib::TimeVal etime;
    etime.assign_current_time();
    etime.add_milliseconds(timeout);
    while (!cancel_) {
      if(!cond_.timed_wait(lock_, etime)) break;
    }
    v = cancel_;
    lock_.unlock();
    return v;
  }

  bool ThreadRegistry::WaitForExit(int timeout) {
    int n = 0;
    lock_.lock();
    if(timeout >= 0) {
      Glib::TimeVal etime;
      etime.assign_current_time();
      etime.add_milliseconds(timeout);
      while (counter_ > 0) {
        if(!cond_.timed_wait(lock_, etime)) break;
      }
    } else {
      while (counter_ > 0) {
        cond_.wait(lock_);
      }
    }
    n = counter_;
    lock_.unlock();
    return (n <= 0);
  }

  void ThreadRegistry::RequestCancel(void) {
    lock_.lock();
    cancel_=true;
    cond_.broadcast();
    lock_.unlock();
  }

} // namespace Arc
