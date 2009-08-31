#include <string>

#include <arc/Thread.h>

class Janitor {
 private:
  std::string path_;
  std::string id_;
  std::string cdir_;
  bool running_;
  bool result_;
  Arc::SimpleCondition completed_;
  Arc::SimpleCondition cancel_;
  static void deploy_thread(void* arg);
  static void remove_thread(void* arg);
  void cancel(void);
 public:
  Janitor(const std::string& id,const std::string& cdir);
  ~Janitor(void);
  operator bool(void) { return !path_.empty(); };
  bool operator!(void) { return path_.empty(); };
  bool deploy(void);
  bool remove(void);
  bool wait(int timeout);
  bool result(void);
};

