#include <glibmm.h>

#include <arc/Thread.h>

namespace Arc {

  /// Utility functions for handling files and directories.
  /// Those functions offer posibility to access files and directories
  /// under user and group ids different from those of current user.
  /// Id switching is done in way safe for muti-threaded application.
  /// If any of specified ids is 0 then such id is not switched and
  /// current id is used instead.

  /// Copy file source_path to file destination_path.
  /// Specified uid and gid are used for accessing filesystem.
  bool FileCopy(const std::string& source_path,const std::string& destination_path, uid_t uid, gid_t gid);

  /// Copy file source_path to file destination_path.
  bool FileCopy(const std::string& source_path,const std::string& destination_path);

  /// Copy file source_path to file handle destination_handle.
  bool FileCopy(const std::string& source_path,int destination_handle);

  /// Copy from file handle source_handle to file destination_path.
  bool FileCopy(int source_handle,const std::string& destination_path);

  /// Copy from file handle source_handle to file handle destination_handle.
  bool FileCopy(int source_handle,int destination_handle);

  /// Simple method to read file content from filename.
  /// Specified uid and gid are used for accessing filesystem.
  /** The content is split into lines with the new line character removed,
   * and the lines are returned in the data list.
   * If protected access is required, FileLock should be used in addition
   * to FileRead. */
  bool FileRead(const std::string& filename, std::list<std::string>& data, uid_t uid=0, gid_t gid=0);

  /// Simple method to create a new file containing given data.
  /// Specified uid and gid are used for accessing filesystem.
  /** An existing file is overwritten with the new data. Permissions of the
   * created file are determined using the current umask. For more complex
   * file handling or large files, FileOpen() should be used. If protected
   * access is required, FileLock should be used in addition to FileRead.
   * If uid/gid are zero then no real switch of uid/gid is done. */
  bool FileCreate(const std::string& filename, const std::string& data, uid_t uid=0, gid_t gid=0);

  /// Stat a file and put info into the st struct
  bool FileStat(const std::string& path,struct stat *st,bool follow_symlinks);

  /// Stat a file using the specified uid and gid and put info into the st struct
  /// Specified uid and gid are used for accessing filesystem.
  bool FileStat(const std::string& path,struct stat *st,uid_t uid,gid_t gid,bool follow_symlinks);

  /// Make symbolic or hard link of file
  bool FileLink(const std::string& oldpath,const std::string& newpath, bool symbolic);

  /// Make symbolic or hard link of file using the specified uid and gid
  /// Specified uid and gid are used for accessing filesystem.
  bool FileLink(const std::string& oldpath,const std::string& newpath,uid_t uid,gid_t gid,bool symbolic);

  /// Returns path at which symbolic link is pointing
  std::string FileReadLink(const std::string& path);

  /// Returns path at which symbolic link is pointing using the specified uid and gid
  /// Specified uid and gid are used for accessing filesystem.
  std::string FileReadLink(const std::string& path,uid_t uid,gid_t gid);

  /// Deletes file at path
  bool FileDelete(const std::string& path);

  /// Deletes file at path using the specified uid and gid
  /// Specified uid and gid are used for accessing filesystem.
  bool FileDelete(const std::string& path,uid_t uid,gid_t gid);

  /// Create a new directory
  bool DirCreate(const std::string& path,mode_t mode,bool with_parents = false);

  /// Create a new directory using the specified uid and gid
  /// Specified uid and gid are used for accessing filesystem.
  bool DirCreate(const std::string& path,uid_t uid,gid_t gid,mode_t mode,bool with_parents = false);

  /// Delete a directory and its content.
  bool DirDelete(const std::string& path);

  /// Delete a directory using the specified uid and gid
  /// Specified uid and gid are used for accessing filesystem.
  bool DirDelete(const std::string& path,uid_t uid,gid_t gid);

  /// Create a temporary directory under the system defined temp location, and return its path
  /** Uses mkdtemp if available, and a combination of random parameters if not. This
   * latter method is not as safe as mkdtemp. */
  bool TmpDirCreate(std::string& path);

  /// Simple method to create a temporary file containing given data.
  /// Specified uid and gid are used for accessing filesystem.
  /** Permissions of the created file are determined using the current umask.
   * If uid/gid are zero then no real switch of uid/gid is done. */
  bool TmpFileCreate(std::string& filename, const std::string& data, uid_t uid=0, gid_t gid=0);

} // namespace Arc

