#ifndef __ARC_DAEMON_H__
#define __ARC_DAEMON_H__

#include <string>
#include <arc/Logger.h>

namespace Arc {

class Daemon {
  public:
    Daemon() {};
    Daemon(const std::string &pid_file_, const std::string &log_file_, bool watchdog);
    ~Daemon();
    void logreopen(void);
  private:
    const std::string pid_file;
    const std::string log_file;
    static Logger logger;
};

class Watchdogs {
  private:
    class Channel {
      public:
        int timeout;
        time_t next;
    };
    std::vector<int> timeouts_; 
    Watchdogs(const Watchdogs&);
  public:
    Watchdogs() {};
    ~Watchdogs() {};
    /// Open watchdog channel with specified timeout in ms
    int Open(int timeout);
    /// Register one more handler of open channel.
    /// This increases by one how many times Close() method need to be called.
    void Dup(int channel);
    /// Send "I'm alive" signal to watchdog
    void Kick(int channel);
    /// Close watchdog channel 
    void Close(int channel);
};

class Watchdog {
  private:
    int id_;
  public:
    Watchdog();
    Watchdog(const Watchdog& handle);
    ~Watchdog();
    void Kick(void);
};

} // namespace Arc

#endif // __ARC_DAEMON_H__
