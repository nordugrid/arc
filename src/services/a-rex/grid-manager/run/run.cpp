//@ #include "../std.h"
#include <dlfcn.h>

#include "../jobs/users.h"
#include "../jobs/states.h"
#include <sys/resource.h>
#include <sys/wait.h>
#include <pthread.h>
#include "../conf/environment.h"
#include "../conf/conf.h"
//@ #include "../misc/substitute.h"
//@ #include "../misc/log_time.h"
#include "run.h"

//@
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#define olog std::cerr 
//@

extern char** environ;

bool Run::hup_detected = false;

bool Run::in_signal  = false;
RunElement* Run::begin   = NULL;
pthread_cond_t* Run::cond  = NULL;
pthread_t Run::handler_thread;
bool Run::handler_thread_inited = false;

pthread_mutex_t Run::list_lock = PTHREAD_MUTEX_INITIALIZER;


// static Run run_environment; // father of all - it must be at least one

/* Linux specific - stupid g++ limitation */
// struct sigaction Run::old_sig_chld = { SIG_ERR, { }, ~SA_SIGINFO };
// struct sigaction Run::old_sig_hup  = { SIG_ERR, { }, ~SA_SIGINFO };
struct sigaction Run::old_sig_chld = {  };
struct sigaction Run::old_sig_hup  = {  };
struct sigaction Run::old_sig_term  = {  };
bool Run::old_sig_chld_inited = false;
bool Run::old_sig_hup_inited  = false;
bool Run::old_sig_term_inited  = false;

RunElement* Run::add_handled(void) {
  pthread_mutex_lock(&list_lock);
  RunElement* re;
  for(re=begin;re;re=re->next) {
    if((re->released) && (re->pid == -1)) {
      re->reset();
      break;
    };
  };
  if(re == NULL) {
    re = new RunElement;
    if(re == NULL) return NULL;
    re->next=begin;
    begin=re;
  };
  pthread_mutex_unlock(&list_lock);
  return re;
}

// This function can be called only once (like free() ).
void Run::release(RunElement* re) {
  if(re != NULL) {
    re->released=true;
  };
}

/* restore signal handlers */
void Run::deinit(void) {
  if(old_sig_chld_inited) 
    sigaction(SIGCHLD,&old_sig_chld,NULL);
  if(old_sig_hup_inited) 
    sigaction(SIGHUP,&old_sig_hup,NULL);
  if(old_sig_term_inited) 
    sigaction(SIGTERM,&old_sig_term,NULL);
}

/* setup environment to catch child's death signal */
bool Run::init(void) {
  pthread_mutex_lock(&list_lock);
  /* Try hard to block SIGCHLD */
  sigset_t sig;
  sigemptyset(&sig);
  sigaddset(&sig,SIGCHLD);
  // if(sigprocmask(SIG_BLOCK,&sig,NULL)) perror("sigprocmask");
#ifndef NONPOSIX_SIGNALS_IN_THREADS
  if(pthread_sigmask(SIG_BLOCK,&sig,NULL)) perror("pthread_sigmask");
#else
  if(sigprocmask(SIG_UNBLOCK,&sig,NULL)) perror("sigprocmask");
#endif
  /* Set actions for other signals and SIGCHLD just in case */
  struct sigaction act;
  if(!old_sig_chld_inited) { 
    act.sa_sigaction=&sig_chld;
    sigemptyset(&(act.sa_mask)); sigaddset(&(act.sa_mask),SIGHUP);
    act.sa_flags=SA_NOCLDSTOP | SA_SIGINFO;
    if(sigaction(SIGCHLD,&act,&old_sig_chld) == -1) {
      pthread_mutex_unlock(&list_lock);
      olog << "Failed setting signal handler" << std::endl; return false;
    };
    old_sig_chld_inited=true; 
  };
  if(!old_sig_hup_inited) { 
    act.sa_sigaction=&sig_hup;
    sigemptyset(&(act.sa_mask)); sigaddset(&(act.sa_mask),SIGCHLD);
    act.sa_flags=SA_SIGINFO;
    if(sigaction(SIGHUP,&act,&old_sig_hup) == -1) {
      pthread_mutex_unlock(&list_lock);
      olog << "Failed setting signal handler" << std::endl; return false;
    };
    old_sig_hup_inited=true;
  };
  if(!old_sig_term_inited) {
    act.sa_sigaction=&sig_term;
    sigemptyset(&(act.sa_mask)); sigaddset(&(act.sa_mask),SIGCHLD);
    act.sa_flags=SA_SIGINFO;
    if(sigaction(SIGTERM,&act,&old_sig_hup) == -1) {
      pthread_mutex_unlock(&list_lock);
      olog << "Failed setting signal handler" << std::endl; return false;
    };
    old_sig_term_inited=true; 
  };
  if(!handler_thread_inited) {
#ifndef NONPOSIX_SIGNALS_IN_THREADS
    if(pthread_create(&handler_thread,NULL,&signal_handler,this) != 0) {
      olog << "Failed to create thread for handling signals"<<std::endl;
    };
#endif
    handler_thread_inited=true;
  };
  pthread_mutex_unlock(&list_lock);
  return true;
}

