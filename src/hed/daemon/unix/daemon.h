#ifndef __ARC_DAEMON_H__
#define __ARC_DAEMON_H__

#include <string>
#include <arc/Logger.h>

namespace Arc {

class Daemon {
  public:
    Daemon() {};
    Daemon(const std::string &pid_file_, const std::string &log_file_);
    ~Daemon();
  private:
    const std::string pid_file;
    static Logger logger;
};

} // namespace Arc

#endif // __ARC_DAEMON_H__
