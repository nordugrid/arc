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

#define BUFSIZE 4096 


namespace Arc {

  struct PipeThreadArg {
    HANDLE child;
    HANDLE parent;
  };

  void pipe_handler(void *arg);

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
 
      // Stdin, stdout, stderr redirect code is from here:
      // http://msdn.microsoft.com/en-us/library/ms682499(VS.85).aspx

      SECURITY_ATTRIBUTES saAttr; 
      saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
      saAttr.bInheritHandle = TRUE; 
      saAttr.lpSecurityDescriptor = NULL; 

      HANDLE g_hChildStd_OUT_Rd = NULL;
      HANDLE g_hChildStd_OUT_Wr = NULL;
      HANDLE g_hChildStd_ERR_Rd = NULL;
      HANDLE g_hChildStd_ERR_Wr = NULL;
      HANDLE g_hChildStd_IN_Rd = NULL;
      HANDLE g_hChildStd_IN_Wr = NULL;

      HANDLE g_hOutputFile = NULL;
      HANDLE g_hErrorFile = NULL;
      HANDLE g_hInputFile = NULL;

      // Create pipe for child process stdout

      if ( ! CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0) )  
        throw "StdoutRd create pipe error!";
      
      if ( ! SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0) )
        throw "Stdout SetHandleInformation error";

      // Create pipe for child process stderr

      if ( ! CreatePipe(&g_hChildStd_ERR_Rd, &g_hChildStd_ERR_Wr, &saAttr, 0) ) 
        throw "StderrRd create pipe error!";
      
      if ( ! SetHandleInformation(g_hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0) )
        throw "Stderr SetHandleInformation error";

      // Create pipe for child process stdin

      if ( ! CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0) ) 
        throw "StdinRd create pipe error!";
      
      if ( ! SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0) )
        throw "Stdin SetHandleInformation error";

      STARTUPINFO startupinfo;
      memset(&startupinfo, 0, sizeof(startupinfo));

      startupinfo.cb = sizeof(STARTUPINFO);
      startupinfo.hStdError = g_hChildStd_ERR_Wr;
      startupinfo.hStdOutput = g_hChildStd_OUT_Wr;
      startupinfo.hStdInput = g_hChildStd_IN_Rd;
      startupinfo.dwFlags |= STARTF_USESTDHANDLES;

      char **args = const_cast<char**>(argv_.data());
      std::string cmd = "";
      for (int i = 0; args[i] != NULL; i++) {
        std::string a(args[i]);
        cmd += (a + " ");
      }
      int result = CreateProcess(NULL,
                                 (LPSTR)cmd.c_str(),
                                 NULL,
                                 NULL,
                                 TRUE,
                                 CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW | IDLE_PRIORITY_CLASS,
                                 NULL,
                                 (LPSTR)working_directory.c_str(),
                                 &startupinfo,
                                 &(pid_->processinfo));

      if (!result) {
        std::cout << "Spawn Error: " << GetOsErrorMessage() << std::endl;
        return false;
      }
      started_ = true;

      if (stdin_str_ != NULL) {

      g_hInputFile = CreateFile(
         (working_directory + "\\" + (*stdin_str_)).c_str(), 
         GENERIC_READ, 
         0, 
         NULL, 
         OPEN_EXISTING, 
         FILE_ATTRIBUTE_READONLY, 
         NULL); 
       
   if ( g_hInputFile == INVALID_HANDLE_VALUE ) 
      throw "Stdin file open error";

   if ( ! CloseHandle(g_hChildStd_IN_Wr) ) 
      throw "StdInWr CloseHandle error";

   PipeThreadArg *arg_in = new PipeThreadArg;
   arg_in->child = g_hInputFile;
   arg_in->parent = g_hChildStd_IN_Wr;

   if (!CreateThreadFunction(&pipe_handler, arg_in)) {
     delete arg_in;
   }

   } // end of the stdin handler

   if (stdout_str_ != NULL) {

   g_hOutputFile = CreateFile (
         (working_directory + "\\" + (*stdout_str_)).c_str(), 
         GENERIC_WRITE, 
         0, 
         NULL, 
         CREATE_NEW, 
         FILE_ATTRIBUTE_NORMAL, 
         NULL); 
       
   if ( g_hOutputFile == INVALID_HANDLE_VALUE ) 
     throw "Stdout file creation error";

   if (!CloseHandle(g_hChildStd_OUT_Wr)) 
     throw "StdoutWr CloseHandle error";

   PipeThreadArg *arg_out = new PipeThreadArg;
   arg_out->child = g_hChildStd_OUT_Rd;
   arg_out->parent = g_hOutputFile;

   if (!CreateThreadFunction(&pipe_handler, arg_out)) {
     delete arg_out;
   }

   } // end of the stdout handler

   if (stderr_str_ != NULL) {

   g_hErrorFile = CreateFile (
         (working_directory + "\\" + (*stderr_str_)).c_str(), 
         GENERIC_WRITE, 
         0, 
         NULL, 
         CREATE_NEW, 
         FILE_ATTRIBUTE_NORMAL, 
         NULL); 

   if ( g_hErrorFile == INVALID_HANDLE_VALUE ) 
      throw "Stderr file creation error";

   if (!CloseHandle(g_hChildStd_ERR_Wr)) 
      throw "StdErrWr CloseHandle error";

   PipeThreadArg *arg_err = new PipeThreadArg;
   arg_err->child = g_hChildStd_ERR_Rd;
   arg_err->parent = g_hErrorFile;

   if (!CreateThreadFunction(&pipe_handler, arg_err)) {
     delete arg_err;
   }

   } // end of the stderr handler

    } // end of the try block

    catch(char * str) {
       std::cerr << "Exception raised: " << str << std::endl;
    }
    catch (std::exception& e) {
      running_ = false;
      std::cerr << e.what() << std::endl;
      return false;
    }
    catch (Glib::Exception& e) {
      std::cerr << e.what() << std::endl;
    }
    return true;
  }

  void pipe_handler(void *arg) {
    DWORD dwRead, dwWritten; 
    CHAR chBuf[BUFSIZE];
    BOOL bSuccess = FALSE;

    PipeThreadArg *thrarg = (PipeThreadArg*)arg;

    for (;;) {
      bSuccess = ReadFile(thrarg->child, chBuf, BUFSIZE, &dwRead, NULL);
      if( ! bSuccess || dwRead == 0 ) break;

      bSuccess = WriteFile(thrarg->parent, chBuf, 
                           dwRead, &dwWritten, NULL);
      if (! bSuccess ) break; 
    } 

    if (!CloseHandle(thrarg->parent)) 
      throw "Parent CloseHandle";
  }

  void Run::Kill(int timeout) {
    if (!running_)
      return;
    // Kill with no merci
    running_ = false;
    TerminateProcess(pid_->processinfo.hProcess, 256);
  }

  bool Run::stdout_handler(Glib::IOCondition) {
    return true;
  }

  bool Run::stderr_handler(Glib::IOCondition) {
    return true;
  }

  bool Run::stdin_handler(Glib::IOCondition) {
    return true;
  }

  void Run::child_handler(Glib::Pid, int result) {
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
    return 0;
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