void* Run::signal_handler(void *arg) {
  for(;;) {
    sigset_t sig;
    sigemptyset(&sig);
    sigaddset(&sig,SIGCHLD);
    siginfo_t info;
#ifndef NONPOSIX_SIGNALS_IN_THREADS
    sigwaitinfo(&sig,&info);
    if(info.si_signo == SIGCHLD) {
      pthread_mutex_lock(&list_lock);
      sig_chld_process(SIGCHLD,&info,NULL);
      pthread_mutex_unlock(&list_lock);
    };
#else
    struct timespec tm;
    tm.tv_sec=60; tm.tv_nsec=0;
    int r = sigtimedwait(&sig,&info,&tm);
#endif
  };
  return NULL;
}

// Force re-setting signal handlers (after fork)
void Run::reinit(bool after_fork) { 
  deinit();
  old_sig_chld_inited=false; 
  old_sig_hup_inited=false; 
  old_sig_term_inited=false; 
  if(after_fork) handler_thread_inited=false; 
  // make sure mutex is not locked by non-existing thread
  if(after_fork) pthread_mutex_unlock(&list_lock);
  init();
}

/* constructor - just init handlers */
Run::Run(pthread_cond_t *cond_) {
  initialized=false;
  if(!init()) { deinit(); return; }
  cond=cond_;
  initialized=true;
}

// destructor - just kill processes. This most probaly will never happen.
Run::~Run(void) {
  if(initialized) {
    pthread_mutex_lock(&list_lock);
    for(RunElement* p=begin;p;p=p->next) p->kill();
    pthread_mutex_unlock(&list_lock);
    deinit();
  };
}

bool Run::was_hup(void) {
  bool res = hup_detected;
  hup_detected = false;
  return res;
};

// Termination signal
void Run::sig_term(int signum,siginfo_t *info,void* arg) {
  /* propagate to all children */
  kill(0,SIGTERM);
  exit(1);
}

// Wake up kind signal
void Run::sig_hup(int signum,siginfo_t *info,void* arg) {
  hup_detected = true;
  if(cond) { pthread_cond_signal(cond); };
//  else { olog << "Warning: sleep condition is undefined" << endl; };
  /* assuming that with SA_SIGINFO always come proper handler */
  /* Disable chain of handlers - more race auditing needed
  if(old_sig_hup_inited) {
    if(old_sig_hup.sa_flags & SA_SIGINFO) {
      (*(old_sig_chld.sa_sigaction))(signum,info,arg);
    }
    else {
      if(old_sig_hup_inited) { 
#ifndef _BROKEN_SIGNAL_HANDLER_DEFINITION_
        if((old_sig_hup.sa_handler != SIG_IGN) && 
           (old_sig_hup.sa_handler != SIG_ERR) && 
           (old_sig_hup.sa_handler != SIG_DFL)) {
          (*(old_sig_chld.sa_handler))(signum);
#else
        typedef void (*signal_handler_t) (int);
        if(((signal_handler_t)(old_sig_hup.sa_handler) != SIG_IGN) && 
           ((signal_handler_t)(old_sig_hup.sa_handler) != SIG_ERR) && 
           ((signal_handler_t)(old_sig_hup.sa_handler) != SIG_DFL)) {
          (*((signal_handler_t)(old_sig_chld.sa_handler)))(signum);
#endif
        };
      };
    };
  };
  */
}

