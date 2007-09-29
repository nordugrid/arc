#include <glibmm.h>

#include <arc/Thread.h>

namespace Arc {

class RunPump;

class Run {
 friend class RunPump;
 private:
  Run(const Run&);
  Run& operator=(Run&);
 protected:
  int stdout_;
  int stderr_;
  int stdin_;
  std::string* stdout_str_;
  std::string* stderr_str_;
  std::string* stdin_str_;
  Glib::Pid pid_;
  bool stdout_handler(Glib::IOCondition cond);
  bool stderr_handler(Glib::IOCondition cond);
  bool stdin_handler(Glib::IOCondition cond);
  void child_handler(Glib::Pid pid,int result);
  bool running_;
 public:
  Run(const std::string& cmdline);
  Run(const std::list<const std::string>& argv);
  ~Run(void);
  operator bool(void) { pid_ != 0; };
  bool operator!(void) { pid_ == 0; };
  bool Start(void);
  bool Wait(int timeout);
  int Result(void);
  bool Running(void) { return running_; };
  int ReadStdout(int timeout,char* buf,int size);
  int ReadStderr(int timeout,char* buf,int size);
  int WriteStdin(int timeout,char* buf,int size);
  void AssignStdout(std::string& str);
  void AssignStderr(std::string& str);
  void AssignStdin(std::string& str);
  void CloseStdout(void);
  void CloseStderr(void);
  void CloseStdin(void);
  void DumpStdout(void);
  void DumpStderr(void);
  void Kill(int timeout);
};

class RunPump {
 friend class Run;
 private:
  static RunPump* instance_;
  static int mark_;
#define RunPumpMagic (0xA73E771F)
  std::list<Run*> runs_;
  Glib::Mutex list_lock_;
  Glib::Mutex pump_lock_;
  Glib::RefPtr<Glib::MainContext> context_;
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

RunPump::RunPump(void) {
  //if(!CreateThreadClass(*this,RunPump::Pump)) return;
  Glib::Thread* thr = NULL;
  try {
      thr = Glib::Thread::create(sigc::mem_fun(*this,&RunPump::Pump), false);
  } catch (std::exception& e) { };
  if(thr == NULL) return;
  // wait for context_ to be intialized
  for(;;) {
    if(context_) break;
    // yield();
  };
}

RunPump& RunPump::Instance(void) {
  if((instance_ == NULL) || (mark_ != RunPumpMagic)) {
    instance_=new RunPump();
  };
  return *instance_;
}

void RunPump::Pump(void) {
  // TODO: put try/catch everythere for glibmm errors
  context_ = Glib::MainContext::create();
  // In infinite loop monitor state of children processes
  // and pump information to/from std* channels if requested
  for(;;) {
    pump_lock_.lock();
    context_->iteration(true);
    pump_lock_.unlock();
    // yield()
  };
}

bool Run::stdout_handler(Glib::IOCondition cond) {
}

bool Run::stderr_handler(Glib::IOCondition cond) {
}

bool Run::stdin_handler(Glib::IOCondition cond) {
}

void Run::child_handler(Glib::Pid pid,int result) {
}

void RunPump::Add(Run* r) {
  if(!r) return;
  if(!(*r)) return;
  if(!(*this)) return;
  list_lock_.lock();
  runs_.push_back(r);
  sigc::connection handle;
  handle = context_->signal_io().connect(sigc::mem_fun(*r,&Run::stdout_handler),r->stdout_, Glib::IO_IN | Glib::IO_HUP);
  handle = context_->signal_io().connect(sigc::mem_fun(*r,&Run::stderr_handler),r->stderr_, Glib::IO_IN | Glib::IO_HUP);
  handle = context_->signal_io().connect(sigc::mem_fun(*r,&Run::stdin_handler),r->stdin_, Glib::IO_IN | Glib::IO_HUP);
  handle = context_->signal_child_watch().connect(sigc::mem_fun(*r,&Run::child_handler),r->pid_);
  // TODO: handle has to be used for disconnection - keep it somehow
  // TODO: child_watch_handler need to be defined
  // TODO: io_handler has to be defined
  list_lock_.unlock();
  // Inform pump loop about changes
  context_->wakeup();
}

void RunPump::Remove(Run* r) {




}






Run::Run(const std::string& cmdline):pid_(0) {
  RunPump::Instance().Add(this);
}

Run::Run(const std::list<const std::string>& argv):pid_(0) {
  RunPump::Instance().Add(this);
}

Run::~Run(void) {
  if(!(*this)) return;
  if(Running()) Kill(0);
  RunPump::Instance().Remove(this);
  ::close(stdout_);
  ::close(stderr_);
  ::close(stdin_);
}

void Run::Kill(int timeout) {
  // Kill softly
  // Kill with no merci
}

}

