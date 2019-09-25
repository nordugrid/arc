// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#define USE_THREAD_POOL
#define USE_THREAD_DATA

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <stdexcept>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <glibmm/init.h>

#ifdef USE_THREAD_POOL
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <arc/Logger.h>
#include <arc/User.h>
#include <arc/GUID.h>

#include "Thread.h"


namespace Arc {

  static Logger threadLogger(Logger::getRootLogger(), "Thread");

  static Glib::Thread* ThreadCreate(const sigc::slot< void >& slot,
                                    unsigned long stack_size,
                                    bool joinable,
                                    bool bound,
                                    Glib::ThreadPriority priority) {
    UserSwitch usw(0,0);
    // Create new thread with all sinals blocked. Each thread should unblock signals it handles.
    sigset_t newsig; sigfillset(&newsig);
    sigset_t oldsig; sigemptyset(&oldsig);
    if(pthread_sigmask(SIG_BLOCK,&newsig,&oldsig) != 0)
      throw std::runtime_error("Failed to block signals");
    Glib::Thread* new_thread = NULL;
    try {
      new_thread = Glib::Thread::create(slot, stack_size, joinable, bound, priority);
    } catch(...) {
      pthread_sigmask(SIG_SETMASK,&oldsig,NULL);
      throw;
    };
    pthread_sigmask(SIG_SETMASK,&oldsig,NULL);
    return new_thread;
  }

#ifdef USE_THREAD_DATA
  class ThreadDataPool;

  class ThreadData {
  friend class ThreadDataPool;
  private:
    std::map<std::string,ThreadDataItem*> items_;
    typedef std::map<std::string,ThreadDataItem*>::iterator items_iterator;
    typedef std::pair<std::string,ThreadDataItem*> items_pair;
    Glib::Mutex lock_;
    // This counter is needed because due to delayed thread creation
    // parent instance may be already destroyed while child is not yet
    // created. Another solution would be to do Inherit() in parent thread.
    // but then interface of ThreadDataItem would become too complicated.
    int usage_count;
    ThreadData(void);
    ~ThreadData(void); // Do not call this
    void Acquire(void);
  public:
    // Get ThreadData instance of current thread
    static ThreadData* Get(void);
    // Destroy ThreadData instance of current thread
    static void Remove(void);
    // Copy items from another instance (uses Dup method)
    void Inherit(ThreadData* parent);
    // Attach item to this instance
    void AddItem(const std::string& key,ThreadDataItem* item);
    // Fetch item from this instance
    ThreadDataItem* GetItem(const std::string& key);
    // Decrease counter and destroy object if 0.
    void Release(void);
  };
#endif

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
    Thread* thr;
    SimpleCounter* count;
#ifdef USE_THREAD_DATA
    ThreadData* data;
#endif
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

    ThreadArgument(
      func_t f,
      void *a,
      SimpleCounter *c,
#ifdef USE_THREAD_DATA
      ThreadData *d
#endif
    )
      : arg(a),
        func(f),
        thr(NULL),
        count(c)
#ifdef USE_THREAD_DATA
        ,data(d)
#endif
#ifdef USE_THREAD_POOL
        ,resource(NULL)
#endif
    {}