// Child terminated signal
void Run::sig_chld(int signum,siginfo_t *info,void* arg) {
  // Make sure signal is re-delivered to proper thread
#ifndef NONPOSIX_SIGNALS_IN_THREADS
  pthread_kill(handler_thread,SIGCHLD);
#else
  sig_chld_process(signum,info,arg);
#endif
}

// Process child terminated signal synchronously
void Run::sig_chld_process(int signum,siginfo_t *info,void* arg) {
 /* You can not be sure if current implementation of
    posix threads does not use SIGCHLD, so lets check
    only our children */
  if(info == NULL) {
    return;
  };
  in_signal = true;
  bool child_exited = false;
#ifndef _BROKEN_SIGNAL_HANDLER_DEFINITION_
  bool our_child_exited = false;
  pid_t pid=info->si_pid;
#else
  bool our_child_exited = true;
#endif
  RunElement* p=begin;
  for(;p;p=p->next) {
#ifndef _BROKEN_SIGNAL_HANDLER_DEFINITION_
    if(p->pid == pid) {  /* our child */
      our_child_exited = true;
    };
#endif
    if(p->pid <= 0) continue;
    /* remove zombies */ 
    /* !!!! ATTENTION - it looks like signals can be lost
       or are not queued properly (the same). So check ALL
       childern here */
    int status;  
    pid_t ch_id=waitpid(p->pid,&status,WNOHANG);
    if(ch_id == p->pid) {
      child_exited = true;
      if(WIFEXITED(status)) { /* Child exited voluntarily */
        p->exit_code=WEXITSTATUS(status); 
      }
      else { /* child did not exit itself - can retry ? */
        p->exit_code=2;  
      };
      p->pid=-1; // marks no more running process
    };
  };
  if(child_exited) {
    /* wake up main loop */
    /* not using mutex because can't be sure this won't lock forewer */
    if(cond) { pthread_cond_signal(cond); };
  };
  in_signal = false;
  if(our_child_exited) return;
  //  Unknown child exited - propagate signal to old handlers
  /* Better race audit is required - disable propagation
  if(old_sig_chld_inited) {
    if(old_sig_chld.sa_flags & SA_SIGINFO) {
      (*(old_sig_chld.sa_sigaction))(signum,info,arg);
    } 
    else {
      if(old_sig_chld_inited) { 
#ifndef _BROKEN_SIGNAL_HANDLER_DEFINITION_
        if((old_sig_chld.sa_handler != SIG_IGN) && 
           (old_sig_chld.sa_handler != SIG_ERR) && 
           (old_sig_chld.sa_handler != SIG_DFL)) {
          (*(old_sig_chld.sa_handler))(signum);
#else
        typedef void (*signal_handler_t) (int);
        if(((signal_handler_t)(old_sig_chld.sa_handler) != SIG_IGN) && 
           ((signal_handler_t)(old_sig_chld.sa_handler) != SIG_ERR) && 
           ((signal_handler_t)(old_sig_chld.sa_handler) != SIG_DFL)) {
          (*((signal_handler_t)(old_sig_chld.sa_handler)))(signum);
#endif
        };
      };
    };
  };
  */
}



