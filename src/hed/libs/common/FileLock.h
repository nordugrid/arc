// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_FILELOCK_H__
#define __ARC_FILELOCK_H__

#include <string>

namespace Arc {

  /// A general file locking class
  class FileLock {
  public:
    /// Create a new FileLock. Blocks until the lock is obtained
    FileLock(const std::string& filename);
    /// Remove the lock
    ~FileLock();
    /// true if the lock is held and valid
    operator bool();
    /// true if the lock is not valid
    bool operator!();
  private:
    const std::string lockfile;
    int fd;
  };

} // namespace Arc

#endif
