// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>

#include <iostream>

#include <glibmm.h>

#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/User.h>

#include "Run.h"


namespace Arc {

#define SAFE_DISCONNECT(CONNECTOR) { \
    try { \
      (CONNECTOR).disconnect(); \
    } catch (Glib::Exception& e) { \
    } catch (std::exception& e) { \
    }; \
}

  class RunPump {
    friend class Run;
  private:
    static Glib::Mutex instance_lock_;
    static RunPump *instance_;
    static unsigned int mark_;
#define RunPumpMagic (0xA73E771F)
    class Abandoned {
    friend class RunPump;
    private:
      sigc::connection child_conn_;
      Glib::Pid pid_;
    public:
      Abandoned(Glib::Pid pid,sigc::connection child_conn):child_conn_(child_conn),pid_(pid) { };
    };
    std::list<Abandoned> abandoned_;
    //std::list<Run*> runs_;
    Glib::Mutex abandoned_lock_;
    Glib::Mutex list_lock_;
    Glib::Mutex pump_lock_;
    Glib::RefPtr<Glib::MainContext> context_;
    Glib::Thread *thread_;
    RunPump(void);
    ~RunPump(void);
    static RunPump& Instance(void);
    operator bool(void) {
      return (bool)context_;
    }
    bool operator!(void) {
      return !(bool)context_;
    }
    void Pump(void);
    void Add(Run *r);
    void Remove(Run *r);
    void child_handler(Glib::Pid pid, int result);
  };

  Glib::Mutex RunPump::instance_lock_;
  RunPump* RunPump::instance_ = NULL;
  unsigned int RunPump::mark_ = ~RunPumpMagic;

  class Pid {
  private:
    Glib::Pid p_;
  public:
    Pid(void):p_(0) { }; 
    Pid(Glib::Pid p):p_(p) { }; 
    operator Glib::Pid(void) { return p_; };
    Glib::Pid pid(void) { return p_; };
    Glib::Pid operator=(Glib::Pid p) { return (p_=p); };
  };

  class RunInitializerArgument {
  private:
    void *arg_;
    void (*func_)(void*);
    UserSwitch* usw_;
  public:
    RunInitializerArgument(void(*func)(void*), void *arg, UserSwitch* usw)
      : arg_(arg),
        func_(func),
        usw_(usw) {}
    void Run(void);
  };

  void RunInitializerArgument::Run(void) {
    void *arg = arg_;
    void (*func)(void*) = func_;
    if(usw_) delete usw_;
    delete this;
    // To leave clean environment reset all signals.
    // Otherwise we may get some signals non-intentionally ignored.
    // Glib takes care of open handles.
#ifdef SIGRTMIN
    for(int n = SIGHUP; n < SIGRTMIN; ++n)
#else
    // At least reset all signals whose numbers are well defined
    for(int n = SIGHUP; n < SIGTERM; ++n)
#endif
      signal(n,SIG_DFL);
    if (!func)
      return;
    (*func)(arg);
    return;
  }

  RunPump::RunPump(void)
    : context_(NULL),
      thread_(NULL) {
    try {
      thread_ = Glib::Thread::create(sigc::mem_fun(*this, &RunPump::Pump), false);
    } catch (Glib::Exception& e) {} catch (std::exception& e) {}
    ;
    if (thread_ == NULL)
      return;
    // Wait for context_ to be intialized
    // TODO: what to do if context never initialized
    for (;;) {
      if (context_)
        break;
      thread_->yield(); // This is simpler than condition+mutex
    }
  }

  RunPump& RunPump::Instance(void) {
    instance_lock_.lock();
    if ((instance_ == NULL) || (mark_ != RunPumpMagic)) {
      instance_ = new RunPump();
      mark_ = RunPumpMagic;
    }
    instance_lock_.unlock();
    return *instance_;
  }

  void RunPump::Pump(void) {
    // TODO: put try/catch everythere for glibmm errors
    try {
      context_ = Glib::MainContext::create();
      // In infinite loop monitor state of children processes
      // and pump information to/from std* channels if requested
      //context_->acquire();
      for (;;) {
        list_lock_.lock();
        //      sleep(1);
        list_lock_.unlock();
        pump_lock_.lock();
        bool dispatched = context_->iteration(true);
        for(std::list<Abandoned>::iterator a = abandoned_.begin();
                            a != abandoned_.end();) {
          if(a->pid_ == 0) {
            SAFE_DISCONNECT(a->child_conn_);
            a = abandoned_.erase(a);
          } else {
            ++a;
          }
        }
        pump_lock_.unlock();
        thread_->yield();
        if (!dispatched)
          sleep(1);
      }
    } catch (Glib::Exception& e) {} catch (std::exception& e) {}
    ;
  }

  void RunPump::Add(Run *r) {
    if (!r)
      return;
    if (!(*r))
      return;
    if (!(*this))
      return;
    // Take full control over context
    list_lock_.lock();
    while (true) {
      context_->wakeup();
      if (pump_lock_.trylock())
        break;
      sleep(1);
    }
    try {
      // Add sources to context
      if (r->stdout_str_ && !(r->stdout_keep_))
        r->stdout_conn_ = context_->signal_io().connect(sigc::mem_fun(*r, &Run::stdout_handler), r->stdout_, Glib::IO_IN | Glib::IO_HUP);
      if (r->stderr_str_ && !(r->stderr_keep_))
        r->stderr_conn_ = context_->signal_io().connect(sigc::mem_fun(*r, &Run::stderr_handler), r->stderr_, Glib::IO_IN | Glib::IO_HUP);
      if (r->stdin_str_ && !(r->stdin_keep_))
        r->stdin_conn_ = context_->signal_io().connect(sigc::mem_fun(*r, &Run::stdin_handler), r->stdin_, Glib::IO_OUT | Glib::IO_HUP);
#ifdef HAVE_GLIBMM_CHILDWATCH
      r->child_conn_ = context_->signal_child_watch().connect(sigc::mem_fun(*r, &Run::child_handler), r->pid_->pid());
      //if(r->child_conn_.empty()) std::cerr<<"connect for signal_child_watch failed"<<std::endl;
#endif
    } catch (Glib::Exception& e) {} catch (std::exception& e) {}
    ;
    pump_lock_.unlock();
    list_lock_.unlock();
  }

  void RunPump::Remove(Run *r) {
    if (!r)
      return;
    if (!(*r))
      return;
    if (!(*this))
      return;
    // Take full control over context
    list_lock_.lock();
    while (true) {
      context_->wakeup();
      if (pump_lock_.trylock())
        break;
      sleep(1);
    }
    // Disconnect sources from context
    SAFE_DISCONNECT(r->stdout_conn_);
    SAFE_DISCONNECT(r->stderr_conn_);
    SAFE_DISCONNECT(r->stdin_conn_);
    SAFE_DISCONNECT(r->child_conn_);
    if(r->running_) {
#ifdef HAVE_GLIBMM_CHILDWATCH
      abandoned_.push_back(Abandoned(r->pid_->pid(),context_->signal_child_watch().connect(sigc::mem_fun(*this,&RunPump::child_handler), r->pid_->pid())));
#endif
      r->running_ = false;
    };
    pump_lock_.unlock();
    list_lock_.unlock();
  }

  void RunPump::child_handler(Glib::Pid pid, int result) {
    abandoned_lock_.lock();
    for(std::list<Abandoned>::iterator a = abandoned_.begin();
                        a != abandoned_.end();++a) {
      if(a->pid_ == pid) {
        a->pid_ = 0;
        break;
      }
    }
    abandoned_lock_.unlock();
  }

  Run::Run(const std::string& cmdline)
    : working_directory("."),
      stdout_(-1),
      stderr_(-1),
      stdin_(-1),
      stdout_str_(NULL),
      stderr_str_(NULL),
      stdin_str_(NULL),
      stdout_keep_(false),
      stderr_keep_(false),
      stdin_keep_(false),
      pid_(NULL),
      argv_(Glib::shell_parse_argv(cmdline)),
      initializer_func_(NULL),
      initializer_arg_(NULL),
      kicker_func_(NULL),
      started_(false),
      running_(false),
      abandoned_(false),
      result_(-1) {
    pid_ = new Pid;
  }

  Run::Run(const std::list<std::string>& argv)
    : working_directory("."),
      stdout_(-1),
      stderr_(-1),
      stdin_(-1),
      stdout_str_(NULL),
      stderr_str_(NULL),
      stdin_str_(NULL),
      pid_(0),
      argv_(argv),
      initializer_func_(NULL),
      initializer_arg_(NULL),
      kicker_func_(NULL),
      started_(false),
      running_(false),
      abandoned_(false),
      result_(-1) {
    pid_ = new Pid;
  }

  Run::~Run(void) {
    if(*this) {
      if(!abandoned_) Kill(0);
      CloseStdout();
      CloseStderr();
      CloseStdin();
      RunPump::Instance().Remove(this);
    };
  }

  bool Run::Start(void) {
    if (started_)
      return false;
    if (argv_.size() < 1)
      return false;
    RunPump& pump = RunPump::Instance();
    UserSwitch* usw = NULL;
    RunInitializerArgument *arg = NULL;
    try {
      running_ = true;
      Glib::Pid pid;
      // Locking user switching to make sure fork is 
      // is done with proper uid
      usw = new UserSwitch(0,0);
      arg = new RunInitializerArgument(initializer_func_, initializer_arg_, usw);
      spawn_async_with_pipes(working_directory, argv_,
                             Glib::SpawnFlags(Glib::SPAWN_DO_NOT_REAP_CHILD),
                             sigc::mem_fun(*arg, &RunInitializerArgument::Run),
                             &pid,
                             stdin_keep_  ? NULL : &stdin_,
                             stdout_keep_ ? NULL : &stdout_,
                             stderr_keep_ ? NULL : &stderr_);
      *pid_ = pid;
      if (!stdin_keep_)
        fcntl(stdin_, F_SETFL, fcntl(stdin_, F_GETFL) | O_NONBLOCK);
      if (!stdout_keep_)
        fcntl(stdout_, F_SETFL, fcntl(stdout_, F_GETFL) | O_NONBLOCK);
      if (!stderr_keep_)
        fcntl(stderr_, F_SETFL, fcntl(stderr_, F_GETFL) | O_NONBLOCK);
      started_ = true;
    } catch (Glib::Exception& e) {
      if(usw) delete usw;
      if(arg) delete arg;
      running_ = false;
      // TODO: report error
      return false;
    } catch (std::exception& e) {
      if(usw) delete usw;
      if(arg) delete arg;
      running_ = false;
      return false;
    };
    pump.Add(this);
    if(usw) delete usw;
    if(arg) delete arg;
    return true;
  }

  void Run::Kill(int timeout) {
#ifndef HAVE_GLIBMM_CHILDWATCH
    Wait(0);
#endif
    if (!running_)
      return;
    if (timeout > 0) {
      // Kill softly
      ::kill(pid_->pid(), SIGTERM);
      Wait(timeout);
    }
    if (!running_)
      return;
    // Kill with no merci
    ::kill(pid_->pid(), SIGKILL);
  }

  void Run::Abandon(void) {
    if(*this) {
      CloseStdout();
      CloseStderr();
      CloseStdin();
      abandoned_=true;
    }
  }

  bool Run::stdout_handler(Glib::IOCondition) {
    if (stdout_str_) {
      char buf[256];
      int l = ReadStdout(0, buf, sizeof(buf));
      if ((l == 0) || (l == -1)) {
        CloseStdout();
        return false;
      }
      else
        stdout_str_->append(buf, l);
    }
    else {
      // Event shouldn't happen if not expected

    }
    return true;
  }

  bool Run::stderr_handler(Glib::IOCondition) {
    if (stderr_str_) {
      char buf[256];
      int l = ReadStderr(0, buf, sizeof(buf));
      if ((l == 0) || (l == -1)) {
        CloseStderr();
        return false;
      }
      else
        stderr_str_->append(buf, l);
    }
    else {
      // Event shouldn't happen if not expected

    }
    return true;
  }

  bool Run::stdin_handler(Glib::IOCondition) {
    if (stdin_str_) {
      if (stdin_str_->length() == 0) {
        CloseStdin();
        stdin_str_ = NULL;
      }
      else {
        int l = WriteStdin(0, stdin_str_->c_str(), stdin_str_->length());
        if (l == -1) {
          CloseStdin();
          return false;
        }
        else
          // Not very effective
          *stdin_str_ = stdin_str_->substr(l);
      }
    }
    else {
      // Event shouldn't happen if not expected

    }
    return true;
  }

  void Run::child_handler(Glib::Pid, int result) {
    if (stdout_str_)
      for (;;)
        if (!stdout_handler(Glib::IO_IN))
          break;
    if (stderr_str_)
      for (;;)
        if (!stderr_handler(Glib::IO_IN))
          break;
    CloseStdout();
    CloseStderr();
    CloseStdin();
    lock_.lock();
    cond_.signal();
    // There is reference in Glib manual that 'result' is same
    // as returned by waitpid. It is not clear how it works for
    // windows but atleast for *nix we can use waitpid related
    // macros.
    if(WIFEXITED(result)) {
      result_ = WEXITSTATUS(result);
    } else {
      result_ = -1;
    }
    running_ = false;
    lock_.unlock();
    if (kicker_func_)
      (*kicker_func_)(kicker_arg_);
  }

  void Run::CloseStdout(void) {
    if (stdout_ != -1)
      ::close(stdout_);
    stdout_ = -1;
    SAFE_DISCONNECT(stdout_conn_);
  }

  void Run::CloseStderr(void) {
    if (stderr_ != -1)
      ::close(stderr_);
    stderr_ = -1;
    SAFE_DISCONNECT(stderr_conn_);
  }

  void Run::CloseStdin(void) {
    if (stdin_ != -1)
      ::close(stdin_);
    stdin_ = -1;
    SAFE_DISCONNECT(stdin_conn_);
  }

  int Run::ReadStdout(int timeout, char *buf, int size) {
    if (stdout_ == -1)
      return -1;
    // TODO: do it through context for timeout?
    pollfd fd;
    fd.fd = stdout_; fd.events = POLLIN; fd.revents = 0;
    int err = poll(&fd, 1, timeout);
    if(err <= 0) return err;
    if(!(fd.revents & POLLIN)) return -1;
    return ::read(stdout_, buf, size);
  }

  int Run::ReadStderr(int timeout, char *buf, int size) {
    if (stderr_ == -1)
      return -1;
    // TODO: do it through context for timeout
    pollfd fd;
    fd.fd = stderr_; fd.events = POLLIN; fd.revents = 0;
    int err = poll(&fd, 1, timeout);
    if(err <= 0) return err;
    if(!(fd.revents & POLLIN)) return -1;
    return ::read(stderr_, buf, size);
  }

  int Run::WriteStdin(int timeout, const char *buf, int size) {
    if (stdin_ == -1)
      return -1;
    // TODO: do it through context for timeout
    pollfd fd;
    fd.fd = stdout_; fd.events = POLLOUT; fd.revents = 0;
    int err = poll(&fd, 1, timeout);
    if(err <= 0) return err;
    if(!(fd.revents & POLLOUT)) return -1;
    return write(stdin_, buf, size);
  }

  bool Run::Running(void) {
#ifdef HAVE_GLIBMM_CHILDWATCH
    return running_;
#else
    Wait(0);
    return running_;
#endif
  }

  bool Run::Wait(int timeout) {
    if (!started_)
      return false;
    if (!running_)
      return true;
    Glib::TimeVal till;
    till.assign_current_time();
    till += timeout;
    lock_.lock();
    while (running_) {
      Glib::TimeVal t;
      t.assign_current_time();
      t.subtract(till);
#ifdef HAVE_GLIBMM_CHILDWATCH
      if (!t.negative())
        break;
      cond_.timed_wait(lock_, till);
#else
      int status;
      int r = waitpid(pid_->pid(), &status, WNOHANG);
      if (r == 0) {
        if (!t.negative())
          break;
        lock_.unlock();
        sleep(1);
        lock_.lock();
        continue;
      }
      if (r == -1) // Child lost?
        status = -1;
      else
        status = WEXITSTATUS(status);
      // Child exited
      lock_.unlock();
      child_handler(pid_->pid(), status << 8);
      lock_.lock();
#endif
    }
    lock_.unlock();
    return (!running_);
  }

  bool Run::Wait(void) {
    if (!started_)
      return false;
    if (!running_)
      return true;
    lock_.lock();
    Glib::TimeVal till;
    while (running_) {
#ifdef HAVE_GLIBMM_CHILDWATCH
      till.assign_current_time();
      till += 1; // one sec later
      cond_.timed_wait(lock_, till);
#else
      int status;
      int r = waitpid(pid_->pid(), &status, WNOHANG);
      if (r == 0) {
        lock_.unlock();
        sleep(1);
        lock_.lock();
        continue;
      }
      if (r == -1) // Child lost?
        status = -1;
      else
        status = WEXITSTATUS(status);
      // Child exited
      lock_.unlock();
      child_handler(pid_->pid(), status << 8);
      lock_.lock();
#endif
    }
    lock_.unlock();
    return (!running_);
  }

  void Run::AssignStdout(std::string& str) {
    if (!running_)
      stdout_str_ = &str;
  }

  void Run::AssignStderr(std::string& str) {
    if (!running_)
      stderr_str_ = &str;
  }

  void Run::AssignStdin(std::string& str) {
    if (!running_)
      stdin_str_ = &str;
  }

  void Run::KeepStdout(bool keep) {
    if (!running_)
      stdout_keep_ = keep;
  }

  void Run::KeepStderr(bool keep) {
    if (!running_)
      stderr_keep_ = keep;
  }

  void Run::KeepStdin(bool keep) {
    if (!running_)
      stdin_keep_ = keep;
  }

  void Run::AssignInitializer(void (*initializer_func)(void *arg), void *initializer_arg) {
    if (!running_) {
      initializer_arg_ = initializer_arg;
      initializer_func_ = initializer_func;
    }
  }

  void Run::AssignKicker(void (*kicker_func)(void *arg), void *kicker_arg) {
    if (!running_) {
      kicker_arg_ = kicker_arg;
      kicker_func_ = kicker_func;
    }
  }

}
