#ifndef __ARC_FILEACCESS_H__
#define __ARC_FILEACCESS_H__

#include <string>

#include <sys/stat.h>
#include <sys/types.h>

namespace Arc {

  class Run;

  /// Defines interface for accessing filesystems
  /** This class access local filesystem through proxy executable
    which allows to switch user id in multithreaded systems without 
    introducing conflict with other threads. */
  class FileAccess {
  public:
    FileAccess(void);
    ~FileAccess(void);
    bool ping(void);
    bool setuid(int uid,int gid);
    bool mkdir(const std::string& path, mode_t mode);
    bool link(const std::string& oldpath, const std::string& newpath);
    bool softlink(const std::string& oldpath, const std::string& newpath);
    bool stat(const std::string& path, struct stat& st);
    bool lstat(const std::string& path, struct stat& st);
    bool remove(const std::string& path);
    bool unlink(const std::string& path);
    bool rmdir(const std::string& path);
    bool opendir(const std::string& path);
    bool closedir(void);
    bool readdir(std::string& name);
    bool open(const std::string& path, int flags, mode_t mode);
    bool close(void);
    off_t lseek(off_t offset, int whence);
    ssize_t read(void* buf,size_t size);
    ssize_t write(const void* buf,size_t size);
    ssize_t pread(void* buf,size_t size,off_t offset);
    ssize_t pwrite(const void* buf,size_t size,off_t offset);
    int errno() { return errno_; };
    operator bool(void) { return (file_access_ != NULL); };
    bool operator!(void) { return (file_access_ == NULL); };
  private:
    Run* file_access_;
    int errno_;
    typedef struct {
      unsigned int size;
      unsigned int cmd;
    } header_t;
  };

} // namespace Arc 

#endif // __ARC_FILEACCESS_H__