bool Run::plain_run_redirected(char *const args[],int din,int dout,int derr,int& timeout,int* result) {
  RunElement* re = add_handled();
  if(re == NULL) {
    olog << "Failure creating slot for child process." << std::endl;
    return false;
  };
  block();
  pid_t* p_pid = &(re->pid);
  pid_t cpid;
  // { sigset_t sig; sigemptyset(&sig); sigaddset(&sig,SIGCHLD); if(sigprocmask(SIG_BLOCK,&sig,NULL)) perror("sigprocmask"); };
  (*p_pid)=fork();
  if((*p_pid)==-1) {
    unblock();
    // { sigset_t sig; sigemptyset(&sig); sigaddset(&sig,SIGCHLD); if(sigprocmask(SIG_UNBLOCK,&sig,NULL)) perror("sigprocmask"); };
    olog << "Failure forking child process." << std::endl;
    release(re);
    return false;
  };
  if((*p_pid) != 0) { /* parent */
    close(din); close(dout); close(derr);
    unblock();
    // { sigset_t sig; sigemptyset(&sig); sigaddset(&sig,SIGCHLD); if(sigprocmask(SIG_UNBLOCK,&sig,NULL)) perror("sigprocmask"); };
    time_t ct = time(NULL);
    time_t lt = ct + timeout;
    /* wait for job to finish */
    while(re->get_pid() != -1) {
      ct=time(NULL);
      if(ct>=lt) {
        olog << "Timeout waiting for child to finish"<< std::endl;
        re->kill(); release(re); timeout=-1;
        return false;
      };
      usleep(100000);
    };
    if(result) (*result)=re->get_exit_code();
    release(re); timeout=lt-ct;
    return true;
  };
  /* child */
  sched_yield();
  /* set up stdin,stdout and stderr */
  if(din != -1) {
    close(0);
    if(dup2(din,0) != 0) { perror("dup2"); exit(1); };
  };
  if(dout != -1) {
    close(1);
    if(dup2(dout,1) != 1) { perror("dup2"); exit(1); };
  };
  if(derr != -1) {
    close(2);
    if(dup2(derr,2) != 2) { perror("dup2"); exit(1); };
  };
  struct rlimit lim;
  int max_files = 4096;
  if(getrlimit(RLIMIT_NOFILE,&lim) != 0) { max_files=lim.rlim_cur; };
  /* close all handles inherited from parent */
  if(max_files == RLIM_INFINITY) max_files=4096;
  for(int i=3;i<max_files;i++) { close(i); };
  /* run executable */
  execv(args[0],args);
  perror("execv");
  std::cerr<<"Failed to start external program: "<<args[0]<<std::endl;
  exit(1);
}

