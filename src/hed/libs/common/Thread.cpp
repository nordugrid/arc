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

#include <arc/Logger.h>
#include <arc/User.h>
#include <arc/GUID.h>

#include "Thread.h"


namespace Arc {

  static Logger threadLogger(Logger::getRootLogger(), "Thread");

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
    void Inherit(ThreadData* old);
    // Attach item to this instance
    void AddItem(const std::string& key,ThreadDataItem* item);
    // Fetch item from this instance
    ThreadDataItem* GetItem(const std::string& key);
    // Decrease counter and destroy object if 0.
    void Release(void);
  };

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
    ThreadData* data;
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
    ThreadArgument(func_t f, void *a, SimpleCounter *c, ThreadData *d)
      : arg(a),
        func(f),
        count(c),
        data(d)
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
    ThreadData* tdata = ThreadData::Get();
    if(tdata) {
      tdata->Inherit(data);
      tdata->Release();
    }
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
    ThreadData::Remove();
  }

  bool CreateThreadFunction(void (*func)(void*), void *arg, SimpleCounter* count
) {
#ifdef USE_THREAD_POOL
    if(!pool) return false;
    ThreadArgument *argument = new ThreadArgument(func, arg, count, ThreadData::Get());
    if(count) count->inc();
    pool->PushQueue(argument);
#else
    ThreadArgument *argument = new ThreadArgument(func, arg, count, ThreadData::Get());
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
      data->Release();
      //delete data;
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
    if(key.empty()) return;
    ThreadData* data = ThreadData::Get();
    if(data) {
      data->AddItem(key,this);
      data->Release();
    }
  }

  ThreadDataItem::ThreadDataItem(std::string& key) {
    Attach(key);
  }

  void ThreadDataItem::Attach(std::string& key) {
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
  }

  ThreadDataItem* ThreadDataItem::Get(const std::string& key) {
    ThreadData* data = ThreadData::Get();
    if(!data) return NULL;
    ThreadDataItem* item = data->GetItem(key);
    data->Release();
    return item;
  }

  void ThreadDataItem::Dup(void) {
    // new ThreadDataItem;
  }

  // ----------------------------------------

  void GlibThreadInitialize(void) {
    Glib::init();
    if (!Glib::thread_supported())
      Glib::thread_init();
#ifdef USE_THREAD_POOL
    if (!pool)
      data_pool = new ThreadDataPool;
      pool = new ThreadPool;
#endif
  }

} // namespace Arc

