#ifndef GRID_MANAGER_H
#define GRID_MANAGER_H

#include <arc/Thread.h>

namespace ARex {

class JobsList;
class DTRGenerator;
class GMConfig;

class GridManager {
 private:
  Arc::SimpleCounter active_;
  bool tostop_;
  GMConfig& config_;
  JobsList* jobs_;
  GridManager();
  GridManager(const GridManager&);
  static void grid_manager(void* arg);
  bool thread(void);
 public:
  GridManager(GMConfig& config);
  ~GridManager(void);
  operator bool(void) { return (active_.get()>0); };
  void RequestJobAttention(const std::string& job_id);
};

} // namespace ARex

#endif // GRID_MANAGER_H