    ThreadArgument(
      Thread* t,
      SimpleCounter *c,
#ifdef USE_THREAD_DATA
      ThreadData *d
#endif
    )
      : arg(NULL),
        func(NULL),
        thr(t),
        count(c)
#ifdef USE_THREAD_DATA
        ,data(d)
#endif
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
    int Num(void) { return count; };
  };

  ThreadPool::ThreadPool(void):max_count(0),count(0) {
    // Estimating amount of available memory 
    uint64_t n_max;
    {
      // This is very estimation of size of virtual memory available for process
      // Using size of pointer (32-bit vs 64-bit architecture)
      unsigned int bits = 8*sizeof(int*);
      // Still all 64 bits can't be used for addressing.
      // Normally max adressable size is 2^48
      // Source: http://en.wikipedia.org/wiki/X86-64#Virtual_address_space_details
      if(bits > 48) bits = 48;
      // It is common to have half taken by OS
      bits = bits - 1;
      // Dividing by 2 assuming each thread will equally use 
      // stack and heap
      uint64_t n = (((uint64_t)1)<<bits)/thread_stacksize/2;
      n_max = n;
    };
    struct rlimit rl;
    if(getrlimit(RLIMIT_AS,&rl) == 0) {
      if(rl.rlim_cur != RLIM_INFINITY) {
        uint64_t n = rl.rlim_cur/thread_stacksize/2;
        // What else can we do. Application will fail on first thread
        if(n == 0) n=1;
        if(n < n_max) n_max = n;
      };
      if(rl.rlim_max != RLIM_INFINITY) {
        uint64_t n = rl.rlim_max/thread_stacksize/2;
        if(n == 0) n=1;
        if(n < n_max) n_max = n;
      };
    };
    // Just make number to fit
    if(n_max > INT_MAX) n_max = INT_MAX;
    max_count = (int)n_max-1;
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
        ThreadCreate(sigc::mem_fun(*argument,
                     &ThreadArgument::thread),
                     thread_stacksize, false, false,
                     Glib::THREAD_PRIORITY_NORMAL);
        queue.erase(queue.begin());
      } catch (Glib::Error& e) {
        threadLogger.msg(ERROR, "%s", e.what());
        argument->release();
      } catch (Glib::Exception& e) {
        threadLogger.msg(ERROR, "%s", e.what());
        argument->release();
      } catch (std::exception& e) {
        threadLogger.msg(ERROR, "%s", e.what());
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
    ThreadId::getInstance().add();
#ifdef USE_THREAD_DATA
    ThreadData* tdata = ThreadData::Get();
    if(tdata) {
      tdata->Inherit(data);
      tdata->Release();
    }
#endif
#ifdef USE_THREAD_POOL
    ThreadInc resource_;
    release();
#endif
    func_t f_temp = func;
    void *a_temp = arg;
    Thread* t_temp = thr;
    SimpleCounter *c_temp = count;
    delete this;
    try {
      if(f_temp) {
        (*f_temp)(a_temp);
      } else if(t_temp) {
        t_temp->thread();
      }
    } catch (Glib::Error& e) {
      threadLogger.msg(ERROR, "Thread exited with Glib error: %s", e.what());
    } catch (Glib::Exception& e) {
      threadLogger.msg(ERROR, "Thread exited with Glib exception: %s", e.what());
    } catch (std::exception& e) {
      threadLogger.msg(ERROR, "Thread exited with generic exception: %s", e.what());
    };
    if(c_temp) c_temp->dec();
#ifdef USE_THREAD_DATA
    ThreadData::Remove();
#endif
    ThreadId::getInstance().remove();
  }

  ThreadId& ThreadId::getInstance() {
    static ThreadId* id = new ThreadId();
    return *id;
  }

  ThreadId::ThreadId(): thread_no(0) {}

  void ThreadId::add() {
    Glib::Mutex::Lock lock(mutex);
    if (thread_no == ULONG_MAX) thread_no = 0;
    thread_ids[(size_t)(void*)Glib::Thread::self()] = ++thread_no;
  }

  void ThreadId::remove() {
    Glib::Mutex::Lock lock(mutex);
    thread_ids.erase((size_t)(void*)Glib::Thread::self());
  }

  unsigned long int ThreadId::get() {
    Glib::Mutex::Lock lock(mutex);
    size_t id = (size_t)(void*)Glib::Thread::self();
    if (thread_ids.count(id) == 0) return id;
    return thread_ids[id];
  }

  bool CreateThreadFunction(void (*func)(void*), void *arg, SimpleCounter* count
) {
#ifdef USE_THREAD_POOL
    if(!pool) return false;
#ifdef USE_THREAD_DATA
    ThreadArgument *argument = new ThreadArgument(func, arg, count, ThreadData::Get());
#else
    ThreadArgument *argument = new ThreadArgument(func, arg, count);
#endif
    if(count) count->inc();
    pool->PushQueue(argument);
#else
#ifdef USE_THREAD_DATA
    ThreadArgument *argument = new ThreadArgument(func, arg, count, ThreadData::Get());
#else
    ThreadArgument *argument = new ThreadArgument(func, arg, count);
#endif
    if(count) count->inc();
    try {
      ThreadCreate(sigc::mem_fun(*argument, &ThreadArgument::thread),
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

  bool Thread::start(SimpleCounter* count) {
#ifdef USE_THREAD_POOL

    if(!pool) return false;
#ifdef USE_THREAD_DATA
    ThreadArgument *argument = new ThreadArgument(this, count, ThreadData::Get());
#else
    ThreadArgument *argument = new ThreadArgument(this, count);
#endif
    if(count) count->inc();
    pool->PushQueue(argument);

#else

#ifdef USE_THREAD_DATA
    ThreadArgument *argument = new ThreadArgument(this, count, ThreadData::Get());
#else
    ThreadArgument *argument = new ThreadArgument(this, count);
#endif
    if(count) count->inc();
    try {
      ThreadCreate(sigc::mem_fun(*argument, &ThreadArgument::thread),
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
      thread = ThreadCreate(sigc::mem_fun(*argument, &ThreadArgument::thread),
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


  // ----------------------------------------

  SimpleCounter::~SimpleCounter(void) {
    /* race condition ? */
    lock_.lock();
    count_ = 0;
    cond_.broadcast();
    lock_.unlock();
  }

  int SimpleCounter::inc(void) {
    lock_.lock();
    ++count_;
    cond_.broadcast();
    int r = count_;
    lock_.unlock();
    return r;
  }

  int SimpleCounter::dec(void) {
    lock_.lock();
    if(count_ > 0) --count_;
    cond_.broadcast();
    int r = count_;
    lock_.unlock();
    return r;
  }

  int SimpleCounter::get(void) const {
    Glib::Mutex& vlock = const_cast<Glib::Mutex&>(lock_);
    vlock.lock();
    int r = count_;
    vlock.unlock();
    return r;
  }

  int SimpleCounter::set(int v) {
    lock_.lock();
    count_ = v;
    cond_.broadcast();
    int r = count_;
    lock_.unlock();
    return r;
  }

  void SimpleCounter::wait(void) const {
    Glib::Mutex& vlock = const_cast<Glib::Mutex&>(lock_);
    Glib::Cond& vcond = const_cast<Glib::Cond&>(cond_);
    vlock.lock();
    while (count_ > 0) vcond.wait(vlock);
    vlock.unlock();
  }

  bool SimpleCounter::wait(int t) const {
    if(t < 0) { wait(); return true; }
    Glib::Mutex& vlock = const_cast<Glib::Mutex&>(lock_);
    Glib::Cond& vcond = const_cast<Glib::Cond&>(cond_);
    vlock.lock();
    Glib::TimeVal etime;
    etime.assign_current_time();
    etime.add_milliseconds(t);
    bool res(true);
    while (count_ > 0) {
      res = vcond.timed_wait(vlock, etime);
      if (!res) break;
    }
    vlock.unlock();
    return res;
  }

  // ----------------------------------------

  void SharedMutex::add_shared_lock(void) {
    shared_list::iterator s = shared_.find(Glib::Thread::self());
    if(s != shared_.end()) {
      ++(s->second);
    } else {
      shared_[Glib::Thread::self()] = 1;
    };
  }

  void SharedMutex::remove_shared_lock(void) {
    shared_list::iterator s = shared_.find(Glib::Thread::self());
    if(s != shared_.end()) {
      --(s->second);
      if(!(s->second)) {
        shared_.erase(s);
      };
    };
  }

  bool SharedMutex::have_shared_lock(void) {
    if(shared_.size() >= 2) return true;
    if(shared_.size() == 1) {
      if(shared_.begin()->first != Glib::Thread::self()) return true;
    };
    return false;
  }

  void SharedMutex::lockShared(void) {
    lock_.lock();
    while(have_exclusive_lock()) {
      cond_.wait(lock_);
    };
    add_shared_lock();
    lock_.unlock();
  };

  void SharedMutex::unlockShared(void) {
    lock_.lock();
    remove_shared_lock();
    cond_.broadcast();
    lock_.unlock();
  };

  void SharedMutex::lockExclusive(void) {
    lock_.lock();
    while(have_exclusive_lock() || have_shared_lock()) {
      cond_.wait(lock_);
    };
    ++exclusive_;
    thread_ = Glib::Thread::self();
    lock_.unlock();
  }

  void SharedMutex::unlockExclusive(void) {
    lock_.lock();
    if(thread_ == Glib::Thread::self()) {
      if(exclusive_) --exclusive_;
      if(!exclusive_) thread_ = NULL;
    };
    cond_.broadcast();
    lock_.unlock();
  }

  // ----------------------------------------

  ThreadedPointerBase::~ThreadedPointerBase(void) {
    //if (ptr && !released) delete ptr;
  }

  ThreadedPointerBase::ThreadedPointerBase(void *p)
    : cnt_(0),
      ptr_(p),
      released_(false) {
    add();
  }

  ThreadedPointerBase* ThreadedPointerBase::add(void) {
    Glib::Mutex::Lock lock(lock_);
    ++cnt_;
    cond_.broadcast();
    return this;
  }

  void* ThreadedPointerBase::rem(void) {
    Glib::Mutex::Lock lock(lock_);
    cond_.broadcast();
    if (--cnt_ == 0) {
      void* p = released_?NULL:ptr_;
      lock.release();
      delete this;
      return p;
    }
    return NULL;
  }

  // ----------------------------------------

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

  // ----------------------------------------

#ifdef USE_THREAD_DATA
  class ThreadDataPool {
  private:
    std::map<Glib::Thread*,ThreadData*> datas_;
    typedef std::map<Glib::Thread*,ThreadData*>::iterator datas_iterator;
    typedef std::pair<Glib::Thread*,ThreadData*> datas_pair;
    Glib::Mutex lock_;
    ~ThreadDataPool(void);
  public:
    ThreadDataPool(void);
    ThreadData* GetData(void);
    void RemoveData(void);
  };

  static ThreadDataPool* data_pool = NULL;

  ThreadDataPool::ThreadDataPool(void) {
  }

  ThreadData* ThreadDataPool::GetData(void) {
    ThreadData* data = NULL;
    Glib::Thread* self = Glib::Thread::self();
    lock_.lock();
    datas_iterator d = datas_.find(self);
    if(d == datas_.end()) {
      data = new ThreadData;
      d = datas_.insert(datas_.end(),datas_pair(self,data));
    } else {
      data = d->second;
    };
    lock_.unlock();
    return data;
  }

  void ThreadDataPool::RemoveData(void) {
    Glib::Thread* self = Glib::Thread::self();
    lock_.lock();
    datas_iterator d = datas_.find(self);
    if(d != datas_.end()) {
      ThreadData* data = d->second;
      datas_.erase(d);
      lock_.unlock();
      data->Release();
      //delete data;
      return;
    }
    lock_.unlock();
  }

  void ThreadData::Inherit(ThreadData* parent) {
    if(!parent) return;
    parent->lock_.lock();
    for(items_iterator d = parent->items_.begin();d != parent->items_.end();++d) {
      d->second->Dup();
    };
    parent->lock_.unlock();
    parent->Release();
  }

  ThreadData* ThreadData::Get(void) {
    if(!data_pool) return NULL;
    ThreadData* data = data_pool->GetData();
    if(data) data->Acquire();
    return data;
  }

  void ThreadData::Remove(void) {
    if(!data_pool) return;
    data_pool->RemoveData();
  }

  ThreadData::ThreadData(void):usage_count(1) {
  }

  void ThreadData::Acquire(void) {
    lock_.lock();
    ++usage_count;
    lock_.unlock();
  }

  void ThreadData::Release(void) {
    lock_.lock();
    if(usage_count) --usage_count;
    if(!usage_count) {
      delete this;
    } else {
      lock_.unlock();
    }
  }

  ThreadData::~ThreadData(void) {
    //lock_.lock();
    for(items_iterator i = items_.begin(); i != items_.end(); ++i) {
      delete i->second;
    };
    lock_.unlock();
  }
 
  void ThreadData::AddItem(const std::string& key,ThreadDataItem* item) {
    lock_.lock();
    items_iterator i = items_.find(key);
    if(i != items_.end()) {
      if(i->second != item) {
        delete i->second;
        i->second = item;
      };
    } else {
      i = items_.insert(items_.end(),items_pair(key,item));
    };
    lock_.unlock();
  }

  ThreadDataItem* ThreadData::GetItem(const std::string& key) {
    ThreadDataItem* item = NULL;
    lock_.lock();
    items_iterator i = items_.find(key);
    if(i != items_.end()) item = i->second;
    lock_.unlock();
    return item;
  }
#endif

  ThreadDataItem::ThreadDataItem(void) {
  }

  ThreadDataItem::ThreadDataItem(const ThreadDataItem& it) {
    // Never happens
  }

  ThreadDataItem::~ThreadDataItem(void) {
    // Called by pool
  }

  ThreadDataItem::ThreadDataItem(const std::string& key) {
    Attach(key);
  }

  void ThreadDataItem::Attach(const std::string& key) {
#ifdef USE_THREAD_DATA
    if(key.empty()) return;
    ThreadData* data = ThreadData::Get();
    if(data) {
      data->AddItem(key,this);
      data->Release();
    }
#endif
  }

  ThreadDataItem::ThreadDataItem(std::string& key) {
#ifdef USE_THREAD_DATA
    Attach(key);
#endif
  }

  void ThreadDataItem::Attach(std::string& key) {
#ifdef USE_THREAD_DATA
    ThreadData* data = ThreadData::Get();
    if(!data) return;
    if(key.empty()) {
      for(;;) {
        key = UUID();
        if(!(data->GetItem(key))) break;
      };
    };
    data->AddItem(key,this);
    data->Release();
#endif
  }

  ThreadDataItem* ThreadDataItem::Get(const std::string& key) {
#ifdef USE_THREAD_DATA
    ThreadData* data = ThreadData::Get();
    if(!data) return NULL;
    ThreadDataItem* item = data->GetItem(key);
    data->Release();
    return item;
#else
    return NULL;
#endif
  }

  void ThreadDataItem::Dup(void) {
    // new ThreadDataItem;
  }

  // ----------------------------------------

  void GlibThreadInitialize(void) {
    Glib::init();
    if (!Glib::thread_supported()) Glib::thread_init();
#ifdef USE_THREAD_POOL
    if (!pool) {
#ifdef USE_THREAD_DATA
      data_pool = new ThreadDataPool;
#endif
      pool = new ThreadPool;
    }
#endif
  }


  void ThreadInitializer::forceReset(void) {
    // This function is deprecated and its body removed because
    // there is no safe way to reset locks after call to fork().
  }

  void ThreadInitializer::waitExit(void) {
#ifdef USE_THREAD_POOL
    while(pool->Num() > 0) {
      sleep(1);
    }
#endif
  }

} // namespace Arc

