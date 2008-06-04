#include <string>
#include <list>

#include <arc/XMLNode.h>

namespace ARex {

class GridManager {
 private:
  bool active_;
  GridManager(void) { };
  GridManager(const GridManager&) { };
 public:
  //GridManager(Arc::XMLNode argv);
  GridManager(const char* config_filename);
  ~GridManager(void);
  operator bool(void) { return active_; };
};

}

