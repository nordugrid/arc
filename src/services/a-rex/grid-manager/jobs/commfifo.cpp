#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//@ #include "../std.h"
//@
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

//@ 
#include "commfifo.h"


CommFIFO::CommFIFO(void) {
  timeout_=-1;
  //@ lock.block();
  lock.lock();
  int filedes[2];
  kick_in=-1; kick_out=-1;
  if(pipe(filedes) == 0) {
    kick_in=filedes[1];
    kick_out=filedes[0];
    long arg;
    arg=fcntl(kick_in,F_GETFL);
    if(arg != -1) { arg|=O_NONBLOCK; fcntl(kick_in,F_SETFL,&arg); };
    arg=fcntl(kick_out,F_GETFL);
    if(arg != -1) { arg|=O_NONBLOCK; fcntl(kick_out,F_SETFL,&arg); };
  };
  //@ lock.unblock();
  lock.unlock();
}

CommFIFO::~CommFIFO(void) {
}

JobUser* CommFIFO::wait(int timeout) {
  for(;;) {
    fd_set fin,fout,fexc;
    FD_ZERO(&fin); FD_ZERO(&fout); FD_ZERO(&fexc);
    int maxfd=-1;
    if(kick_out >= 0) { maxfd=kick_out; FD_SET(kick_out,&fin); };
    //@ lock.block();
    lock.lock();
    for(std::list<elem_t>::iterator i = fds.begin();i!=fds.end();++i) {
      if(i->fd < 0) continue;
      if(i->fd>maxfd) maxfd=i->fd;
      FD_SET(i->fd,&fin);
    };
    //@ lock.unblock();
    lock.unlock();
    int n;
    maxfd++;
    if(timeout >= 0) {
      struct timeval t;
      t.tv_sec=timeout;
      t.tv_usec=0;
      n = select(maxfd,&fin,&fout,&fexc,&t);
    } else {
      n = select(maxfd,&fin,&fout,&fexc,NULL);
    };
    if(n == 0) return NULL;
    if(n == -1) {
      // One of fifos must be broken ?

    };
    if(kick_out >= 0) {
      if(FD_ISSET(kick_out,&fin)) {
        char buf[256]; ssize_t l = read(kick_out,buf,256);
        continue;
      };
    };
    //@ lock.block();
    lock.lock();
    for(std::list<elem_t>::iterator i = fds.begin();i!=fds.end();++i) {
      if(i->fd < 0) continue;
      if(FD_ISSET(i->fd,&fin)) {
        //@ lock.unblock();
        lock.unlock();
        char buf[256]; ssize_t l = read(i->fd,buf,256);
        // -1 ???
        return i->user;
      };
    };
    //@ lock.unblock();
    lock.unlock();
  };
}

bool CommFIFO::add(JobUser& user) {
  std::string path = user.ControlDir() + "/gm." + user.UnixName() + ".fifo";
  if(mkfifo(path.c_str(),S_IRUSR | S_IWUSR) != 0) {
    if(errno != EEXIST) {
      return false; 
    };
  };
  (void)chmod(path.c_str(),S_IRUSR | S_IWUSR);
  uid_t uid = user.get_uid();
  gid_t gid = user.get_gid();
  lchown(path.c_str(),uid,gid);
  int fd = open(path.c_str(),O_RDONLY | O_NONBLOCK);
  if(fd == -1) return false;
  int fd_keep = open(path.c_str(),O_WRONLY | O_NONBLOCK);
  if(fd_keep == -1) { close(fd); return false; };
  elem_t el; el.user=&user; el.fd=fd; el.fd_keep=fd_keep;
  //@ lock.block();
  lock.lock();
  fds.push_back(el);
  //@ lock.unblock();
  lock.unlock();
  if(kick_in >= 0) {
    char c = 0;
    int l = write(kick_in,&c,1);
  };
  return true;
}

bool SignalFIFO(const JobUser& user) {
  std::string path = user.ControlDir() + "/gm." + user.UnixName() + ".fifo";
  int fd = open(path.c_str(),O_WRONLY | O_NONBLOCK);
  if(fd == -1) {
    // If there is no fifo for this user, try to use common one
    path=user.ControlDir() + "/gm..fifo";
    fd=open(path.c_str(),O_WRONLY | O_NONBLOCK);
    if(fd == -1) return false;
  };
  char c = 0;
  if(write(fd,&c,1) != 1) { close(fd); return false; };
  close(fd);
  return true;
}

