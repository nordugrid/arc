#ifndef __ARC_DAEMON_H__
#define __ARC_DAEMON_H__

#include <string>

#include "common/Logger.h"

namespace Arc {

class Daemon {
    public:
        Daemon() {};
        Daemon(std::string& pid_file, std::string& log_file);
        ~Daemon();
    private:
        std::string pid_file;
	static Logger logger;
};

}; // namespace Arc

#endif // __ARC_DAEMON_H__
