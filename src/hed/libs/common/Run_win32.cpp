// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm.h>

#include <iostream>
#include <unistd.h>

#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/win32.h>
#include "Run.h"


namespace Arc {

  class RunPump {
    // NOP
  };

  class Pid {
    friend class Run;
  private:
    PROCESS_INFORMATION processinfo;
    Pid(void) {}
    Pid(int p_) {}
    Pid(const PROCESS_INFORMATION p_) {
      processinfo = p_;
    }
  };

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
      argv_(Glib::shell_parse_argv(cmdline)),
      initializer_func_(NULL),
      kicker_func_(NULL),
      started_(false),
      running_(false),
      result_(-1) {
    pid_ = new Pid();
  }

  Run::Run(const std::list<std::string>& argv)
    : working_directory("."),
      stdout_(-1),
      stderr_(-1),
      stdin_(-1),
      stdout_str_(NULL),
      stderr_str_(NULL),
      stdin_str_(NULL),
      argv_(argv),
      initializer_func_(NULL),
      kicker_func_(NULL),
      started_(false),
      running_(false),
      result_(-1) {
    pid_ = new Pid();
  }

  Run::~Run(void) {
    if (!(*this))
      return;
    Kill(0);
    CloseStdout();
    CloseStderr();
    CloseStdin();
    delete pid_;
  }

  bool Run::Start(void) {
    if (started_)
      return false;
    if (argv_.size() < 1)
      return false;
    try {
      running_ = true;

      STARTUPINFO startupinfo;
      memset(&startupinfo, 0, sizeof(startupinfo));
      char **args = const_cast<char**>(argv_.data());
      std::string cmd = "";
      for (int i = 0; args[i] != NULL; i++) {
        std::string a(args[i]);
        cmd += (a + " ");
      }
      std::cout << "Cmd: " << cmd << std::endl;
      int result = CreateProcess(NULL,
                                 (LPSTR)cmd.c_str(),
                                 NULL,
                                 NULL,
                                 FALSE,
                                 CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW | IDLE_PRIORITY_CLASS,
                                 NULL,
                                 (LPSTR)working_directory.c_str(),
                                 &startupinfo,
                                 &(pid_->processinfo));

      if (!result) {
        ;
        std::cout << "Spawn Error: " << GetOsErrorMessage() << std::endl;
        return false;
      }
      started_ = true;
    } catch (std::exception& e) {
      running_ = false;
      return false;
    }
    ;
    return true;
  }

  void Run::Kill(int timeout) {
    if (!running_)
      return;
    // Kill with no merci
    running_ = false;
    TerminateProcess(pid_->processinfo.hProcess, 256);
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
    running_ = false;
    if (kicker_func_)
      (*kicker_func_)(kicker_arg_);
  }

  void Run::CloseStdout(void) {
    if (stdout_ != -1)
      ::close(stdout_);
    stdout_ = -1;
  }

  void Run::CloseStderr(void) {
    if (stderr_ != -1)
      ::close(stderr_);
    stderr_ = -1;
  }

  void Run::CloseStdin(void) {
    if (stdin_ != -1)
      ::close(stdin_);
    stdin_ = -1;
  }

  int Run::ReadStdout(int /*timeout*/, char *buf, int size) {
    if (stdout_ == -1)
      return -1;
    // TODO: do it through context for timeout
    return ::read(stdout_, buf, size);
  }

  int Run::ReadStderr(int /*timeout*/, char *buf, int size) {
    if (stderr_ == -1)
      return -1;
    // TODO: do it through context for timeout
    return ::read(stderr_, buf, size);
  }

  int Run::WriteStdin(int /*timeout*/, const char *buf, int size) {
    if (stdin_ == -1)
      return -1;
    // TODO: do it through context for timeout
    return write(stdin_, buf, size);
  }

  bool Run::Running(void) {
    Wait(0);
    return running_;
  }

  bool Run::Wait(int timeout) {
    if (!started_)
      return false;
    if (!running_)
      return true;
    // XXX timeouted wait
    return true;
  }

  bool Run::Wait(void) {
    if (!started_)
      return false;
    if (!running_)
      return true;
    WaitForSingleObject(pid_->processinfo.hProcess, INFINITE);
    WaitForSingleObject(pid_->processinfo.hThread, INFINITE);
    CloseHandle(pid_->processinfo.hThread);
    CloseHandle(pid_->processinfo.hProcess);
    running_ = false;
    return true;
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
