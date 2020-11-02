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
#include <signal.h>

#include <iostream>
#include <set>

#include <glibmm.h>

#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/User.h>
#include <arc/Utils.h>

#include "Run.h"


namespace Arc {

  static Logger logger(Logger::getRootLogger(), "Run");

  class RunPump {
    friend class Run;
  private:
    static Glib::StaticMutex instance_lock_;
    static RunPump *instance_;
    static unsigned int mark_;
#define RunPumpMagic (0xA73E771F)
    // Reference to processes we are not interested in anymore.
    // It is only kept for calling waitpid() on it.
    std::set<pid_t> abandoned_;
    // Processes to monitor
    std::set<Run*> monitored_;
    // Lock for containers of monitored handles and pids.
    Glib::Mutex list_lock_;
    // pipe for kicking main loop.
    int loop_kick_[2];
    // Barrier for slowing down main loop.
    Glib::Mutex loop_lock_;
    // Thread which performs monitoring.
    Glib::Thread *thread_;
    RunPump(void);
    ~RunPump(void);
    static RunPump& Instance(void);
    operator bool(void) const { return (thread_ != NULL); }
    bool operator!(void) const { return (thread_ == NULL); }
    void Pump(void);
    // Add process and associated handles to those monitored by this instance
    void Add(Run *r);
    // Remove process  and handles from monitored.
    // If process is still running it's pid is still monitored.
    void Remove(Run *r);
    static void fork_handler(void);
    static void sigchld_handler(int);
  };

  Glib::StaticMutex RunPump::instance_lock_ = GLIBMM_STATIC_MUTEX_INIT;
  RunPump* RunPump::instance_ = NULL;
  unsigned int RunPump::mark_ = ~RunPumpMagic;

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
    // Post-fork code takes care of open handles.
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

  RunPump::RunPump()
    : thread_(NULL)/*, storm_count(0)*/ {
    loop_kick_[0] = -1;
    loop_kick_[1] = -1;
    try {
      if(::pipe(loop_kick_) != 0)
        return;
      (void)fcntl(loop_kick_[0], F_SETFL, fcntl(loop_kick_[0], F_GETFL) | O_NONBLOCK);
      (void)fcntl(loop_kick_[1], F_SETFL, fcntl(loop_kick_[1], F_GETFL) | O_NONBLOCK);
      thread_ = Glib::Thread::create(sigc::mem_fun(*this, &RunPump::Pump), false);
    } catch (Glib::Exception& e) {} catch (std::exception& e) {};
    if (thread_ == NULL) return;
  }

  RunPump::~RunPump() {
    // There is only one static instance of this class.
    // So there is no sense to do any destruction.
  }

  void RunPump::fork_handler(void) {
    // In forked process disable access to instance while keeping initialization mark.
    instance_ = NULL;
    mark_ = RunPumpMagic;
    (void)signal(SIGCHLD, SIG_DFL);
  }

  void RunPump::sigchld_handler(int) {
    if ((instance_ == NULL) || (mark_ != RunPumpMagic)) return;
    if (instance_->loop_kick_[1] == -1) return;
    char dummy;
    (void)write(instance_->loop_kick_[1], &dummy, 1);
  }

  RunPump& RunPump::Instance(void) {
    Glib::Mutex::Lock lock(instance_lock_);
    // Check against fork_handler
    if ((instance_ == NULL) || (mark_ != RunPumpMagic)) {
      instance_ = new RunPump();
      mark_ = RunPumpMagic;
      (void)signal(SIGCHLD, &sigchld_handler);
    }
    return *instance_;
  }

  static short find_events(int fd, pollfd* handles, nfds_t handles_num) {
    for(nfds_t idx = 0; idx < handles_num; ++idx) {
      if(handles[idx].fd == fd) {
        return handles[idx].revents;
      }
    }
    return 0;
  }

  static void pollfd_free(pollfd* arg) { ::free(arg); };

