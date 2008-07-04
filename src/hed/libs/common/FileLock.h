#ifndef __ARC_FILELOCK_H__
#define __ARC_FILELOCK_H__

#include <string>

namespace Arc {

  class FileLock {
  public:
    FileLock(const std::string& filename);
    ~FileLock();
    operator bool();
    bool operator!();
  private:
    const std::string lockfile;
    int fd;
  };

} // namespace Arc

#endif
