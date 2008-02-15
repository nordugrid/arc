#include <string>
#include <list>

#include <arc/XMLNode.h>

namespace ARex {

class GridManager {
 private:
  bool active_;
 public:
  GridManager(Arc::XMLNode argv);
  ~GridManager(void);
  operator bool(void) { return active_; };
};

}

