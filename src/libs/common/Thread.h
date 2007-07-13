#ifndef __ARC_THREAD_H__
#define __ARC_THREAD_H__

#include <glibmm/thread.h>

namespace Arc {
/** This module provides convenient helpers for Glibmm interface
 for thread management. So far it takes care of automatic initialization
 of threading environment and creation of simple detached threads. */

/** This macro behaves like function which makes thread of
 class' method. It accepts class instance and full  name of 
 method - like class::method. 'method' should not be static member
 of the class. Result is true if creation of thread succeeded. 
  Specified instance must be valid during whole lifetime of 
 thread. So probably it is safer to destroy 'instance' in 
 'method' just before exiting. */
#define CreateThreadClass(instance,method) \
{ \
    Glib::Thread* thr = NULL; \
    try { \
        thr = Glib::Thread::create(sigc::mem_fun((instance),&method), false); \
    } catch (std::exception& e) { }; \
    (thr != NULL); \
} \

/** Helper function to create simple thread.
  It takes care of all pecularities og Glib::Thread API.
 As result it runs function 'func' with argument 'arg' in a separate
 thread. 
  Returns true on success. */
bool CreateThreadFunction(void (*func)(void*), void* arg);

} // namespace Arc 

#endif /* __ARC_THREAD_H__ */
