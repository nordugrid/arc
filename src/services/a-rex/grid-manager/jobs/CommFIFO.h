#ifndef GM_COMMFIFO_H
#define GM_COMMFIFO_H

#include <list>

#include <arc/Thread.h>

namespace ARex {

class CommFIFO {
 public:
  typedef enum {
    add_success,
    add_busy,
    add_error
  } add_result;

 private:
  class elem_t {
   public:
    elem_t(void):fd(-1),fd_keep(-1) { };
    int fd;
    int fd_keep;
    std::string path;
    std::list<std::string> ids;
    std::string buffer;
  };
  // Open external pipes
  std::list<elem_t> fds;
  // Internal pipe used to report about addition
  // of new external pipes
  int kick_in;
  int kick_out;
  // Multi-threading protection 
  Glib::RecMutex lock;
  int timeout_;
  // Create internal pipe
  bool make_pipe(void);
  // Open external pipe
  add_result take_pipe(const std::string& dir_path, elem_t& el); 

 public:
  CommFIFO(void);
  ~CommFIFO(void);

  /// Add new external signal source
  add_result add(const std::string& dir_path);

  /// Remove external signal source
  bool remove(const std::string& dir_path);

  /// Wait for any event with specified timeout
  bool wait(int timeout) { std::string event; return wait(timeout, event); };
 
  /// Wait for any event with specified timeout and collect event id
  bool wait(int timeout, std::string& event);
 
  /// Wait for any event with default timeout
  bool wait(void) { std::string event; return wait(timeout_, event); };
 
  /// Wait for any event with default timeout and collect event id
  bool wait(std::string& event) { return wait(timeout_, event); };

  /// Kick waiting wait() causing it to exit as if timeout accured
  void kick();

  /// Set default timeout (negative for infinite)
  void timeout(int t) { timeout_=t; };

  /// Signal to A-REX job id which changed. Or generic kick if id is not set.
  static bool Signal(const std::string& dir_path, const std::string& id = "");
  static bool Signal(const std::string& dir_path, const std::vector<std::string>& ids);

  /// Check if A-REX is listening to signals (without sending any signal).
  static bool Ping(const std::string& dir_path);
};


} // namespace ARex

#endif // GM_COMMFIFO_H
