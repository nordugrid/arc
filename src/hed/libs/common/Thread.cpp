// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <glibmm/init.h>

#include "Thread.h"
#include "Logger.h"

#define USE_THREAD_POOL

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
    ThreadArgument(func_t f, void *a)
      : arg(a),
        func(f)
#ifdef USE_THREAD_POOL
        ,resource(NULL)
#endif
    {}
    ~ThreadArgument(void) {}
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
    void CheckQueue(void);
    ~ThreadPool(void) { };
   public:
    ThreadPool(void);
    void PushQueue(ThreadArgument* arg);
  };

  ThreadPool::ThreadPool(void):max_count(32),count(0) {
    // Very rough estimation of number of threads which can be run
    uint64_t n = (((uint64_t)1)<<(8*sizeof(int*) - 2))/thread_stacksize;
    if(n > INT_MAX) n = INT_MAX;
    max_count = (int)n;
    // TODO: can't use logger here because it will try to initilize pool
    //threadLogger.msg(VERBOSE, "Maximum number of threads is %i",max_count);
  }
  void ThreadPool::CheckQueue(void) {
    Glib::Mutex::Lock l(queue_lock);
    while((count < max_count) && (queue.size() > 0)) {
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
    }
  }

  void ThreadPool::PushQueue(ThreadArgument* arg) {
    queue_lock.lock();
    queue.push_back(arg);
    queue_lock.unlock();
    CheckQueue();
    queue_lock.lock();
    if(queue.size() > 0)
      threadLogger.msg(WARNING, "Maximum number of threads running - puting new request into queue");
    queue_lock.unlock();
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
    delete this;
    (*f_temp)(a_temp);
  }

  bool CreateThreadFunction(void (*func)(void*), void *arg) {
#ifdef USE_THREAD_POOL
    if(!pool) return false;
    ThreadArgument *argument = new ThreadArgument(func, arg);
    pool->PushQueue(argument);
#else
    ThreadArgument *argument = new ThreadArgument(func, arg);
    try {
      UserSwitch usw(0,0);
      Glib::Thread::create(sigc::mem_fun(*argument, &ThreadArgument::thread),
                           thread_stacksize, false, false,
                           Glib::THREAD_PRIORITY_NORMAL);
    } catch (std::exception& e) {
      threadLogger.msg(ERROR, e.what());
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
    if (!Glib::thread_supported()) {
      Glib::thread_init();
#ifdef USE_THREAD_POOL
      pool = new ThreadPool;
#endif
    }
  }

} // namespace Arc
