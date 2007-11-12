#ifndef __GM_DAEMON_H__
#define __GM_DAEMON_H__

#include <string>
#include <arc/Logger.h>

#define DAEMON_OPTS "ZzFL:U:P:d:"

class Daemon {
 private:
  std::string logfile_;
  int logsize_;
  int lognum_;
  uid_t uid_;
  gid_t gid_;
  bool daemon_;
  std::string pidfile_;
  int debug_;
  Arc::Logger& logger_;
 public:
  Daemon(void);
  ~Daemon(void);
  int arg(char c);
  int config(const std::string& cmd,std::string& rest);
  static int skip_config(const std::string& cmd);
  int getopt(int argc, char * const argv[],const char *optstring);
  int daemon(bool close_fds = false);
  const char* short_help(void);
  void logfile(const char* path);
  void pidfile(const char* path);
};

#endif // __GM_DAEMON_H__
