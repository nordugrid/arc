#include <string>

#define SELFUNMAP_TIME (10*24*60*60)

namespace ArcSec {

class SimpleMap {
 private:
  std::string dir_;
  int pool_handle_;
 public:
  SimpleMap(const std::string& dir);
  ~SimpleMap(void);
  std::string map(const std::string& subject);
  bool unmap(const std::string& subject);
  operator bool(void) { return (pool_handle_ != -1); };
  bool operator!(void) { return (pool_handle_ == -1); };
};

}

