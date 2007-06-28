#ifndef GRID_MANAGER_RUN_H
#define GRID_MANAGER_RUN_H

//@ #include "../std.h"
#include "../jobs/users.h"
#include "../jobs/states.h"
#include <sys/resource.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string>

extern char** environ;

class Run;
class RunParallel;
class RunCommands;

class RunElement {
 friend class Run;
 friend class RunParallel;
 friend class RunCommands;
 private:
  int pid;
  int exit_code;
  bool released;
  RunElement* next;
 public:
  void kill(int signum = SIGTERM) {
    int tmp_pid=pid; if(tmp_pid != -1) ::kill(tmp_pid,signum);
  };
  int get_exit_code(void) { return exit_code; };
  int get_pid(void) { return pid; };
  RunElement() {
    pid=0; exit_code=-1; released=false; next=NULL;
  };
  RunElement(const RunElement &re) { 
    pid=re.pid; exit_code=re.exit_code; released=re.released; next=NULL;
  };
  void reset(void) {
    pid=0; exit_code=-1; released=false;
  };
  ~RunElement() { };
};

class Run {
 private:
  bool initialized;
  static bool hup_detected;
  static RunElement dumb_re;
  static bool in_signal;
  static pthread_cond_t *cond;
  static pthread_mutex_t list_lock;
  static struct sigaction old_sig_chld; 
  static struct sigaction old_sig_hup; 
  static struct sigaction old_sig_term; 
  static bool old_sig_chld_inited; 
  static bool old_sig_hup_inited; 
  static bool old_sig_term_inited; 
  static void sig_hup(int signum,siginfo_t *info,void* arg);
  static void sig_chld(int signum,siginfo_t *info,void* arg);
  static void sig_chld_process(int signum,siginfo_t *info,void* arg);
  static void sig_term(int signum,siginfo_t *info,void* arg);
  static RunElement* begin;
  static pthread_t handler_thread;
  static bool handler_thread_inited;
  static void* signal_handler(void *arg);
  bool init(void);
  void deinit(void);
 public:
  static bool plain_run_piped(char *const args[],const std::string *Din,std::string *Dout,std::string *Derr,int& timeout,int* result);
  static bool plain_run_redirected(char *const args[],int din,int dout,int derr,int& timeout,int* result);
  static void release(RunElement* re);
  static RunElement* add_handled(void);
  bool is_initialized(void) { return initialized; };
  bool was_hup(void);
  void reinit(bool after_fork = true);
  static void block(void) { pthread_mutex_lock(&list_lock); };
  static void unblock(void) { pthread_mutex_unlock(&list_lock); };
  Run(pthread_cond_t *cond = NULL);
  ~Run();
};

#endif // GRID_MANAGER_RUN_H

