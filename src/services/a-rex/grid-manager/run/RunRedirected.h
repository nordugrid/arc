#ifndef GRID_MANAGER_RUN_REDIRECTED_H
#define GRID_MANAGER_RUN_REDIRECTED_H

#include <arc/Run.h>
#include <arc/User.h>

namespace ARex {

/// Run child process with stdin, stdout and stderr redirected to specified handles
class RunRedirected {
 private:
  RunRedirected(int in,int out,int err):stdin_(in),stdout_(out),stderr_(err) { };
  ~RunRedirected(void) { };
  int stdin_;
  int stdout_;
  int stderr_;
  static void initializer(void* arg);
 public:
  operator bool(void) { return true; };
  bool operator!(void) { return false; };
  static int run(const Arc::User& user,const char* cmdname,int in,int out,int err,char *const args[],int timeout);
  static int run(const Arc::User& user,const char* cmdname,int in,int out,int err,const char* cmd,int timeoutd);
};

} // namespace ARex

#endif
