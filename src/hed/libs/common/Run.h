#include <glibmm.h>

namespace Arc {

class RunPump;

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
  // Signal connections
  sigc::connection stdout_conn_;
  sigc::connection stderr_conn_;
  sigc::connection stdin_conn_;
  sigc::connection child_conn_;
  // PID of child
  Glib::Pid pid_;
  // Arguments to execute
  Glib::ArrayHandle<std::string> argv_;
  // IO handlers
  bool stdout_handler(Glib::IOCondition cond);
  bool stderr_handler(Glib::IOCondition cond);
  bool stdin_handler(Glib::IOCondition cond);
  // Child exit handler
  void child_handler(Glib::Pid pid,int result);
  bool started_;
  bool running_;
  int result_;
 public:
  Run(const std::string& cmdline);
  Run(const std::list<std::string>& argv);
  ~Run(void);
  operator bool(void) { return argv_.size() != 0; };
  bool operator!(void) { return argv_.size() == 0; };
  bool Start(void);
  bool Wait(int timeout);
  int Result(void) { return result_; };
  bool Running(void) { return running_; };
  int ReadStdout(int timeout,char* buf,int size);
  int ReadStderr(int timeout,char* buf,int size);
  int WriteStdin(int timeout,const char* buf,int size);
  void AssignStdout(std::string& str);
  void AssignStderr(std::string& str);
  void AssignStdin(std::string& str);
  void CloseStdout(void);
  void CloseStderr(void);
  void CloseStdin(void);
  //void DumpStdout(void);
  //void DumpStderr(void);
  void Kill(int timeout);
};

}

