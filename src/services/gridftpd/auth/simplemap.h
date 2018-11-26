#include <string>

#define SELFUNMAP_TIME (10*24*60*60)

class SimpleMap {
 private:
  std::string dir_;
  int pool_handle_;
  unsigned int selfunmap_time_;
 public:
  SimpleMap(const char* dir);
  ~SimpleMap(void);
  std::string map(const char* subject);
  bool unmap(const char* subject);
  operator bool(void) const { return (pool_handle_ != -1); };
  bool operator!(void) const { return (pool_handle_ == -1); };
};

