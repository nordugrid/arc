// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <cstdio>
// NOTE: On Solaris errno is not working properly if cerrno is included first
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <unistd.h>

#include <iostream>

#include <glibmm.h>

#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/User.h>
#include <arc/Utils.h>

#include "Run.h"
#include "Watchdog.h"

#define DUAL_CHECK_LOST_CHILD

namespace Arc {

  // This function is hackish workaround for
  // problem of child processes disappearing without
  // trace and even waitpid() can't detect that.
  static bool check_pid(pid_t p, Time& t) {
    if(p <= 0) {
      // Child PID is lost - memory corruption?
      std::cerr<<"HUGE PROBLEM: lost PID of child process: "<<p<<std::endl;
      return false;
    }
    if(::kill(p, 0) != 0) {
      // ESRCH - child exited and lost
      // EPERM - PID reused
      if((errno == ESRCH) || (errno == EPERM)) {
        // There is slight possibility of race condition
        // when child already exited but waitpid for it
        // was not called yet. Let's give code 1 min
        // to process lost child to be on safe side.
        if(t.GetTime() != Time::UNDEFINED) {
          if((Time()-t) > Period(60)) {
            std::cerr<<"HUGE PROBLEM: lost child process: "<<p<<std::endl;
            return false;
          }
        } else {
          t = Time();
        }
      }
    }
    return true;
  }

#define SAFE_DISCONNECT(CONNECTOR) { \
    try { \
      (CONNECTOR).disconnect(); \
    } catch (Glib::Exception& e) { \
    } catch (std::exception& e) { \
    }; \
}

  class Watchdog;

  class RunPump {
    friend class Run;
    friend class Watchdog;
  private:
    static Glib::StaticMutex instance_lock_;
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
    //Glib::Mutex pump_lock_;
    TimedMutex pump_lock_;
    Glib::RefPtr<Glib::MainContext> context_;
    Glib::Thread *thread_;
    int storm_count;
    RunPump(void);
    ~RunPump(void);
    static RunPump& Instance(void);
    operator bool(void) const {
      return (bool)context_;
    }
    bool operator!(void) const {
      return !(bool)context_;
    }
    void Pump(void);
    void Add(Run *r);
    void Remove(Run *r);
    void child_handler(Glib::Pid pid, int result);
    static void fork_handler(void);
  };

  Glib::StaticMutex RunPump::instance_lock_ = GLIBMM_STATIC_MUTEX_INIT;
  RunPump* RunPump::instance_ = NULL;
  unsigned int RunPump::mark_ = ~RunPumpMagic;

  class Pid {
  private:
    Glib::Pid p_;
  public:
    Pid(void):p_(0) { }; 
    Pid(Glib::Pid p):p_(p) { }; 
    ~Pid(void) { if(p_) Glib::spawn_close_pid(p_); };
    operator Glib::Pid(void) const { return p_; };
    Glib::Pid pid(void) const { return p_; };
    Glib::Pid& operator=(const Glib::Pid& p) { return (p_=p); };
  };

  class RunInitializerArgument {
  private:
    void *arg_;
    void (*func_)(void*);
    UserSwitch* usw_;
    int user_id_;
    int group_id_;
  public:
    RunInitializerArgument(void(*func)(void*), void *arg, UserSwitch* usw, int user_id, int group_id)
      : arg_(arg),
        func_(func),
        usw_(usw),
        user_id_(user_id),
        group_id_(group_id) {}
    void Run(void);
  };

  void RunInitializerArgument::Run(void) {
    // It would be nice to have function which removes all Glib::Mutex locks.
    // But so far we need to save ourselves only from Logger and SetEnv/GetEnv.
    void *arg = arg_;
    void (*func)(void*) = func_;
    if(group_id_ > 0) ::setgid(group_id_);
    if(user_id_ != 0) {
      if(::setuid(user_id_) != 0) {
        // Can't switch user id
        _exit(-1);
      };
      // in case previous user was not allowed to switch group id
      if(group_id_ > 0) ::setgid(group_id_);
    };
    // set proper umask
    ::umask(0077);
    // To leave clean environment reset all signals.
    // Otherwise we may get some signals non-intentionally ignored.
    // Glib takes care of open handles.
#ifdef SIGRTMIN
    for(int n = SIGHUP; n < SIGRTMIN; ++n) {
#else
    // At least reset all signals whose numbers are well defined
    for(int n = SIGHUP; n < SIGTERM; ++n) {
#endif
      signal(n,SIG_DFL);
    }
    if (!func) return;
    // Run initializer requested by caller
    (*func)(arg);
    return;
  }

  RunPump::RunPump(void)
    : context_(NULL),
      thread_(NULL),
      storm_count(0) {
    try {
      thread_ = Glib::Thread::create(sigc::mem_fun(*this, &RunPump::Pump), false);
    } catch (Glib::Exception& e) {} catch (std::exception& e) {}
    ;
    if (thread_ == NULL) return;
    // Wait for context_ to be intialized
    // TODO: what to do if context never initialized
    for (;;) {
      if (context_) break;
      thread_->yield(); // This is simpler than condition+mutex
    }
  }

  void RunPump::fork_handler(void) {
    instance_ = NULL;
    mark_ = RunPumpMagic;
  }

  RunPump& RunPump::Instance(void) {
    instance_lock_.lock();
    if ((instance_ == NULL) || (mark_ != RunPumpMagic)) {
      //pthread_atfork(NULL,NULL,&fork_handler);
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
        if (!dispatched) {
          // Under some unclear circumstance storm of iteration()
          // returning false non-stop was observed. So here we 
          // are trying to prevent that.
          if((++storm_count) >= 10) {
            sleep(1);
            storm_count = 0;
          };
        } else {
          storm_count = 0;
        }
      }
    } catch (Glib::Exception& e) {} catch (std::exception& e) {};
  }

  void RunPump::Add(Run *r) {
    if (!r) return;
    if (!(*r)) return;
    if (!(*this)) return;
    // Take full control over context
    list_lock_.lock();
    while (true) {
      context_->wakeup();
      // doing it like that because experience says
      // wakeup does not always wakes it up
      if(pump_lock_.lock(100)) break;
    }
    try {
      // Add sources to context
      if (r->stdout_str_ && !(r->stdout_keep_)) {
        r->stdout_conn_ = context_->signal_io().connect(sigc::mem_fun(*r, &Run::stdout_handler), r->stdout_, Glib::IO_IN | Glib::IO_HUP);
      }
      if (r->stderr_str_ && !(r->stderr_keep_)) {
        r->stderr_conn_ = context_->signal_io().connect(sigc::mem_fun(*r, &Run::stderr_handler), r->stderr_, Glib::IO_IN | Glib::IO_HUP);
      }
      if (r->stdin_str_ && !(r->stdin_keep_)) {
        r->stdin_conn_ = context_->signal_io().connect(sigc::mem_fun(*r, &Run::stdin_handler), r->stdin_, Glib::IO_OUT | Glib::IO_HUP);
      }
#ifdef HAVE_GLIBMM_CHILDWATCH
      r->child_conn_ = context_->signal_child_watch().connect(sigc::mem_fun(*r, &Run::child_handler), r->pid_->pid());
      //if(r->child_conn_.empty()) std::cerr<<"connect for signal_child_watch failed"<<std::endl;
#endif
    } catch (Glib::Exception& e) {} catch (std::exception& e) {}
    pump_lock_.unlock();
    list_lock_.unlock();
  }

  void RunPump::Remove(Run *r) {
    if (!r) return;
    if (!(*r)) return;
    if (!(*this)) return;
    // Take full control over context
    list_lock_.lock();
    while (true) {
      context_->wakeup();
      // doing it like that because experience says
      // wakeup does not always wakes it up
      if(pump_lock_.lock(100)) break;
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
    }
    pump_lock_.unlock();
    list_lock_.unlock();
  }

  void RunPump::child_handler(Glib::Pid pid, int /* result */) {
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
      kicker_arg_(NULL),
      started_(false),
      running_(false),
      abandoned_(false),
      result_(-1),
      user_id_(0),
      group_id_(0),
      run_time_(Time::UNDEFINED),
      exit_time_(Time::UNDEFINED) {
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
      stdout_keep_(false),
      stderr_keep_(false),
      stdin_keep_(false),
      pid_(NULL),
      argv_(argv),
      initializer_func_(NULL),
      initializer_arg_(NULL),
      kicker_func_(NULL),
      kicker_arg_(NULL),
      started_(false),
      running_(false),
      abandoned_(false),
      result_(-1),
      user_id_(0),
      group_id_(0),
      run_time_(Time::UNDEFINED),
      exit_time_(Time::UNDEFINED) {
    pid_ = new Pid;
  }

  Run::~Run(void) {
    if(*this) {
      if(!abandoned_) Kill(0);
      CloseStdout();
      CloseStderr();
      CloseStdin();
      RunPump::Instance().Remove(this);
      delete pid_;
    };
  }

  static void remove_env(std::list<std::string>& envp, const std::string& key) {
    std::list<std::string>::iterator e = envp.begin();
    while(e != envp.end()) {
      if((strncmp(e->c_str(),key.c_str(),key.length()) == 0) &&
         ((e->length() == key.length()) || (e->at(key.length()) == '='))) {
        e = envp.erase(e);
        continue;
      };
      ++e;
    };
  }

  static void remove_env(std::list<std::string>& envp, const std::list<std::string>& keys) {
    for(std::list<std::string>::const_iterator key = keys.begin();
                   key != keys.end(); ++key) {
      remove_env(envp, *key);
    };
  }

  static void add_env(std::list<std::string>& envp, const std::string& rec) {
    std::string key(rec);
    std::string::size_type pos = key.find('=');
    if(pos != std::string::npos) key.resize(pos);
    remove_env(envp,key);
    envp.push_back(rec);
  }

  static void add_env(std::list<std::string>& envp, const std::list<std::string>& recs) {
    for(std::list<std::string>::const_iterator rec = recs.begin();
                   rec != recs.end(); ++rec) {
      add_env(envp, *rec);
    };
  }

  static void exit_child(int code, char const * msg) {
    int l = strlen(msg);
    (void)write(2, msg, l);
    _exit(code);
  }

  bool Run::Start(void) {
    if (started_) return false;
    if (argv_.size() < 1) return false;
    RunPump& pump = RunPump::Instance();
    UserSwitch* usw = NULL;
    RunInitializerArgument *arg = NULL;
    std::list<std::string> envp_tmp;
    try {
      running_ = true;
      Glib::Pid pid = 0;
      // Locking user switching to make sure fork is 
      // is done with proper uid
      usw = new UserSwitch(0,0);
      arg = new RunInitializerArgument(initializer_func_, initializer_arg_, usw, user_id_, group_id_);
#ifdef USE_GLIB_PROCESS_SPAWN
      {
      EnvLockWrapper wrapper; // Protection against gettext using getenv
      envp_tmp = GetEnv();
      remove_env(envp_tmp, envx_);
      add_env(envp_tmp, envp_);
      spawn_async_with_pipes(working_directory, argv_, envp_tmp,
                             Glib::SpawnFlags(Glib::SPAWN_DO_NOT_REAP_CHILD),
                             sigc::mem_fun(*arg, &RunInitializerArgument::Run),
                             &pid,
                             stdin_keep_  ? NULL : &stdin_,
                             stdout_keep_ ? NULL : &stdout_,
                             stderr_keep_ ? NULL : &stderr_);
      };
#else
      envp_tmp = GetEnv();
      remove_env(envp_tmp, envx_);
      add_env(envp_tmp, envp_);
      int pipe_stdin[2] = { -1, -1 };
      int pipe_stdout[2] = { -1, -1 };
      int pipe_stderr[2] = { -1, -1 };
      pid = -1;
      if((stdin_keep_  || (::pipe(pipe_stdin) == 0)) && 
         (stdout_keep_ || (::pipe(pipe_stdout) == 0)) &&
         (stderr_keep_ || (::pipe(pipe_stderr) == 0))) {

        uint64_t max_files = RLIM_INFINITY;
        struct rlimit lim;
        if(getrlimit(RLIMIT_NOFILE,&lim) == 0) { max_files=lim.rlim_cur; };
        if(max_files == RLIM_INFINITY) max_files=4096; // some safe value 
        char * * argv = new char*[argv_.size()+1];
        char * * envp = new char*[envp_tmp.size()+1];
        int n = 0;
        for(std::list<std::string>::iterator item = argv_.begin(); item != argv_.end(); ++item) {
          argv[n++] = const_cast<char*>(item->c_str());
        };
        argv[n] = NULL;
        n = 0;
        for(std::list<std::string>::iterator item = envp_tmp.begin(); item != envp_tmp.end(); ++item) {
          envp[n++] = const_cast<char*>(item->c_str());
        };
        envp[n] = NULL;

        // Stop acceptin signals temporarily to avoid signal handlers calling unsafe
        // functions inside child context.
        sigset_t newsig; sigfillset(&newsig);
        sigset_t oldsig; sigemptyset(&oldsig);
        bool oldsig_set = (pthread_sigmask(SIG_BLOCK,&newsig,&oldsig) == 0);

        if (oldsig_set)
          pid = ::fork();
        if(pid == 0) {
          // child - set std* and do exec
          if(pipe_stdin[0] != -1) {
            close(pipe_stdin[1]);
            if(dup2(pipe_stdin[0], 0) != 0) exit_child(-1, "Failed to setup stdin\n");
            close(pipe_stdin[0]);
          };
          if(pipe_stdout[1] != -1) {
            close(pipe_stdout[0]);
            if(dup2(pipe_stdout[1], 1) != 1) exit_child(-1, "Failed to setup stdout\n");
            close(pipe_stdout[1]);
          };
          if(pipe_stderr[1] != -1) {
            close(pipe_stderr[0]);
            if(dup2(pipe_stderr[1], 2) != 2) exit_child(-1, "Failed to setup stderr\n");
            close(pipe_stderr[1]);
          };
          arg->Run();
          if(::chdir(working_directory.c_str()) != 0) {
            exit_child(-1, "Failed to change working directory\n");
          }

          // close all handles inherited from parent
 	  for(int i=3;i<max_files;i++) { close(i); }; // skiping std* handles

          (void)::execve(argv[0], argv, envp);
          exit_child(-1, "Failed to execute command\n");
        } else if(pid != -1) {
          // parent - close unneeded sides of pipes
          if(pipe_stdin[1] != -1) {
            close(pipe_stdin[0]); pipe_stdin[0] = -1;
            stdin_ = pipe_stdin[1]; pipe_stdin[1] = -1;
          };
          if(pipe_stdout[0] != -1) {
            close(pipe_stdout[1]); pipe_stdout[1] = -1;
            stdout_ = pipe_stdout[0]; pipe_stdout[0] = -1;
          };
          if(pipe_stderr[0] != -1) {
            close(pipe_stderr[1]); pipe_stderr[1] = -1;
            stderr_ = pipe_stderr[0]; pipe_stderr[0] = -1;
          };
        };
        if(oldsig_set)
          pthread_sigmask(SIG_SETMASK,&oldsig,NULL);

        delete[] argv;
        delete[] envp;
      };
      if(pid == -1) {
        // child was not spawned
        if(pipe_stdin[0] != -1) close(pipe_stdin[0]);
        if(pipe_stdin[1] != -1) close(pipe_stdin[1]);
        if(pipe_stdout[0] != -1) close(pipe_stdout[0]);
        if(pipe_stdout[1] != -1) close(pipe_stdout[1]);
        if(pipe_stderr[0] != -1) close(pipe_stderr[0]);
        if(pipe_stderr[1] != -1) close(pipe_stderr[1]);
        throw Glib::SpawnError(Glib::SpawnError::FORK, "Generic error");
      };
#endif
      *pid_ = pid;
      if (!stdin_keep_) {
        fcntl(stdin_, F_SETFL, fcntl(stdin_, F_GETFL) | O_NONBLOCK);
      };
      if (!stdout_keep_) {
        fcntl(stdout_, F_SETFL, fcntl(stdout_, F_GETFL) | O_NONBLOCK);
      };
      if (!stderr_keep_) {
        fcntl(stderr_, F_SETFL, fcntl(stderr_, F_GETFL) | O_NONBLOCK);
      };
      run_time_ = Time();
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
    if (!running_) return;
    if (timeout > 0) {
      // Kill softly
      ::kill(pid_->pid(), SIGTERM);
      Wait(timeout);
    }
    if (!running_) return;
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
      } else {
        stdout_str_->Append(buf, l);
      }
    } else {
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
      } else {
        stderr_str_->Append(buf, l);
      }
    } else {
      // Event shouldn't happen if not expected

    }
    return true;
  }

  bool Run::stdin_handler(Glib::IOCondition) {
    if (stdin_str_) {
      if (stdin_str_->Size() == 0) {
        CloseStdin();
        stdin_str_ = NULL;
      } else {
        int l = WriteStdin(0, stdin_str_->Get(), stdin_str_->Size());
        if (l == -1) {
          CloseStdin();
          return false;
        } else {
          stdin_str_->Remove(l);
        }
      }
    } else {
      // Event shouldn't happen if not expected

    }
    return true;
  }

  void Run::child_handler(Glib::Pid, int result) {
    if (stdout_str_) for (;;) if (!stdout_handler(Glib::IO_IN)) break;
    if (stderr_str_) for (;;) if (!stderr_handler(Glib::IO_IN)) break;
    //CloseStdout();
    //CloseStderr();
    CloseStdin();
    lock_.lock();
    cond_.signal();
    // There is reference in Glib manual that 'result' is same
    // as returned by waitpid. It is not clear how it works for
    // windows but atleast for *nix we can use waitpid related
    // macros.
#ifdef DUAL_CHECK_LOST_CHILD
    if(result == -1) { // special value to indicate lost child
      result_ = -1;
    } else
#endif
    if(WIFEXITED(result)) {
      result_ = WEXITSTATUS(result);
    } else {
      result_ = -1;
    }
    running_ = false;
    exit_time_ = Time();
    lock_.unlock();
    if (kicker_func_) (*kicker_func_)(kicker_arg_);
  }

  void Run::CloseStdout(void) {
    if (stdout_ != -1) ::close(stdout_);
    stdout_ = -1;
    SAFE_DISCONNECT(stdout_conn_);
  }

  void Run::CloseStderr(void) {
    if (stderr_ != -1) ::close(stderr_);
    stderr_ = -1;
    SAFE_DISCONNECT(stderr_conn_);
  }

  void Run::CloseStdin(void) {
    if (stdin_ != -1) ::close(stdin_);
    stdin_ = -1;
    SAFE_DISCONNECT(stdin_conn_);
  }

  int Run::ReadStdout(int timeout, char *buf, int size) {
    if (stdout_ == -1) return -1;
    // TODO: do it through context for timeout?
    for(;;) {
      pollfd fd;
      fd.fd = stdout_; fd.events = POLLIN; fd.revents = 0;
      int err = ::poll(&fd, 1, timeout);
      if((err < 0) && (errno == EINTR)) continue;
      if(err <= 0) return err;
      if(!(fd.revents & POLLIN)) return -1;
      break;
    }
    return ::read(stdout_, buf, size);
  }

  int Run::ReadStderr(int timeout, char *buf, int size) {
    if (stderr_ == -1) return -1;
    // TODO: do it through context for timeout
    for(;;) {
      pollfd fd;
      fd.fd = stderr_; fd.events = POLLIN; fd.revents = 0;
      int err = ::poll(&fd, 1, timeout);
      if((err < 0) && (errno == EINTR)) continue;
      if(err <= 0) return err;
      if(!(fd.revents & POLLIN)) return -1;
      break;
    }
    return ::read(stderr_, buf, size);
  }

  int Run::WriteStdin(int timeout, const char *buf, int size) {
    if (stdin_ == -1) return -1;
    // TODO: do it through context for timeout
    for(;;) {
      pollfd fd;
      fd.fd = stdin_; fd.events = POLLOUT; fd.revents = 0;
      int err = ::poll(&fd, 1, timeout);
      if((err < 0) && (errno == EINTR)) continue;
      if(err <= 0) return err;
      if(!(fd.revents & POLLOUT)) return -1;
      break;
    }
    return write(stdin_, buf, size);
  }

  bool Run::Running(void) {
#ifdef DUAL_CHECK_LOST_CHILD
      if(running_) {
        if(!check_pid(pid_->pid(),exit_time_)) {
          lock_.unlock();
          child_handler(pid_->pid(), -1); // simulate exit
          lock_.lock();
        }
      }
#endif
#ifdef HAVE_GLIBMM_CHILDWATCH
    return running_;
#else
    Wait(0);
    return running_;
#endif
  }

  bool Run::Wait(int timeout) {
    if (!started_) return false;
    if (!running_) return true;
    Glib::TimeVal till;
    till.assign_current_time();
    till += timeout;
    lock_.lock();
    while (running_) {
      Glib::TimeVal t;
      t.assign_current_time();
      t.subtract(till);
#ifdef HAVE_GLIBMM_CHILDWATCH
      if (!t.negative()) break;
      cond_.timed_wait(lock_, till);
#else
      int status;
      int r = ::waitpid(pid_->pid(), &status, WNOHANG);
      if (r == 0) {
        if (!t.negative()) break;
        lock_.unlock();
        sleep(1);
        lock_.lock();
        continue;
      }
      if (r == -1) {
        // Child lost?
        status = (-1)<<8;
      }
      // Child exited
      lock_.unlock();
      child_handler(pid_->pid(), status);
      lock_.lock();
#endif
#ifdef DUAL_CHECK_LOST_CHILD
      if(running_) {
        if(!check_pid(pid_->pid(),exit_time_)) {
          lock_.unlock();
          child_handler(pid_->pid(), -1); // simulate exit
          lock_.lock();
        }
      }
#endif
    }
    lock_.unlock();
    return (!running_);
  }

  bool Run::Wait(void) {
    if (!started_) return false;
    if (!running_) return true;
    lock_.lock();
    Glib::TimeVal till;
    while (running_) {
#ifdef HAVE_GLIBMM_CHILDWATCH
      till.assign_current_time();
      till += 1; // one sec later
      cond_.timed_wait(lock_, till);
#else
      int status;
      int r = ::waitpid(pid_->pid(), &status, WNOHANG);
      if (r == 0) {
        lock_.unlock();
        sleep(1);
        lock_.lock();
        continue;
      }
      if (r == -1) {
        // Child lost?
        status = (-1)<<8;
      }
      // Child exited
      lock_.unlock();
      child_handler(pid_->pid(), status);
      lock_.lock();
#endif
#ifdef DUAL_CHECK_LOST_CHILD
      if(running_) {
        if(!check_pid(pid_->pid(),exit_time_)) {
          lock_.unlock();
          child_handler(pid_->pid(), -1); // simulate exit
          lock_.lock();
        }
      }
#endif
    }
    lock_.unlock();
    return (!running_);
  }

  void Run::AssignStdout(std::string& str) {
    if (!running_) {
      stdout_str_wrap_.Assign(str);
      stdout_str_ = &stdout_str_wrap_;
    }
  }

  void Run::AssignStdout(Data& str) {
    if (!running_) {
      stdout_str_ = &str;
    }
  }

  void Run::AssignStderr(std::string& str) {
    if (!running_) {
      stderr_str_wrap_.Assign(str);
      stderr_str_ = &stderr_str_wrap_;
    }
  }

  void Run::AssignStderr(Data& str) {
    if (!running_) {
      stderr_str_ = &str;
    }
  }

  void Run::AssignStdin(std::string& str) {
    if (!running_) {
      stdin_str_wrap_.Assign(str);
      stdin_str_ = &stdin_str_wrap_;
    }
  }

  void Run::AssignStdin(Data& str) {
    if (!running_) {
      stdin_str_ = &str;
    }
  }

  void Run::KeepStdout(bool keep) {
    if (!running_) stdout_keep_ = keep;
  }

  void Run::KeepStderr(bool keep) {
    if (!running_) stderr_keep_ = keep;
  }

  void Run::KeepStdin(bool keep) {
    if (!running_) stdin_keep_ = keep;
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

  void Run::AfterFork(void) {
    RunPump::fork_handler();
  }


#define WATCHDOG_TEST_INTERVAL (60)
#define WATCHDOG_KICK_INTERVAL (10)

  class Watchdog {
  friend class WatchdogListener;
  friend class WatchdogChannel;
  private:
    class Channel {
    public:
      int timeout;
      time_t next;
      Channel(void):timeout(-1),next(0) {};
    };
    int lpipe[2];
    sigc::connection timer_;
    static Glib::Mutex instance_lock_;
    std::vector<Channel> channels_;
    static Watchdog *instance_;
    static unsigned int mark_;
#define WatchdogMagic (0x1E84FC05)
    static Watchdog& Instance(void);
    int Open(int timeout);
    void Kick(int channel);
    void Close(int channel);
    int Listen(void);
    bool Timer(void);
  public:
    Watchdog(void);
    ~Watchdog(void);
  };

  Glib::Mutex Watchdog::instance_lock_;
  Watchdog* Watchdog::instance_ = NULL;
  unsigned int Watchdog::mark_ = ~WatchdogMagic;

  Watchdog& Watchdog::Instance(void) {
    instance_lock_.lock();
    if ((instance_ == NULL) || (mark_ != WatchdogMagic)) {
      instance_ = new Watchdog();
      mark_ = WatchdogMagic;
    }
    instance_lock_.unlock();
    return *instance_;
  }

  Watchdog::Watchdog(void) {
    lpipe[0] = -1; lpipe[1] = -1;
    ::pipe(lpipe);
    if(lpipe[1] != -1) fcntl(lpipe[1], F_SETFL, fcntl(lpipe[1], F_GETFL) | O_NONBLOCK);
  }

  Watchdog::~Watchdog(void) {
    if(timer_.connected()) timer_.disconnect();
    if(lpipe[0] != -1) ::close(lpipe[0]);
    if(lpipe[1] != -1) ::close(lpipe[1]);
  }

  bool Watchdog::Timer(void) {
    char c = '\0';
    time_t now = ::time(NULL);
    {
      Glib::Mutex::Lock lock(instance_lock_);
      for(int n = 0; n < channels_.size(); ++n) {
        if(channels_[n].timeout < 0) continue;
        if(((int)(now - channels_[n].next)) > 0) return true; // timeout
      }
    }
    if(lpipe[1] != -1) write(lpipe[1],&c,1);
    return true;
  }

  int Watchdog::Open(int timeout) {
    if(timeout <= 0) return -1;
    Glib::Mutex::Lock lock(instance_lock_);
    if(!timer_.connected()) {
      // start glib loop and attach timer to context
      Glib::RefPtr<Glib::MainContext> context = RunPump::Instance().context_;
      timer_ = context->signal_timeout().connect(sigc::mem_fun(*this,&Watchdog::Timer),WATCHDOG_KICK_INTERVAL*1000);
    }
    int n = 0;
    for(; n < channels_.size(); ++n) {
      if(channels_[n].timeout < 0) {
        channels_[n].timeout = timeout;
        channels_[n].next = ::time(NULL) + timeout;
        return n;
      }
    }
    channels_.resize(n+1);
    channels_[n].timeout = timeout;
    channels_[n].next = ::time(NULL) + timeout;
    return n;
  }

  void Watchdog::Kick(int channel) {
    Glib::Mutex::Lock lock(instance_lock_);
    if((channel < 0) || (channel >= channels_.size())) return;
    if(channels_[channel].timeout < 0) return;
    channels_[channel].next = ::time(NULL) + channels_[channel].timeout;
  }

  void Watchdog::Close(int channel) {
    Glib::Mutex::Lock lock(instance_lock_);
    if((channel < 0) || (channel >= channels_.size())) return;
    channels_[channel].timeout = -1;
    // resize?
  }

  int Watchdog::Listen(void) {
    return lpipe[0];
  }

  WatchdogChannel::WatchdogChannel(int timeout) {
    id_ = Watchdog::Instance().Open(timeout);
  }

  WatchdogChannel::~WatchdogChannel(void) {
    Watchdog::Instance().Close(id_);
  }
 
  void WatchdogChannel::Kick(void) {
    Watchdog::Instance().Kick(id_);
  }

  WatchdogListener::WatchdogListener(void):
             instance_(Watchdog::Instance()),last((time_t)(-1)) {
  }

  bool WatchdogListener::Listen(int limit, bool& error) {
    error = false;
    int h = instance_.Listen();
    if(h == -1) return !(error = true);
    time_t out = (time_t)(-1); // when to leave
    if(limit >= 0) out = ::time(NULL) + limit;
    int to = 0; // initailly just check if something already arrived
    for(;;) {
      pollfd fd;
      fd.fd = h; fd.events = POLLIN; fd.revents = 0;
      int err = ::poll(&fd, 1, to);
      // process errors
      if((err < 0) && (errno != EINTR)) break; // unrecoverable error
      if(err > 0) { // unexpected results
        if(err != 1) break;
        if(!(fd.revents & POLLIN)) break;
      };
      time_t now = ::time(NULL);
      time_t next = (time_t)(-1); // when to timeout
      if(err == 1) {
        // something arrived
        char c;
        ::read(fd.fd,&c,1);
        last = now; next = now + WATCHDOG_TEST_INTERVAL;
      } else {
        // check timeout
        if(last != (time_t)(-1)) next = last + WATCHDOG_TEST_INTERVAL;
        if((next != (time_t)(-1)) && (((int)(next-now)) <= 0)) return true;
      }
      // check for time limit
      if((limit >= 0) && (((int)(out-now)) <= 0)) return false;
      // prepare timeout for poll
      to = WATCHDOG_TEST_INTERVAL;
      if(next != (time_t)(-1)) {
        int tto = next-now;
        if(tto < to) to = tto;
      }
      if(limit >= 0) {
        int tto = out-now;
        if(tto < to) to = tto;
      }
      if(to < 0) to = 0;
    }
    // communication failure
    error = true;
    return false;
  }

  bool WatchdogListener::Listen(void) {
    bool error;
    return Listen(-1,error);
  }


  Run::StringData::StringData(): content_(NULL) {
  }

  Run::StringData::~StringData() {
  }

  void Run::StringData::Assign(std::string& str) {
    content_ = &str;
  }

  void Run::StringData::Append(char const* data, unsigned int size) {
    if(content_) content_->append(data, size);
  }

  void Run::StringData::Remove(unsigned int size) {
    if(content_) content_->erase(0, size);
  }

  char const* Run::StringData::Get() const {
    if(!content_) return NULL;
    return content_->c_str();
  }

  unsigned int Run::StringData::Size() const {
    if(!content_) return 0;
    return content_->length();
  }


}

