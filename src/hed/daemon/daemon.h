#ifndef __ARC_DAEMON_H__
#define __ARC_DAEMON_H__

#include <string>

namespace Arc {

class Daemon {
    public:
        Daemon() {};
        Daemon(std::string& pid_file);
        ~Daemon();
    private:
        std::string pid_file;
};

}; // namespace Arc

#endif // __ARC_DAEMON_H__
