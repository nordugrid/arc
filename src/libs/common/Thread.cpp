#include "Thread.h"

namespace Arc {
class ThreadInitializer {
    public:
        ThreadInitializer(void) {
            if(!Glib::thread_supported()) Glib::thread_init();
        };
};

class ThreadArgument {
    public:
        void* arg;
        void (*func)(void*);
        ThreadArgument(void (*f)(void*), void* a):func(f),arg(a) { }
        ~ThreadArgument(void) { };
        void thread(void) { (*func)(arg); delete this; };
};

static ThreadInitializer thread_initializer;

bool CreateThreadFunction(void (*func)(void*), void* arg) {
    ThreadArgument* argument = new ThreadArgument(func,arg);
    try {
        Glib::Thread::create(sigc::mem_fun(*argument,&ThreadArgument::thread),
                             false);
    } catch(std::exception& e) { delete argument; };
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

};
