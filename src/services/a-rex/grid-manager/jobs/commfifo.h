#ifndef GM_COMMFIFO_H
#define GM_COMMFIFO_H

#include <list>

#include <arc/Thread.h>

namespace ARex {

class CommFIFO {
 private:
  class elem_t {
   public:
    elem_t(void):fd(-1),fd_keep(-1) { };
    int fd;
    int fd_keep;
  };
  std::list<elem_t> fds;
  int kick_in;
  int kick_out;
  Glib::Mutex lock;
  int timeout_;
  bool make_pipe(void);
 public:
  typedef enum {
    add_success,
    add_busy,
    add_error
  } add_result;
  CommFIFO(void);
  ~CommFIFO(void);
  add_result add(const std::string& dir_path);
  void wait(int timeout);
  void wait(void) { wait(timeout_); };
  void timeout(int t) { timeout_=t; };
};

bool SignalFIFO(const std::string& dir_path);
bool PingFIFO(const std::string& dir_path);

} // namespace ARex

#endif // GM_COMMFIFO_H