  void RunPump::Pump(void) {
    try {
      while(true) {
        {
          Glib::Mutex::Lock lock(list_lock_);
          nfds_t handles_num = monitored_.size()*3 + 1;
          AutoPointer<pollfd> handles(reinterpret_cast<pollfd*>(malloc(sizeof(pollfd[handles_num]))), &pollfd_free);
          // wait for events on pipe handles and signals from child exit
          if(!handles) {
            // memory exhaustion?
            lock.release();
            sleep(1);
            continue;
          };
          size_t idx = 0;
          handles[idx].fd = loop_kick_[0]; handles[idx].events = POLLIN; handles[idx].revents = 0; 
          ++idx;
          for(std::set<Run*>::iterator it = monitored_.begin(); it != monitored_.end(); ++it) {
            Run* r = *it;
            if (r->stdout_str_ && !(r->stdout_keep_) && (r->stdout_ != -1)) {
              handles[idx].fd = r->stdout_; handles[idx].events = POLLIN; handles[idx].revents = 0; 
              ++idx;
            }
            if (r->stderr_str_ && !(r->stderr_keep_) && (r->stderr_ != -1)) {
              handles[idx].fd = r->stderr_; handles[idx].events = POLLIN; handles[idx].revents = 0; 
              ++idx;
            }
            if (r->stdin_str_ && !(r->stdin_keep_) && (r->stdin_ != -1)) {
              handles[idx].fd = r->stdin_; handles[idx].events = POLLOUT; handles[idx].revents = 0; 
              ++idx;
            }
          }
          handles_num = idx;
          sigset_t signals;
          sigemptyset(&signals);
          sigaddset(&signals, SIGCHLD);
          int res = ppoll(handles.Ptr(), handles_num, NULL, &signals);
          if(res > 0) {
            // handles need attention
          } else if(res < 0) {
            if(errno == EINTR) {
              // Probably child exited
              logger.msg(DEBUG, "Child monitoring signal detected");
            } else {
              int err = errno;
              // Some error. Only expected error is some handles already closed.
              lock.release();
              logger.msg(DEBUG, "Child monitoring error: %i", err);
              sleep(1);
              continue;
            }
            // In case of error ignore any information about handle.
            handles.Release();
          }
          // Handles processing
          if(handles) {
            // check kick pipe first
            if(handles[0].revents & POLLIN) {
              char dummy;
              (void)read(handles[0].fd, &dummy, 1);
              logger.msg(DEBUG, "Child monitoring kick detected");
            }
            if(handles[0].revents & POLLERR) {
              logger.msg(ERROR, "Child monitoring internal communication error");
            }

            for(std::set<Run*>::iterator it = monitored_.begin(); it != monitored_.end(); ++it) {
              Run* r = *it;
              if (r->stdout_str_ && !(r->stdout_keep_) && (r->stdout_ != -1)) {
                short revents = find_events(r->stdout_, handles.Ptr(), handles_num);
                if(revents & POLLIN) r->stdout_handler();
                if(revents & POLLERR) {
                  // In case of error just stop monitoring.
                  close(r->stdout_);
                  r->stdout_ = -1;
                  logger.msg(VERBOSE, "Child monitoring stdout is closed");
                }
              }
              if (r->stderr_str_ && !(r->stderr_keep_) && (r->stderr_ != -1)) {
                short revents = find_events(r->stderr_, handles.Ptr(), handles_num);
                if(revents & POLLIN) r->stderr_handler();
                if(revents & POLLERR) {
                  // In case of error just stop monitoring.
                  close(r->stderr_);
                  r->stderr_ = -1;
                  logger.msg(VERBOSE, "Child monitoring stderr is closed");
                }
              }
              if (r->stdin_str_ && !(r->stdin_keep_) && (r->stdin_ != -1)) {
                short revents = find_events(r->stdin_, handles.Ptr(), handles_num);
                if(revents & POLLOUT) r->stdin_handler();
                if(revents & POLLERR) {
                  // In case of error just stop monitoring.
                  close(r->stdin_);
                  r->stdin_ = -1;
                  logger.msg(VERBOSE, "Child monitoring stdin is closed");
                }
              }
            }
          }
          // Always do pid processing because there is no reliable information about signal
          {
            std::list<Run*> todrop;
            for(std::set<Run*>::iterator it = monitored_.begin(); it != monitored_.end(); ++it) {
              Run* r = *it;
              if(r->running_ && (r->pid_ > 0)) {
                int status = 0;
                pid_t rpid = waitpid(r->pid_, &status, WNOHANG);
                if(rpid == 0) {
                  // still running
                } else {
                  if(rpid == r->pid_) {
                    // exited
                    logger.msg(DEBUG, "Child monitoring child %d exited", r->pid_);
                    r->child_handler(status);
                  } else { // it should be -1
                    // error - child lost?
                    logger.msg(ERROR, "Child monitoring lost child %d (%d)", r->pid_, rpid);
                    r->child_handler(-1);
                  }
                  // Not monitoring exited and failed processes
                  todrop.push_back(r);
                }
              }
            }
            for(std::list<Run*>::iterator itd = todrop.begin(); itd != todrop.end(); ++itd) monitored_.erase(*itd);
          }
          {
            std::list<pid_t> todrop;
            for(std::set<pid_t>::iterator it = abandoned_.begin(); it != abandoned_.end(); ++it) {
              pid_t r = *it;
              if(r > 0) {
                int status = 0;
                pid_t rpid = waitpid(r, &status, WNOHANG);
                if(rpid == 0) {
                  // still running
                } else {
                  todrop.push_back(r);
                  logger.msg(DEBUG, "Child monitoring drops abandoned child %d (%d)", r, rpid);
                }
              }
            }
            for(std::list<pid_t>::iterator itd = todrop.begin(); itd != todrop.end(); ++itd) abandoned_.erase(*itd);
          }
        } // End of main lock
        // TODO: storm protection
        // Acquire barrier lock to allow modification of monitored handles
        Glib::Mutex::Lock llock(loop_lock_);
      }
    } catch (Glib::Exception& e) {} catch (std::exception& e) {};
  }

