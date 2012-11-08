#ifndef GRID_MANAGER_H
#define GRID_MANAGER_H

#include <string>
#include <list>

#include <arc/XMLNode.h>
#include <arc/Thread.h>

namespace ARex {

class DTRGenerator;
class CommFIFO;
class GMConfig;
class sleep_st;

class GridManager {
 private:
  Arc::SimpleCounter active_;
  bool tostop_;
  Arc::SimpleCondition* sleep_cond_;
  CommFIFO* wakeup_interface_;
  GMConfig& config_;
  sleep_st* wakeup_;
  DTRGenerator* dtr_generator_;
  GridManager();
  GridManager(const GridManager&);
  static void grid_manager(void* arg);
  bool thread(void);
 public:
  GridManager(GMConfig& config);
  ~GridManager(void);
  operator bool(void) { return (active_.get()>0); };
};

} // namespace ARex

#endif // GRID_MANAGER_H
