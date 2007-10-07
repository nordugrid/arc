#include <glibmm.h>

#include <iostream>

#include <arc/Thread.h>
#ifndef WIN32
#include <sys/types.h>
#include <signal.h>
#endif

#include "Run.h"

namespace Arc {

class RunPump {
 friend class Run;
 private:
  static RunPump* instance_;
  static int mark_;
#define RunPumpMagic (0xA73E771F)
  //std::list<Run*> runs_;
  Glib::Mutex list_lock_;
  Glib::Mutex pump_lock_;
  Glib::RefPtr<Glib::MainContext> context_;
  Glib::Thread* thread_;
  RunPump(void);
  ~RunPump(void);
  static RunPump& Instance(void);
  operator bool(void) { return (bool)context_; };
  bool operator!(void) { return !(bool)context_; };
  void Pump(void);
  void Add(Run* r);
  void Remove(Run* r);
};

RunPump* RunPump::instance_ = NULL;
int RunPump::mark_ = ~RunPumpMagic;

RunPump::RunPump(void):thread_(NULL) {
  try {
      thread_ = Glib::Thread::create(sigc::mem_fun(*this,&RunPump::Pump), false);
  } catch (std::exception& e) { };
  if(thread_ == NULL) return;
  // Wait for context_ to be intialized
  // TODO: what to do if context never initialized
  for(;;) {
    if(context_) break;
    thread_->yield(); // This is simpler than condition+mutex
  };
}

RunPump& RunPump::Instance(void) {
  if((instance_ == NULL) || (mark_ != RunPumpMagic)) {
    instance_=new RunPump();
    mark_=RunPumpMagic;
  };
  return *instance_;
}

void RunPump::Pump(void) {
  // TODO: put try/catch everythere for glibmm errors
  context_ = Glib::MainContext::create();
  // In infinite loop monitor state of children processes
  // and pump information to/from std* channels if requested
  //context_->acquire();
  for(;;) {
    list_lock_.lock();
    list_lock_.unlock();
    pump_lock_.lock();
    context_->iteration(true);
    pump_lock_.unlock();
    thread_->yield();
  };
}

void RunPump::Add(Run* r) {
  if(!r) return;
  if(!(*r)) return;
  if(!(*this)) return;
  // Take full control over context
  list_lock_.lock();
  context_->wakeup();
  pump_lock_.lock();
  // Add sources to context
  if(r->stdout_str_)
    r->stdout_conn_=context_->signal_io().connect(sigc::mem_fun(*r,&Run::stdout_handler),r->stdout_, Glib::IO_IN | Glib::IO_HUP);
  if(r->stderr_str_)
    r->stderr_conn_=context_->signal_io().connect(sigc::mem_fun(*r,&Run::stderr_handler),r->stderr_, Glib::IO_IN | Glib::IO_HUP);
  if(r->stdin_str_)
    r->stdin_conn_ =context_->signal_io().connect(sigc::mem_fun(*r,&Run::stdin_handler), r->stdin_, Glib::IO_OUT | Glib::IO_HUP);
  r->child_conn_=context_->signal_child_watch().connect(sigc::mem_fun(*r,&Run::child_handler),r->pid_);
  //if(r->child_conn_.empty()) std::cerr<<"connect for signal_child_watch failed"<<std::endl;
  pump_lock_.unlock();
  list_lock_.unlock();
}

void RunPump::Remove(Run* r) {
  if(!r) return;
  if(!(*r)) return;
  if(!(*this)) return;
  // Take full control over context
  list_lock_.lock();
  context_->wakeup();
  pump_lock_.lock();
  // Disconnect sources from context
  r->stdout_conn_.disconnect();
  r->stderr_conn_.disconnect();
  r->stdin_conn_.disconnect();
  r->child_conn_.disconnect();
  pump_lock_.unlock();
  list_lock_.unlock();
}

Run::Run(const std::string& cmdline):stdout_(-1),stderr_(-1),stdin_(-1),stdout_str_(NULL),stderr_str_(NULL),stdin_str_(NULL),pid_(0),argv_(Glib::shell_parse_argv(cmdline)),started_(false),running_(false),result_(-1) {
}

Run::Run(const std::list<std::string>& argv):stdout_(-1),stderr_(-1),stdin_(-1),stdout_str_(NULL),stderr_str_(NULL),stdin_str_(NULL),pid_(0),argv_(argv),started_(false),running_(false),result_(-1) {
}

Run::~Run(void) {
  if(!(*this)) return;
  Kill(0);
  CloseStdout();
  CloseStderr();
  CloseStdin();
  RunPump::Instance().Remove(this);
}

bool Run::Start(void) {
  if(started_) return false;
  if(argv_.size() < 1) return false;
  // TODO: Windows paths
  running_=true; started_=true;
  spawn_async_with_pipes(".",argv_,Glib::SpawnFlags(Glib::SPAWN_DO_NOT_REAP_CHILD),sigc::slot<void>(),&pid_,&stdin_,&stdout_,&stderr_);
  RunPump::Instance().Add(this);
}

void Run::Kill(int timeout) {
  if(!running_) return;
#ifndef WIN32
  if(timeout > 0) {
    // Kill softly
    ::kill(pid_,SIGTERM);
    Wait(5);
  };
  if(!running_) return;
  // Kill with no merci
  running_=false;
  ::kill(pid_,SIGKILL);
  pid_=0;
#else
#error Must implement kill functionality for Windows
#endif
}

bool Run::stdout_handler(Glib::IOCondition cond) {
  if(stdout_str_) {
    char buf[256];
    int l = ReadStdout(0,buf,sizeof(buf));
    if((l == 0) || (l == -1)) {
      CloseStdout(); return false;
    } else {
      stdout_str_->append(buf,l);
    };
  } else {
    // Event shouldn't happen if not expected

  };
  return true;
}

bool Run::stderr_handler(Glib::IOCondition cond) {
  if(stderr_str_) {
    char buf[256];
    int l = ReadStderr(0,buf,sizeof(buf));
    if((l == 0) || (l == -1)) {
      CloseStderr(); return false;
    } else {
      stderr_str_->append(buf,l);
    };
  } else {
    // Event shouldn't happen if not expected

  };
  return true;
}

bool Run::stdin_handler(Glib::IOCondition cond) {
  if(stdin_str_) {
    if(stdin_str_->length() == 0) {
      CloseStdin(); stdin_str_=NULL;
    } else {
      int l = WriteStdin(0,stdin_str_->c_str(),stdin_str_->length());
      if(l == -1) {
        CloseStdin(); return false;
      } else {
        // Not very effective
        *stdin_str_=stdin_str_->substr(l);
      };
    };
  } else {
    // Event shouldn't happen if not expected

  };
  return true;
}

void Run::child_handler(Glib::Pid pid,int result) {
  CloseStdout();
  CloseStderr();
  CloseStdin();
  lock_.lock();
  cond_.signal();
  result_=(result >> 8); // Why ?
  running_=false;
  lock_.unlock();
}

void Run::CloseStdout(void) {
  if(stdout_ != -1) ::close(stdout_); stdout_==-1;
}

void Run::CloseStderr(void) {
  if(stderr_ != -1) ::close(stderr_); stderr_=-1;
}

void Run::CloseStdin(void) {
  if(stdin_ != -1) ::close(stdin_); stdin_=-1;
}

int Run::ReadStdout(int timeout,char* buf,int size) {
  if(stdout_ == -1) return -1;
  // TODO: do it through context for timeout
  return ::read(stdout_,buf,size);
}

int Run::ReadStderr(int timeout,char* buf,int size) {
  if(stderr_ == -1) return -1;
  // TODO: do it through context for timeout
  return ::read(stderr_,buf,size);
}

int Run::WriteStdin(int timeout,const char* buf,int size) {
  if(stdin_ == -1) return -1;
  // TODO: do it through context for timeout
  return write(stdin_,buf,size);
}

bool Run::Wait(int timeout) {
  if(!started_) return false;
  if(!running_) return true;
  Glib::TimeVal till;
  till.assign_current_time(); till+=timeout;
  lock_.lock();
  while(running_) {
    Glib::TimeVal t; t.assign_current_time(); t.subtract(till);
    if(!t.negative()) break;
    cond_.timed_wait(lock_,till);
  };
  lock_.unlock();
  return (!running_);
}

}

