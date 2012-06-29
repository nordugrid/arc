#ifndef __ARC_FILEACCESS_H__
#define __ARC_FILEACCESS_H__

#include <string>
#include <list>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <glibmm.h>

#ifdef WIN32
#ifndef uid_t
#define uid_t int
#endif
#ifndef gid_t
#define gid_t int
#endif
#endif

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
    /// Constructor which takes already existing object from global cache
    static FileAccess* Acquire(void);
    /// Destructor which returns object into global cache
    static void Release(FileAccess* fa);
    /// Check if communication with proxy works
    bool ping(void);
    /// Modify user uid and gid.
    /// If any is set to 0 then executable is switched to original uid/gid.
    bool fa_setuid(int uid,int gid);
    /// Make a directory and assign it specified mode.
    bool fa_mkdir(const std::string& path, mode_t mode);
    /// Make a directory and assign it specified mode.
    /// If missing all intermediate directories are created too.
    bool fa_mkdirp(const std::string& path, mode_t mode);
    /// Create hard link.
    bool fa_link(const std::string& oldpath, const std::string& newpath);
    /// Create symbolic (aka soft) link.
    bool fa_softlink(const std::string& oldpath, const std::string& newpath);
    /// Copy file to new location.
    /// If new file is created it is assigned secified mode.
    bool fa_copy(const std::string& oldpath, const std::string& newpath, mode_t mode);
    /// Rename file
    bool fa_rename(const std::string& oldpath, const std::string& newpath);
    /// Change mode of filesystem object
    bool fa_chmod(const std::string& path,mode_t mode);
    /// stat file.
    bool fa_stat(const std::string& path, struct stat& st);
    /// stat symbolic link or file.
    bool fa_lstat(const std::string& path, struct stat& st);
    /// stat open file.
    bool fa_fstat(struct stat& st);
    /// Truncate open file.
    bool fa_ftruncate(off_t length);
    /// Allocate disk space for open file.
    off_t fa_fallocate(off_t length);
    /// Read content of symbolic link.
    bool fa_readlink(const std::string& path, std::string& linkpath);
    /// Remove file system object.
    bool fa_remove(const std::string& path);
    /// Remove file.
    bool fa_unlink(const std::string& path);
    /// Remove directory (if empty).
    bool fa_rmdir(const std::string& path);
    /// Remove directory recursively.
    bool fa_rmdirr(const std::string& path);
    /// Open directory.
    /// Only one directory may be open at a time.
    bool fa_opendir(const std::string& path);
    /// Close open directory.
    bool fa_closedir(void);
    /// Read relative name of object in open directory.
    bool fa_readdir(std::string& name);
    /// Open file.
    /// Only one file may be open at a time.
    bool fa_open(const std::string& path, int flags, mode_t mode);
    /// Close open file.
    bool fa_close(void);
    /// Open new temporary file for writing.
    /// On input path contains template of file name ending with XXXXXX.
    /// On output path is path to created file.
    bool fa_mkstemp(std::string& path, mode_t mode);
    /// Change current position in open file.
    off_t fa_lseek(off_t offset, int whence);
    /// Read from open file.
    ssize_t fa_read(void* buf,size_t size);
    /// Write to open file.
    ssize_t fa_write(const void* buf,size_t size);
    /// Read from open file at specified offset.
    ssize_t fa_pread(void* buf,size_t size,off_t offset);
    /// Write to open file at specified offset.
    ssize_t fa_pwrite(const void* buf,size_t size,off_t offset);
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

  /// Container for shared FileAccess objects
  class FileAccessContainer {
  public:
    /// Creates container with number of stored objects between minval and maxval.
    FileAccessContainer(unsigned int minval, unsigned int maxval);
    FileAccessContainer(void);
    /// Destroys container and all stored object
    ~FileAccessContainer(void);
    /// Get object from container.
    /// Object either is taken from stored ones or new one created.
    /// Acquired object looses its connection to container and 
    /// can be safely destroyed or returned into other container.
    FileAccess* Acquire(void);
    /// Returns object into container.
    /// It canbe any object - taken from another cotainer or created
    /// using new.
    void Release(FileAccess* fa);
    /// Adjust minimal number of stored objects.
    void SetMin(unsigned int val);
    /// Adjust maximal number of stored objects.
    void SetMax(unsigned int val);
  private:
    Glib::Mutex lock_;
    unsigned int min_;
    unsigned int max_;
    std::list<FileAccess*> fas_;
    void KeepRange(void);
  };

} // namespace Arc 

#endif // __ARC_FILEACCESS_H__

