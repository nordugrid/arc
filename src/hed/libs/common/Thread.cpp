// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/init.h>

#include "Thread.h"
#include "Logger.h"

namespace Arc {

  static Logger threadLogger(Logger::getRootLogger(), "Thread");

  class ThreadInitializer {
  public:
    ThreadInitializer(void) {
      Glib::init();
      threadLogger.msg(INFO, "Initialize thread system");
      if (!Glib::thread_supported())
        Glib::thread_init();
    }
  };

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
  static ThreadInitializer thread_initializer;

  bool CreateThreadFunction(void (*func)(void*), void *arg) {
    ThreadArgument *argument = new ThreadArgument(func, arg);
    try {
      // TODO. Who is going to destroy created object? Check for memory leaks.
      Glib::Thread::create(sigc::mem_fun(*argument, &ThreadArgument::thread),
                           thread_stacksize, false, false,
                           Glib::THREAD_PRIORITY_NORMAL);
    } catch (std::exception& e) {
      threadLogger.msg(ERROR, e.what());
      delete argument;
      return false;
    }
    ;
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