  void RunPump::Add(Run *r) {
    // Check validity of parameters
    if (!r) return;
    if (!(*r)) return;
    if (!(*this)) return;
    try {
      // Take full control over context
      Glib::Mutex::Lock llock(loop_lock_); // barrier
      char dummy;
      (void)write(loop_kick_[1], &dummy, 1);
      Glib::Mutex::Lock lock(list_lock_);
      // Add sources to context
      monitored_.insert(r);
    } catch (Glib::Exception& e) {} catch (std::exception& e) {}
  }

  void RunPump::Remove(Run *r) {
    // Check validity of parameters
    if (!r) return;
    if (!(*r)) return;
    if (!(*this)) return;
    try {
      // Take full control over context
      Glib::Mutex::Lock llock(loop_lock_); // barrier
      char dummy;
      (void)write(loop_kick_[1], &dummy, 1);
      Glib::Mutex::Lock lock(list_lock_);
      // Disconnect sources from context
      if(monitored_.erase(r) > 0) {
        // it is the process we are monitoring
        if(r->running_ && (r->pid_ > 0)) {
          // so keep monitoring it just for waitpid calls
          abandoned_.insert(r->pid_); 
        }
        r->running_ = false; // let Run instance think it finished
      }
    } catch (Glib::Exception& e) {} catch (std::exception& e) {}
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
      pid_(-1),
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
      pid_(-1),
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
    AutoPointer<UserSwitch> usw;
    AutoPointer<RunInitializerArgument> arg;
    std::list<std::string> envp_tmp;
    try {
      running_ = true;
      pid_t pid = -1;
      // Locking user switching to make sure fork is 
      // is done with proper uid
      usw = new UserSwitch(0,0);
      arg = new RunInitializerArgument(initializer_func_, initializer_arg_, usw.Ptr(), user_id_, group_id_);
      envp_tmp = GetEnv();
      remove_env(envp_tmp, envx_);
      add_env(envp_tmp, envp_);
      int pipe_stdin[2] = { -1, -1 };
      int pipe_stdout[2] = { -1, -1 };
      int pipe_stderr[2] = { -1, -1 };
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
          for(unsigned int i=3;i<max_files;i++) { close(i); }; // skiping std* handles

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
      pid_ = pid;
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
      running_ = false;
      // TODO: report error
      return false;
    } catch (std::exception& e) {
      running_ = false;
      return false;
    };
    pump.Add(this);
    return true;
  }

  void Run::Kill(int timeout) {
    if (!running_) return;
    pid_t pid = pid_;
    if (pid == -1) return;
    if (timeout > 0) {
      // Kill softly
      ::kill(pid, SIGTERM);
      Wait(timeout);
    }
    if (!running_) return;
    // Kill with no merci
    ::kill(pid, SIGKILL);
  }

  void Run::Abandon(void) {
    if(*this) {
      CloseStdout();
      CloseStderr();
      CloseStdin();
      abandoned_=true;
    }
  }

  bool Run::stdout_handler() {
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

  bool Run::stderr_handler() {
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

  bool Run::stdin_handler() {
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

  void Run::child_handler(int result) {
    if (stdout_str_) for (;;) if (!stdout_handler()) break;
    if (stderr_str_) for (;;) if (!stderr_handler()) break;
    CloseStdin();
    {
      Glib::Mutex::Lock lock(lock_);
      cond_.signal();
      if(result == -1) { // special value to indicate lost child
        result_ = -1;
      } else {
        if(WIFEXITED(result)) {
          result_ = WEXITSTATUS(result);
        } else {
          result_ = -1;
        }
      }
      running_ = false;
      exit_time_ = Time();
    }
    if (kicker_func_) (*kicker_func_)(kicker_arg_);
  }

  void Run::CloseStdout(void) {
    if (stdout_ != -1) ::close(stdout_);
    stdout_ = -1;
  }

  void Run::CloseStderr(void) {
    if (stderr_ != -1) ::close(stderr_);
    stderr_ = -1;
  }

  void Run::CloseStdin(void) {
    if (stdin_ != -1) ::close(stdin_);
    stdin_ = -1;
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
    return running_;
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
      if (!t.negative()) break;
      cond_.timed_wait(lock_, till);
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
      till.assign_current_time();
      till += 1; // one sec later
      cond_.timed_wait(lock_, till);
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

