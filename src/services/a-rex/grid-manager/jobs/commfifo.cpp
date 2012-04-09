#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "commfifo.h"


#ifndef WIN32

bool CommFIFO::make_pipe(void) {
  bool res = false;
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
    res = (kick_in != -1);
  };
  lock.unlock();
  return res;
}

CommFIFO::CommFIFO(void) {
  timeout_=-1;
  kick_in=-1; kick_out=-1;
  make_pipe();
}

CommFIFO::~CommFIFO(void) {
}

JobUser* CommFIFO::wait(int timeout) {
  time_t start_time = time(NULL);
  time_t end_time = start_time + timeout;
  for(;;) {
    fd_set fin,fout,fexc;
    FD_ZERO(&fin); FD_ZERO(&fout); FD_ZERO(&fexc);
    int maxfd=-1;
    if(kick_out < 0) make_pipe();
    if(kick_out >= 0) { maxfd=kick_out; FD_SET(kick_out,&fin); };
    lock.lock();
    for(std::list<elem_t>::iterator i = fds.begin();i!=fds.end();++i) {
      if(i->fd < 0) continue;
      if(i->fd>maxfd) maxfd=i->fd;
      FD_SET(i->fd,&fin);
    };
    lock.unlock();
    int err;
    maxfd++;
    if(timeout >= 0) {
      struct timeval t;
      if(((int)(end_time-start_time)) < 0) return NULL;
      t.tv_sec=end_time-start_time;
      t.tv_usec=0;
      if(maxfd > 0) {
        err = select(maxfd,&fin,&fout,&fexc,&t);
      } else {
        sleep(t.tv_sec);
        err = 0;
      };
      start_time = time(NULL);
    } else {
      if(maxfd > 0) {
        err = select(maxfd,&fin,&fout,&fexc,NULL);
      } else {
        err = 0;
      };
    };
    if(err == 0) return NULL;
    if(err == -1) {
      if(errno == EBADF) {
        // One of fifos must be broken. Let read() find that out.
      } else if(errno == EINTR) {
        // interrupted by signal, retry
        continue;
      };
      // No idea how this could happen and how to deal with it.
      // Lets try to escape and start from beginning
      return NULL;
    };
    if(kick_out >= 0) {
      if((err < 0) || FD_ISSET(kick_out,&fin)) {
        char buf[256];
        if(read(kick_out,buf,256) != -1) {
          close(kick_in); close(kick_out);
          make_pipe();
        };
        continue;
      };
    };
    lock.lock();
    for(std::list<elem_t>::iterator i = fds.begin();i!=fds.end();++i) {
      if(i->fd < 0) continue;
      if((err < 0) || FD_ISSET(i->fd,&fin)) {
        lock.unlock();
        char buf[256];
        ssize_t l = read(i->fd,buf,sizeof(buf));
        if(l < 0) {
          if((errno == EBADF) || (errno == EINVAL) || (errno == EIO)) {
            close(i->fd); close(i->fd_keep);
            i->fd = -1; i->fd_keep = -1;
          };
        } else if(l > 0) {
          // 0 means kick, 1 - ping, rest undefined yet
          if(memchr(buf,0,sizeof(buf))) return i->user;
        };
      };
    };
    lock.unlock();
  };
}

CommFIFO::add_result CommFIFO::add(JobUser& user) {
  std::string path = user.ControlDir() + "/gm." + user.UnixName() + ".fifo";
  if(mkfifo(path.c_str(),S_IRUSR | S_IWUSR) != 0) {
    if(errno != EEXIST) {
      return add_error; 
    };
  };
  (void)chmod(path.c_str(),S_IRUSR | S_IWUSR);
  uid_t uid = user.get_uid();
  gid_t gid = user.get_gid();
  (lchown(path.c_str(),uid,gid) != 0);
  int fd = -1;
  // This must fail. If not then there is another a-rex hanging around.
  fd = open(path.c_str(),O_WRONLY | O_NONBLOCK);
  if(fd != -1) { close(fd); return add_busy; };
  // (errno != ENXIO)) {
  fd = open(path.c_str(),O_RDONLY | O_NONBLOCK);
  if(fd == -1) return add_error;
  int fd_keep = open(path.c_str(),O_WRONLY | O_NONBLOCK);
  if(fd_keep == -1) { close(fd); return add_error; };
  elem_t el; el.user=&user; el.fd=fd; el.fd_keep=fd_keep;
  lock.lock();
  fds.push_back(el);
  lock.unlock();
  if(kick_in >= 0) {
    char c = 0;
    (write(kick_in,&c,1) != -1);
  };
  return add_success;
}

static int OpenFIFO(const JobUser& user) {
  // Here O_NONBLOCK ensures open() will fail if nothing listens
  std::string path = user.ControlDir() + "/gm." + user.UnixName() + ".fifo";
  int fd = open(path.c_str(),O_WRONLY | O_NONBLOCK);
  if(fd == -1) {
    // If there is no fifo for this user, try to use common one
    path=user.ControlDir() + "/gm..fifo";
    fd=open(path.c_str(),O_WRONLY | O_NONBLOCK);
  };
  // If fd == -1 here there is no FIFO or nothing is listening on another end
  return fd;
}

bool SignalFIFO(const JobUser& user) {
  int fd = OpenFIFO(user);
  if(fd == -1) return false;
  char c = 0;
  if(write(fd,&c,1) != 1) { close(fd); return false; };
  close(fd);
  return true;
}

bool PingFIFO(const JobUser& user) {
  int fd = OpenFIFO(user);
  // If nothing is listening open() will fail
  // so there is no need to send anything.
  if(fd == -1) return false;
  close(fd);
  return true;
}

#else

CommFIFO::CommFIFO(void) {
}

CommFIFO::~CommFIFO(void) {
}

JobUser* CommFIFO::wait(int timeout) {
  return NULL;
}

bool CommFIFO::add(JobUser& user) {
  retrn add_error;
}

bool SignalFIFO(const JobUser& user) {
  return false;
}

bool PingFIFO(const JobUser& user) {
  return false;
}

#endif

