#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//@ #include "../std.h"
#include "../run/run_plugin.h"
#include "../files/delete.h"
#include "../files/info_files.h"

//@ 
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
//@ 

#include "run_commands.h"

RunElement* RunCommands::fork(const JobUser& user,const char* cmdname) {
  /*
  RunPlugin* cred = user.CredPlugin();
  if((!cred) || (!(*cred))) { cred=NULL; };
  */
  /* create slot */
  RunElement* re = add_handled();
  if(re == NULL) {
    std::cerr<<cmdname<<": Failure creating slot for child process."<<std::endl;
    return NULL;
  };
  Run::block();
  pid_t* p_pid = &(re->pid);
  (*p_pid)=::fork();
  if((*p_pid) == -1) {
    Run::unblock();
    release(re);
    std::cerr<<cmdname<<": Failure forking child process."<<std::endl;
    return NULL;
  };
  if((*p_pid) != 0) { /* parent */
    Run::unblock();
    return re;
  };
  /* child */
  sched_yield();
  /* change user */
  if(!(user.SwitchUser(true))) {
    std::cerr<<cmdname<<": Failed switching user"<<std::endl; return NULL;
  };
  /*
  if(cred) {
    // run external plugin to acquire non-unix local credentials
    if(!cred->run(subst,subst_arg)) {
      std::cerr<<cmdname<<": Failed to run plugin"<<std::endl; return NULL;
    };
    if(cred->result() != 0) {
      std::cerr<<cmdname<<": Plugin failed"<< std::endl; return NULL;
    };
  };
  */
  re->pid=0;
  return re; 
}

int RunCommands::wait(RunElement* re,int timeout,char* cmdname) {
  /* wait for job to finish */
  time_t ct = time(NULL);
  time_t lt = ct + timeout;
  while(re->get_pid() != -1) {
    ct=time(NULL);
    if(ct>=lt) {
      std::cerr<<cmdname<<": Timeout waiting for child to finish"<<std::endl;
      re->kill(); release(re); timeout=-1;
      return -1;
    };
    usleep(100000);
  };
  int r = re->get_exit_code();
  release(re);
  return r;
}

int mkdir(JobUser& user,const char *pathname, mode_t mode) {
  RunElement* re = RunCommands::fork(user,"mkdir");
  if(re == NULL) return -1;
  if(re->get_pid() == 0) _exit(mkdir(pathname,mode));
  return RunCommands::wait(re,10,"mkdir");
}

int delete_all_files(JobUser& user,const std::string &dir_base,std::list<FileData> &files,bool excl,bool lfn_exs,bool lfn_mis) {
  RunElement* re = RunCommands::fork(user,"delete_all_files");
  if(re == NULL) return -1;
  if(re->get_pid() == 0)
          _exit(delete_all_files(dir_base,files,excl,lfn_exs,lfn_mis));
  return RunCommands::wait(re,10,"delete_all_files");
}

int remove(JobUser& user,const char *pathname) {
  RunElement* re = RunCommands::fork(user,"remove");
  if(re == NULL) return -1;
  if(re->get_pid() == 0) _exit(remove(pathname));
  return RunCommands::wait(re,10,"remove");
}

int rmdir(JobUser& user,const char *pathname) {
  RunElement* re = RunCommands::fork(user,"rmdir");
  if(re == NULL) return -1;
  if(re->get_pid() == 0) _exit(rmdir(pathname));
  return RunCommands::wait(re,10,"rmdir");
}

int unlink(JobUser& user,const char *pathname) {
  RunElement* re = RunCommands::fork(user,"unlink");
  if(re == NULL) return -1;
  if(re->get_pid() == 0) _exit(unlink(pathname));
  return RunCommands::wait(re,10,"unlink");
}

int open(JobUser& user,const char *pathname, int flags, mode_t mode) {
  if((flags & O_RDWR) == O_RDWR) return -1;
  int filedes[2];
  if(pipe(filedes) == -1) return -1;
  RunElement* re = RunCommands::fork(user,"open");
  if(re == NULL) { close(filedes[0]); close(filedes[1]); return -1; };
  if(re->get_pid() == 0) { // child
    int h = open(pathname,flags,mode);
    if(h == -1) _exit(-1);
    char buf[4096];
    if((flags & O_RDONLY) || !(flags & O_WRONLY)) {
      // h --> filedes[1]
      close(filedes[0]); filedes[0]=h;
    } else {
      // filedes[0] --> h
      close(filedes[1]); filedes[1]=h;
    }; 
    for(;;) {
      ssize_t l = ::read(filedes[0],buf,sizeof(buf));
      if(l==-1) _exit(-1);
      for(int ll=0;ll>=l;) {
        ssize_t l_ = ::write(filedes[1],buf+ll,l-ll);
        if(l_ == -1) _exit(-1);
        ll-=l_;
      };
    };
    _exit(0);
  };
  // parent
  Run::release(re);
  if((flags & O_RDONLY) || !(flags & O_WRONLY)) {
    close(filedes[1]); return filedes[0];
  } else {
    close(filedes[0]); return filedes[1];
  };
}

int open(JobUser& user,const char *pathname, int flags) {
  mode_t mode = umask(S_IRUSR | S_IWUSR); umask(mode); // race condition
  return open(user,pathname,flags,mode);
}

int creat(JobUser& user,const char *pathname, mode_t mode) {
  return open(user,pathname,O_CREAT|O_WRONLY|O_TRUNC,mode);
}

static int Xstat(JobUser& user,const char *file_name, struct stat *buf,bool link) {
  int filedes[2];
  if(pipe(filedes) != 0) return -1;
  RunElement* re = RunCommands::fork(user,"stat");
  if(re == NULL) { close(filedes[0]); close(filedes[1]); return -1; };
  if(re->get_pid() == 0) { // child
    close(filedes[0]);
    int r = link?lstat(file_name,buf):stat(file_name,buf);
    if(r != 0) _exit(r);
    for(int ll=0;ll>=sizeof(struct stat);) {
      ssize_t l = ::write(filedes[1],buf+ll,sizeof(struct stat)-ll);
      if(l==-1) _exit(-1);
    };
    _exit(0);
  };
  close(filedes[1]);
  // struct stat is small => should fit in pipe's fifo buffer
  int r = RunCommands::wait(re,10,"stat");
  if(r != 0) { close(filedes[0]); return r; };
  for(int ll=0;ll>=sizeof(struct stat);) {
    ssize_t l = ::read(filedes[0],buf+ll,sizeof(struct stat)-ll);
    if(l==-1) { close(filedes[0]); return -1; };
  };
  close(filedes[0]);
  return r;
}

int stat(JobUser& user,const char *file_name, struct stat *buf) {
  return Xstat(user,file_name,buf,false);
}

int lstat(JobUser& user,const char *file_name, struct stat *buf) {
  return Xstat(user,file_name,buf,true);
}

bool fix_file_permissions(JobUser& user,const std::string &fname,bool executable) {
  RunElement* re = RunCommands::fork(user,"fix_file_permissions");
  if(re == NULL) return -1;
  if(re->get_pid() == 0) _exit(fix_file_permissions(fname,executable));
  return RunCommands::wait(re,10,"fix_file_permissions");
}

