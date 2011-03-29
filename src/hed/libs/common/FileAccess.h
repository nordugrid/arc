#ifndef __ARC_FILEACCESS_H__
#define __ARC_FILEACCESS_H__

#include <string>

#include <sys/stat.h>
#include <sys/types.h>

#include <glibmm.h>

namespace Arc {

  class Run;

  /// Defines interface for accessing filesystems
  /** This class accesses local filesystem through proxy executable
    which allows to switch user id in multithreaded systems without 
    introducing conflict with other threads. Its methods are mostly
    replicas of corresponding POSIX functions with some convenience
    tweaking. */
  class FileAccess {
  public:
    FileAccess(void);
    ~FileAccess(void);
    /// Check if communication with proxy works
    bool ping(void);
    /// Modify user uid and gid.
    /// If any is set to 0 then executable is switched to original uid/gid.
    bool setuid(int uid,int gid);
    /// Make a directory and assign it specified mode.
    bool mkdir(const std::string& path, mode_t mode);
    /// Make a directory and assign it specified mode.
    /// If missing all intermediate directories are created too.
    bool mkdirp(const std::string& path, mode_t mode);
    /// Create hard link.
    bool link(const std::string& oldpath, const std::string& newpath);
    /// Create symbolic (aka soft) link.
    bool softlink(const std::string& oldpath, const std::string& newpath);
    /// Copy file to new location.
    /// If new file is created it is assigned secified mode.
    bool copy(const std::string& oldpath, const std::string& newpath, mode_t mode);
    /// stat file.
    bool stat(const std::string& path, struct stat& st);
    /// stat symbolic link or file.
    bool lstat(const std::string& path, struct stat& st);
    /// stat open file.
    bool fstat(struct stat& st);
    /// Truncate open file.
    bool ftruncate(off_t length);
    /// Allocate disk space for open file.
    off_t fallocate(off_t length);
    /// Read content of symbolic link.
    bool readlink(const std::string& path, std::string& linkpath);
    /// Remove file system object.
    bool remove(const std::string& path);
    /// Remove file.
    bool unlink(const std::string& path);
    /// Remove directory (if empty).
    bool rmdir(const std::string& path);
    /// Remove directory recursively.
    bool rmdirr(const std::string& path);
    /// Open directory.
    /// Only one directory may be open at a time.
    bool opendir(const std::string& path);
    /// Close open directory.
    bool closedir(void);
    /// Read relative name of object in open directory.
    bool readdir(std::string& name);
    /// Open file.
    /// Only one file may be open at a time.
    bool open(const std::string& path, int flags, mode_t mode);
    /// Close open file.
    bool close(void);
    /// Change current position in open file.
    off_t lseek(off_t offset, int whence);
    /// Read from open file.
    ssize_t read(void* buf,size_t size);
    /// Write to open file.
    ssize_t write(const void* buf,size_t size);
    /// Read from open file at specified offset.
    ssize_t pread(void* buf,size_t size,off_t offset);
    /// Write to open file at specified offset.
    ssize_t pwrite(const void* buf,size_t size,off_t offset);
    /// Get errno of last operation.
    /// Every operation resets errno.
    int geterrno() { return errno_; };
    /// Returns true if this instance is in useful condition
    operator bool(void) { return (file_access_ != NULL); };
    /// Returns true if this instance is not in useful condition
    bool operator!(void) { return (file_access_ == NULL); };
    /// Special method for using in unit tests.
    static void testtune(void);
  private:
    Glib::Mutex lock_;
    Run* file_access_;
    int errno_;
    uid_t uid_;
    gid_t gid_;
  public:
    typedef struct {
      unsigned int size;
      unsigned int cmd;
    } header_t;
  };

} // namespace Arc 

#endif // __ARC_FILEACCESS_H__

