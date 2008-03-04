#ifndef __ARC_RUN_H__
#define __ARC_RUN_H__

#include <glibmm.h>

namespace Arc {

class RunPump;

/** This class runs external executable.
  It is possible to read/write it's standard handles
 or to redirect then to std::string elements. */
class Run {
 friend class RunPump;
 private:
  Run(const Run&);
  Run& operator=(Run&);
 protected:
  // Handles
  int stdout_;
  int stderr_;
  int stdin_;
  // Associated string containers
  std::string* stdout_str_;
  std::string* stderr_str_;
  std::string* stdin_str_;
  //
  bool stdout_keep_;
  bool stderr_keep_;
  bool stdin_keep_;
  // Signal connections
  sigc::connection stdout_conn_;
  sigc::connection stderr_conn_;
  sigc::connection stdin_conn_;
  sigc::connection child_conn_;
  // PID of child
  Glib::Pid pid_;
  // Arguments to execute
  Glib::ArrayHandle<std::string> argv_;
  void (*initializer_func_)(void*);
  void* initializer_arg_;
  void (*kicker_func_)(void*);
  void* kicker_arg_;
  // IO handlers
  bool stdout_handler(Glib::IOCondition cond);
  bool stderr_handler(Glib::IOCondition cond);
  bool stdin_handler(Glib::IOCondition cond);
  // Child exit handler
  void child_handler(Glib::Pid pid,int result);
  bool started_;
  bool running_;
  int result_;
  Glib::Mutex lock_;
  Glib::Cond  cond_;
 public:
  /** Constructor preapres object to run cmdline */
  Run(const std::string& cmdline);
  /** Constructor preapres object to run executable and arguments specified in argv */
  Run(const std::list<std::string>& argv);
  /** Destructor kill running executable and releases associated resources */
  ~Run(void);
  /** Returns true if object is valid */
  operator bool(void) { return argv_.size() != 0; };
  /** Returns true if object is invalid */
  bool operator!(void) { return argv_.size() == 0; };
  /** Starts running executable. 
    This method may be called only once. */
  bool Start(void);
  /** Wait till execution finished or till timeout seconds expires.
    Returns true if execution is complete. */
  bool Wait(int timeout);
  /** Returns exit code of execution. */
  int Result(void) { return result_; };
  /** Return true if execution is going on. */
  bool Running(void);
  /** Read from stdout handle of running executable.
    This method may be used while stdout is directed to string. 
   But result is unpredictable. */
  int ReadStdout(int timeout,char* buf,int size);
  /** Read from stderr handle of running executable.
    This method may be used while stderr is directed to string. 
   But result is unpredictable. */
  int ReadStderr(int timeout,char* buf,int size);
  /** Write to stdin handle of running executable.
    This method may be used while stdin is directed to string. 
   But result is unpredictable. */
  int WriteStdin(int timeout,const char* buf,int size);
  /** Associate stdout handle of executable with string.
    This method must be called before Start(). str object
   must be valid as long as this object exists. */
  void AssignStdout(std::string& str);
  /** Associate stderr handle of executable with string.
    This method must be called before Start(). str object
   must be valid as long as this object exists. */
  void AssignStderr(std::string& str);
  /** Associate stdin handle of executable with string.
    This method must be called before Start(). str object
   must be valid as long as this object exists. */
  void AssignStdin(std::string& str);
  /** Keep stdout same as parent's if keep = true */
  void KeepStdout(bool keep = true);
  /** Keep stderr same as parent's if keep = true */
  void KeepStderr(bool keep = true);
  /** Keep stdin same as parent's if keep = true */
  void KeepStdin(bool keep = true);
  /** Closes pipe associated with stdout handle */
  void CloseStdout(void);
  /** Closes pipe associated with stderr handle */
  void CloseStderr(void);
  /** Closes pipe associated with stdin handle */
  void CloseStdin(void);
  //void DumpStdout(void);
  //void DumpStderr(void);
  void AssignInitializer(void (*initializer_func)(void*),void* initializer_arg);
  void AssignKicker(void (*kicker_func)(void*),void* kicker_arg);
  /** Kill running executable. 
    First soft kill signal (SIGTERM) is sent to executable. If
   after timeout seconds executable is still running it's killed
   completely. Curently this method does not work for Windows OS */
  void Kill(int timeout);
};

}

#endif // __ARC_RUN_H__
