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
std::cerr<<"RumPump"<<std::endl;
  try {
      thread_ = Glib::Thread::create(sigc::mem_fun(*this,&RunPump::Pump), false);
  } catch (std::exception& e) { };
  if(thread_ == NULL) return;
std::cerr<<"RumPump: thread created"<<std::endl;
  // Wait for context_ to be intialized
  // TODO: what to do if context never initialized
  for(;;) {
std::cerr<<"RumPump: waiting context"<<std::endl;
    if(context_) break;
    thread_->yield(); // This is simpler than condition+mutex
  };
}

RunPump& RunPump::Instance(void) {
std::cerr<<"Instance"<<std::endl;
  if((instance_ == NULL) || (mark_ != RunPumpMagic)) {
    instance_=new RunPump();
    mark_=RunPumpMagic;
  };
std::cerr<<"Instance: "<<(bool)(*instance_)<<std::endl;
  return *instance_;
}

void RunPump::Pump(void) {
  // TODO: put try/catch everythere for glibmm errors
  context_ = Glib::MainContext::create();
  // In infinite loop monitor state of children processes
  // and pump information to/from std* channels if requested
  for(;;) {
    list_lock_.lock();
std::cerr<<"list_lock acquired"<<std::endl;
    list_lock_.unlock();
    pump_lock_.lock();
std::cerr<<"starting iteration"<<std::endl;
    context_->iteration(true);
std::cerr<<"leaving iteration"<<std::endl;
    pump_lock_.unlock();
    thread_->yield();
  };
}

void RunPump::Add(Run* r) {
std::cerr<<"Add 0"<<std::endl;
  if(!r) return;
std::cerr<<"Add 1"<<std::endl;
  if(!(*r)) return;
std::cerr<<"Add 2"<<std::endl;
  if(!(*this)) return;
std::cerr<<"Add 3"<<std::endl;
  // Take full control over context
std::cerr<<"Add: acquiring list_lock"<<std::endl;
  list_lock_.lock();
  context_->wakeup();
std::cerr<<"Add: acquiring pump_lock"<<std::endl;
  pump_lock_.lock();
  // Add sources to context
  if(r->stdout_str_)
    r->stdout_conn_=context_->signal_io().connect(sigc::mem_fun(*r,&Run::stdout_handler),r->stdout_, Glib::IO_IN | Glib::IO_HUP);
  if(r->stderr_str_)
    r->stderr_conn_=context_->signal_io().connect(sigc::mem_fun(*r,&Run::stderr_handler),r->stderr_, Glib::IO_IN | Glib::IO_HUP);
  if(r->stdin_str_)
    r->stdin_conn_ =context_->signal_io().connect(sigc::mem_fun(*r,&Run::stdin_handler), r->stdin_, Glib::IO_OUT | Glib::IO_HUP);
  r->child_conn_ =context_->signal_child_watch().connect(sigc::mem_fun(*r,&Run::child_handler),r->pid_);
  if(r->child_conn_.empty()) std::cerr<<"connect for signal_child_watch failed"<<std::endl;
std::cerr<<"Add: added sources"<<std::endl;
  pump_lock_.unlock();
  list_lock_.unlock();
}

void RunPump::Remove(Run* r) {
std::cerr<<"Remove"<<std::endl;
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
std::cerr<<"Removed"<<std::endl;
  pump_lock_.unlock();
  list_lock_.unlock();
}

Run::Run(const std::string& cmdline):pid_(0),stdout_str_(NULL),stderr_str_(NULL),stdin_str_(NULL),stdout_(-1),stderr_(-1),stdin_(-1),running_(false),started_(false),result_(-1),argv_(Glib::shell_parse_argv(cmdline)) {
}

Run::Run(const std::list<std::string>& argv):pid_(0),stdout_str_(NULL),stderr_str_(NULL),stdin_str_(NULL),stdout_(-1),stderr_(-1),stdin_(-1),running_(false),started_(false),result_(-1),argv_(argv) {
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
  running_=true;
std::cerr<<"Starting application"<<std::endl;
  spawn_async_with_pipes(".",argv_,Glib::SpawnFlags(0),sigc::slot<void>(),&pid_,&stdin_,&stdout_,&stderr_);
std::cerr<<"Started application"<<std::endl;
  RunPump::Instance().Add(this);
}

void Run::Kill(int timeout) {
  if(!running_) return;
#ifndef WIN32
  if(timeout > 0) {
    // Kill softly
std::cerr<<"kill "<<pid_<<" SIGTERM"<<std::endl;
    ::kill(pid_,SIGTERM);
    Wait(5);
  };
  if(!running_) return;
  // Kill with no merci
  running_=false;
std::cerr<<"kill "<<pid_<<" SIGKILL"<<std::endl;
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
std::cerr<<"child_handler: pid="<<pid<<", result="<<result<<std::endl;
  CloseStdout();
  CloseStderr();
  CloseStdin();
  running_=false;
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
  ::sleep(timeout);
  return (Result() != -1);
}

}

