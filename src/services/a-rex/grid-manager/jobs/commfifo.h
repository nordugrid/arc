#include <list>

#include <arc/Thread.h>
#include "users.h"

class CommFIFOListener;

class CommFIFO {
 private:
  class elem_t {
   public:
    elem_t(void):user(NULL),fd(-1),fd_keep(-1) { };
    JobUser* user;
    int fd;
    int fd_keep;
  };
  std::list<elem_t> fds;
  int kick_in;
  int kick_out;
  Glib::Mutex lock;
  int timeout_;
 public:
  CommFIFO(void);
  ~CommFIFO(void);
  bool add(JobUser& user);
  JobUser* wait(int timeout);
  JobUser* wait(void) { return wait(timeout_); };
  void timeout(int t) { timeout_=t; };
};

bool SignalFIFO(const JobUser& user);
