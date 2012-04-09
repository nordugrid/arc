#include <string>
#include <list>

#include <arc/XMLNode.h>
#include <arc/Thread.h>

class GMEnvironment;
class JobUsers;
class JobUser;
class DTRGenerator;

namespace ARex {

class GridManager {
 private:
  bool active_;
  bool tostop_;
  Arc::SimpleCondition* sleep_cond_;
  GMEnvironment* env_;
  JobUser* my_user_;
  bool my_user_owned_;
  JobUsers* users_;
  bool users_owned_;
  DTRGenerator* dtr_generator_;
  GridManager(void) { };
  GridManager(const GridManager&) { };
  static void grid_manager(void* arg);
  bool thread(void);
 public:
  GridManager(GMEnvironment& env);
  GridManager(JobUsers& users, JobUser& my_user);
  ~GridManager(void);
  operator bool(void) { return active_; };
  JobUsers* Users(void) { return users_; };
};

}

