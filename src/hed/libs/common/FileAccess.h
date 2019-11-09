#ifndef __ARC_FILEACCESS_H__
#define __ARC_FILEACCESS_H__

#include <string>
#include <list>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <glibmm.h>


namespace Arc {

  class Run;

  /// Defines interface for accessing filesystems.
  /** This class accesses the local filesystem through a proxy executable
    which allows switching user id in multithreaded systems without
    introducing conflict with other threads. Its methods are mostly
    replicas of corresponding POSIX functions with some convenience
    tweaking.
    \ingroup common
    \headerfile FileAccess.h arc/FileAccess.h
  */
  class FileAccess {
  public:
    /// New FileAccess object.
    FileAccess(void);
    /// Shuts down any spawned executable.
    ~FileAccess(void);
    /// Constructor which takes already existing object from global cache
    static FileAccess* Acquire(void);
    /// Destructor which returns object into global cache
    static void Release(FileAccess* fa);
    /// Check if communication with proxy works
    bool ping(void);
    /// Modify user uid and gid.
    /**
     * If any is set to 0 then executable is switched to original uid/gid.
     * 
     * \since Renamed in 3.0.0 from setuid
     */
    bool fa_setuid(int uid,int gid);
    /// Make a directory and assign it specified mode.
    /** 
     * \since Renamed in 3.0.0 from mkdir
     */
    bool fa_mkdir(const std::string& path, mode_t mode);
    /// Make a directory and assign it specified mode.
    /**
     * If missing all intermediate directories are created too.
     * 
     * \since Renamed in 3.0.0 from mkdirp
     */
    bool fa_mkdirp(const std::string& path, mode_t mode);
    /// Create hard link.
    /** 
     * \since Renamed in 3.0.0 from link
     */
    bool fa_link(const std::string& oldpath, const std::string& newpath);
    /// Create symbolic (aka soft) link.
    /** 
     * \since Renamed in 3.0.0 form softlink
     */
    bool fa_softlink(const std::string& oldpath, const std::string& newpath);
    /// Copy file to new location.
    /**
     * If new file is created it is assigned specified mode.
     * 
     * \since Renamed in 3.0.0 from copy
     */
    bool fa_copy(const std::string& oldpath, const std::string& newpath, mode_t mode);
    /// Rename file
    /** 
     * \since Renamed in 3.0.0 from rename
     */
    bool fa_rename(const std::string& oldpath, const std::string& newpath);
    /// Change mode of filesystem object
    /** 
     * \since Renamed in 3.0.0 from chmod
     */
    bool fa_chmod(const std::string& path,mode_t mode);
    /// Stat file.
    /** 
     * \since Renamed in 3.0.0 from stat
     */
    bool fa_stat(const std::string& path, struct stat& st);
    /// Stat symbolic link or file.
    /** 
     * \since Renamed in 3.0.0 from lstat
     */
    bool fa_lstat(const std::string& path, struct stat& st);
    /// Stat open file.
    /** 
     * \since Renamed in 3.0.0 from fstat
     */
    bool fa_fstat(struct stat& st);
    /// Truncate open file.
    /** 
     * \since Renamed in 3.0.0 from ftruncate
     */
    bool fa_ftruncate(off_t length);
    /// Allocate disk space for open file.
    /** 
     * \since Renamed in 3.0.0 from fallocate
     */
    off_t fa_fallocate(off_t length);
    /// Read content of symbolic link.
    /** 
     * \since Renamed in 3.0.0 from readlink
     */
    bool fa_readlink(const std::string& path, std::string& linkpath);
    /// Remove file system object.
    /** 
     * \since Renamed in 3.0.0 from remove
     */
    bool fa_remove(const std::string& path);
    /// Remove file.
    /** 
     * \since Renamed in 3.0.0 from unlink
     */
    bool fa_unlink(const std::string& path);
    /// Remove directory (if empty).
    /** 
     * \since Renamed in 3.0.0 from rmdir
     */
    bool fa_rmdir(const std::string& path);
    /// Remove directory recursively.
    /** 
     * \since Renamed in 3.0.0 from rmdirr
     */
    bool fa_rmdirr(const std::string& path);
    /// Open directory.
    /**
     * Only one directory may be open at a time.
     * 
     * \since Renamed in 3.0.0 from opendir
     */
    bool fa_opendir(const std::string& path);
    /// Close open directory.
    /** 
     * \since Renamed in 3.0.0 from closedir
     */
    bool fa_closedir(void);
    /// Read relative name of object in open directory.
    /** 
     * \since Renamed in 3.0.0 from readdir
     */
    bool fa_readdir(std::string& name);
    /// Open file. Only one file may be open at a time.
    /** 
     * \since Renamed in 3.0.0 from open
     */
    bool fa_open(const std::string& path, int flags, mode_t mode);
    /// Close open file.
    /** 
     * \since Renamed in 3.0.0 from close
     */
    bool fa_close(void);
    /// Open new temporary file for writing.
    /**
     * On input path contains template of file name ending with XXXXXX.
     * On output path is path to created file.
     * 
     * \since Renamed in 3.0.0 from mkstemp
     */
    bool fa_mkstemp(std::string& path, mode_t mode);
    /// Change current position in open file.
    /** 
     * \since Renamed in 3.0.0 from lseek
     */
    off_t fa_lseek(off_t offset, int whence);
    /// Read from open file.
    /** 
     * \since Renamed in 3.0.0 from read
     */
    ssize_t fa_read(void* buf,size_t size);
    /// Write to open file.
    /** 
     * \since Renamed in 3.0.0 from write
     */
    ssize_t fa_write(const void* buf,size_t size);
    /// Read from open file at specified offset.
    /** 
     * \since Renamed in 3.0.0 from pread
     */
    ssize_t fa_pread(void* buf,size_t size,off_t offset);
    /// Write to open file at specified offset.
    /** 
     * \since Renamed in 3.0.0 from pwrite
     */
    ssize_t fa_pwrite(const void* buf,size_t size,off_t offset);
    /// Get errno of last operation. Every operation resets errno.
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
    /// Internal struct used for communication between processes.
    typedef struct {
      unsigned int size;
      unsigned int cmd;
    } header_t;
  };

  /// Container for shared FileAccess objects.
  /** FileAccessContainer maintains a pool of executables and can be used to
      reduce the overhead in creating and destroying executables when using
      FileAccess.
      \ingroup common
      \headerfile FileAccess.h arc/FileAccess.h */
  class FileAccessContainer {
  public:
    /// Creates container with number of stored objects between minval and maxval.
    FileAccessContainer(unsigned int minval, unsigned int maxval);
    /// Creates container with number of stored objects between 1 and 10.
    FileAccessContainer(void);
    /// Destroys container and all stored objects.
    ~FileAccessContainer(void);
    /// Get object from container.
    /** Object either is taken from stored ones or new one created.
        Acquired object looses its connection to container and
        can be safely destroyed or returned into other container. */
    FileAccess* Acquire(void);
    /// Returns object into container.
    /** It can be any object - taken from another container or created using
        new. */
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

