#include <string>
#include <list>

#include <arc/XMLNode.h>

class GMEnvironment;

namespace ARex {

class GridManager {
 private:
  bool active_;
  GridManager(void) { };
  GridManager(const GridManager&) { };
 public:
  //GridManager(Arc::XMLNode argv);
  GridManager(GMEnvironment& env);
  ~GridManager(void);
  operator bool(void) { return active_; };
};

}

