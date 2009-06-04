// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Thread.h"
#include "Logger.h"

namespace Arc {

  class ThreadArgument {
  public:
    typedef void (*func_t)(void*);
    void *arg;
    func_t func;
    ThreadArgument(func_t f, void *a)
      : arg(a),
        func(f) {}
    ~ThreadArgument(void) {}
    void thread(void) {
      func_t f_temp = func;
      void *a_temp = arg;
      delete this;
      (*f_temp)(a_temp);
    }
  };

  // TODO: do something to make sure this static object is
  // initialized before any other static object which uses
  // mutexes, semaphors, etc.
  // static ThreadInitializer thread_initializer;

  static Logger threadLogger(Logger::getRootLogger(), "Thread");

  bool CreateThreadFunction(void (*func)(void*), void *arg) {
    ThreadArgument *argument = new ThreadArgument(func, arg);
    /* ThreadLock.lock(); */
    try {
      UserSwitch usw(0,0);
      // TODO. Who is going to destroy created object? Check for memory leaks.
      Glib::Thread::create(sigc::mem_fun(*argument, &ThreadArgument::thread),
                           thread_stacksize, false, false,
                           Glib::THREAD_PRIORITY_NORMAL);
    } catch (std::exception& e) {
      threadLogger.msg(ERROR, e.what());
      delete argument;
      /* ThreadLock.unlock(); */
      return false;
    };
    /* ThreadLock.unlock(); */
    return true;
  }

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

} // namespace Arc
