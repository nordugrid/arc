#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "CommFIFO.h"

namespace ARex {

static const unsigned int MAX_ID_SIZE = 64;

static const std::string fifo_file("/gm.fifo");

bool CommFIFO::make_pipe(void) {
  bool res = false;
  lock.lock();
  if (kick_in != -1) {
    close(kick_in); kick_in = -1;
  };
  if (kick_out != -1) {
    close(kick_out); kick_out = -1;
  };
  int filedes[2];
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

bool CommFIFO::wait(int timeout, std::string& event) {
  time_t start_time = time(NULL);
  time_t end_time = start_time + timeout;
  bool have_generic_event = false;
  bool kicked = false;
  event.clear();
  for(;;) {
    // Check if there is something in buffers
    lock.lock();
    for(std::list<elem_t>::iterator i = fds.begin();i!=fds.end();++i) {
      if(!(i->ids.empty())) {
        event = *(i->ids.begin());
        i->ids.pop_front();
        lock.unlock();
        return true;
      };
    };
    lock.unlock();
    if(have_generic_event) return true;
    if(kicked) return false;
    // If nothing found - wait for incoming information
    fd_set fin,fout,fexc;
    FD_ZERO(&fin); FD_ZERO(&fout); FD_ZERO(&fexc);
    int maxfd=-1;
    if(kick_out == -1) make_pipe(); // try to recover if had error previously
    if(kick_out != -1) { maxfd=kick_out; FD_SET(kick_out,&fin); };
    lock.lock();
    for(std::list<elem_t>::iterator i = fds.begin();i!=fds.end();++i) {
      if(i->fd < 0) {
        // try to recover lost pipe
        std::string pipe_dir = i->path;
        take_pipe(pipe_dir, *i);
        if(i->fd < 0) continue;
      };
      if(i->fd > maxfd) maxfd=i->fd;
      FD_SET(i->fd,&fin);
    };
    lock.unlock();
    int err;
    maxfd++;
    if(timeout >= 0) {
      struct timeval t;
      if(((int)(end_time-start_time)) < 0) return false; // timeout
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
    if(err == 0) return false; // timeout
    if(err == -1) {
      if(errno == EBADF) {
        // One of fifos must be broken. Let read() find that out.
      } else if(errno == EINTR) {
        // interrupted by signal, retry
        continue;
      };
      // No idea how this could happen and how to deal with it.
      // Lets try to escape and start from beginning
      return false;
    };
    lock.lock();
    for(std::list<elem_t>::iterator i = fds.begin();i!=fds.end();++i) {
      if(i->fd < 0) continue;
      if((err < 0) || FD_ISSET(i->fd,&fin)) {
        for(;;) {
          char buf[16];
          ssize_t l = read(i->fd,buf,sizeof(buf));
          if(l == 0) {
            break; // eol
          } else if(l < 0) {
            if((errno == EBADF) || (errno == EINVAL) || (errno == EIO)) {
              close(i->fd); close(i->fd_keep);
              i->fd = -1; i->fd_keep = -1;
            };
            break;
          } else if(l > 0) {
            // it must be zero-terminated string representing job id
            for(ssize_t n = 0; n<l; ++n) {
              if(buf[n] == '\0') {
                if(i->buffer.empty()) {
                  have_generic_event = true;
                } else {
                  i->ids.push_back(i->buffer);
                  i->buffer.clear();
                };
              } else {
                // Some sanity check
                if(i->buffer.length() < MAX_ID_SIZE) i->buffer.append(1,buf[n]);
              };
            };
          };
        }; // for(;;)
      };
    }; // for(fds)
    lock.unlock();

    if(kick_out >= 0) {
      if((err < 0) || FD_ISSET(kick_out,&fin)) {
        for(;;) { // read as much as arrived
          char buf[16];
          ssize_t l = read(kick_out,buf,sizeof(buf));
          if(l == -1) {
            if((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
              break; // nothing to read more
            };
            // Recover after error
            make_pipe();
          } else if(l == 0) {
            break; // nothing to read more
          } else if(l > 0) {
            kicked = true;
            break;
          };
        };
      };
    };

  };
  return false;
}

CommFIFO::add_result CommFIFO::take_pipe(const std::string& dir_path, elem_t& el) {
  std::string path = dir_path + fifo_file;
  if(mkfifo(path.c_str(),S_IRUSR | S_IWUSR) != 0) {
    if(errno != EEXIST) {
      return add_error; 
    };
  };
  (void)chmod(path.c_str(),S_IRUSR | S_IWUSR);
  int fd = -1;
  // This must fail. If not then there is another a-rex hanging around.
  fd = open(path.c_str(),O_WRONLY | O_NONBLOCK);
  if(fd != -1) { close(fd); return add_busy; };
  // (errno != ENXIO)) {
  fd = open(path.c_str(),O_RDONLY | O_NONBLOCK);
  if(fd == -1) return add_error;
  int fd_keep = open(path.c_str(),O_WRONLY | O_NONBLOCK);
  if(fd_keep == -1) { close(fd); return add_error; };
  el.fd=fd; el.fd_keep=fd_keep; el.path=dir_path;
  return add_success;
}

void CommFIFO::kick(void) {
  if(kick_in >= 0) {
    char c = '\0';
    write(kick_in,&c,1);
  };
}

CommFIFO::add_result CommFIFO::add(const std::string& dir_path) {
  elem_t el;
  CommFIFO::add_result result = take_pipe(dir_path, el);
  if(result == add_success) {
    lock.lock();
    fds.push_back(el);
    if(kick_in != -1) {
      char c = '\0';
      write(kick_in,&c,1);
    };
    lock.unlock();
  };
  return result;
}

static int OpenFIFO(const std::string& path) {
  // Here O_NONBLOCK ensures open() will fail if nothing listens
  int fd = open(path.c_str(),O_WRONLY | O_NONBLOCK);
  // If fd == -1 here there is no FIFO or nothing is listening on another end
  return fd;
}

bool CommFIFO::Signal(const std::string& dir_path, const std::string& id) {
  std::string path = dir_path + fifo_file;
  int fd = OpenFIFO(path);
  if(fd == -1) return false;
  for(std::string::size_type pos = 0; pos <= id.length(); ++pos) {
    ssize_t l = write(fd, id.c_str()+pos, id.length()+1-pos);
    if(l == -1) {
      if((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
        sleep(1); // todo: select/poll
        continue; // retry
      };
      close(fd); return false;
    };
    pos += l;
  };
  close(fd);
  return true;
}

bool CommFIFO::Ping(const std::string& dir_path) {
  std::string path = dir_path + fifo_file;
  int fd = OpenFIFO(path);
  // If nothing is listening open() will fail
  // so there is no need to send anything.
  if(fd == -1) return false;
  close(fd);
  return true;
}

} // namespace ARex