bool Run::plain_run_piped(char *const args[],const std::string *Din,std::string *Dout,std::string *Derr,int& timeout,int* result) {
  int p[2];
  // establsh pipes and/or handles
  int din = -1;
  int dout = -1;
  int derr = -1;
  int din_ = -1;
  int dout_ = -1;
  int derr_ = -1;
  if(Din) {
    if(pipe(p) == 0) { din=p[1]; din_=p[0]; };
  } else {
    din_=open("/dev/null",O_RDONLY);
  };
  if(Dout) {
    if(pipe(p) == 0) { dout=p[0]; dout_=p[1]; };
  } else {
    dout_=open("/dev/null",O_WRONLY);
  };
  if(Derr) {
    if(pipe(p) == 0) { derr=p[0]; derr_=p[1]; };
  } else {
    derr_=open("/dev/null",O_WRONLY);
  };
  if((din_==-1) || (dout_==-1) || (derr_==-1)) {
    olog << "Failure opening pipes." << std::endl;
    if(din != -1) close(din);   if(din_ != -1) close(din_);
    if(dout != -1) close(dout); if(dout_ != -1) close(dout_);
    if(derr != -1) close(derr); if(derr_ != -1) close(derr_);
    return false;
  };
  /* create slot */
  RunElement* re = add_handled();
  if(re == NULL) {
    olog << "Failure creating slot for child process." << std::endl;
    if(din != -1) close(din);   if(din_ != -1) close(din_);
    if(dout != -1) close(dout); if(dout_ != -1) close(dout_);
    if(derr != -1) close(derr); if(derr_ != -1) close(derr_);
    return false;
  };
  block();
  pid_t* p_pid = &(re->pid);
  pid_t cpid;
  // { sigset_t sig; sigemptyset(&sig); sigaddset(&sig,SIGCHLD); if(sigprocmask(SIG_BLOCK,&sig,NULL)) perror("sigprocmask"); };
  (*p_pid)=fork();
  if((*p_pid)==-1) {
    unblock();
    // { sigset_t sig; sigemptyset(&sig); sigaddset(&sig,SIGCHLD); if(sigprocmask(SIG_UNBLOCK,&sig,NULL)) perror("sigprocmask"); };
    olog << "Failure forking child process." << std::endl;
    release(re);
    if(din != -1) close(din);   if(din_ != -1) close(din_);
    if(dout != -1) close(dout); if(dout_ != -1) close(dout_);
    if(derr != -1) close(derr); if(derr_ != -1) close(derr_);
    return false;
  };
  if((*p_pid) != 0) { /* parent */
    close(din_); close(dout_); close(derr_);
    unblock();
    // { sigset_t sig; sigemptyset(&sig); sigaddset(&sig,SIGCHLD); if(sigprocmask(SIG_UNBLOCK,&sig,NULL)) perror("sigprocmask"); };
    /* pipe to stdin */
    if(Din) { write(din,Din->c_str(),Din->length()); close(din); din=-1; };
    time_t ct = time(NULL);
    time_t lt = ct + timeout;
    if(Dout || Derr) {
    /* read from stdout and stderr */
      int sdmax = din; if(dout>sdmax) sdmax=dout; if(derr>sdmax) sdmax=derr;
      fd_set sdin,sdout,sderr;
      for(;;) {
        FD_ZERO(&sdin); FD_ZERO(&sdout); FD_ZERO(&sderr);
        if(dout!=-1) FD_SET(dout,&sdin);
        if(derr!=-1) FD_SET(derr,&sdin);
        struct timeval to;
        to.tv_usec=0; to.tv_sec=lt-ct;
        int n = select(sdmax+1,&sdin,&sdout,&sderr,&to);
        if(n==0) { // timeout
          olog << "Timeout waiting for child to finish"<< std::endl;
          if(dout!=-1) { close(dout); dout=-1; };
          if(derr!=-1) { close(derr); derr=-1; };
          re->kill(); release(re); timeout=-1;
          return false; 
        };
        if(dout!=-1) if((n>0) && FD_ISSET(dout,&sdin)) {
          char buf[256];
          int l = read(dout,buf,255);
          if(l==0) { close(dout); dout=-1; } // eof
          else { buf[l]=0; (*Dout)+=buf; };
        };
        if(derr!=-1) if((n>0) && FD_ISSET(derr,&sdin)) {
          char buf[256];
          int l = read(derr,buf,255);
          if(l==0) { close(derr); derr=-1; } // eof
          else { buf[l]=0; (*Derr)+=buf; };
        };
        if((dout==-1) && (derr==-1)) break;
        ct=time(NULL);
        if(ct>=lt) { // timeout
          olog << "Timeout waiting for child to finish"<< std::endl;
          if(dout!=-1) { close(dout); dout=-1; };
          if(derr!=-1) { close(derr); derr=-1; };
          re->kill(); release(re); timeout=-1;
          return false; 
        };
      };
    };
    /* wait for job to finish */
    while(re->get_pid() != -1) {
      ct=time(NULL);
      if(ct>=lt) {
        olog << "Timeout waiting for child to finish"<< std::endl;
        re->kill(); release(re); timeout=-1;
        return false;
      };
      usleep(100000);
    };
    if(result) (*result)=re->get_exit_code();
    release(re); timeout=lt-ct;
    return true;
  };
  /* child */
  sched_yield();
  /* set up stdin,stdout and stderr */
  close(0); close(1); close(2);
  if(dup2(din_,0) != 0) { perror("dup2"); exit(1); };
  if(dup2(dout_,1) != 1) { perror("dup2"); exit(1); };
  if(dup2(derr_,2) != 2) { perror("dup2"); exit(1); };
  struct rlimit lim;
  int max_files = 4096;
  if(getrlimit(RLIMIT_NOFILE,&lim) != 0) { max_files=lim.rlim_cur; };
  /* close all handles inherited from parent */
  if(max_files == RLIM_INFINITY) max_files=4096;
  for(int i=3;i<max_files;i++) { close(i); };
  /* run executable */
  execv(args[0],args);
  perror("execv");
  std::cerr<<"Failed to start external program: "<<args[0]<<std::endl;
  exit(1);
}
