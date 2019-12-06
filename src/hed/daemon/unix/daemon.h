#ifndef __ARC_DAEMON_H__
#define __ARC_DAEMON_H__

#include <string>
#include <arc/Logger.h>

namespace Arc {

class Daemon {
  public:
    Daemon():watchdog_pid(0),watchdog_cb(NULL) {};
    /**
     Daemonize application and optionally start watchdog.
     \param pid_file_ path to file to store PID of main process
     \param log_file_ path to file to direct stdout/stderr of main process
     \param watchdog if watchdog must be initialized
     \param watchdog_callback callback to be called when watchdog detects main application failure
    */
    Daemon(const std::string &pid_file_, const std::string &log_file_, bool watchdog = false, void (*watchdog_callback)(Daemon*) = NULL);
    ~Daemon();
    void logreopen(void);
    void shutdown(void);
  private:
    const std::string pid_file;
    const std::string log_file;
    static Logger logger;
    unsigned int watchdog_pid;
    void (*watchdog_cb)(Daemon*);
};

} // namespace Arc

#endif // __ARC_DAEMON_H__
