#include <glibmm.h>

#include <arc/Thread.h>

namespace Arc {

  /// Utility functions for handling files and directories

  /// Open a file and return a file handle
  int FileOpen(const std::string& path,int flags,mode_t mode = 0600);

  /// Open a file using the specified uid and gid and return a file handle
  int FileOpen(const std::string& path,int flags,uid_t uid,gid_t gid,mode_t mode = 0600);

  /// Copy file source_path to file destination_path
  bool FileCopy(const std::string& source_path,const std::string& destination_path);

  /// Copy file source_path to file handle destination_handle
  bool FileCopy(const std::string& source_path,int destination_handle);

  /// Copy from file handle source_handle to file handle destination_path
  bool FileCopy(int source_handle,const std::string& destination_path);

  /// Copy from file handle source_handle to file handle destination_handle
  bool FileCopy(int source_handle,int destination_handle);

  /// Simple method to read file content from filename.
  /** The content is split into lines with the new line character removed,
   * and the lines are returned in the data list. For more complex file
   * handling or large files, FileOpen() should be used.
   * If uid/gid are zero then no real switch of uid/gid is done. */
  bool FileRead(const std::string& filename, std::list<std::string>& data, uid_t uid=0, gid_t gid=0);

  /// Simple method to create a new file containing given data.
  /** An existing file is overwritten with the new data. Permissions of the
   * created file are determined using the current umask. For more complex
   * file handling or large files, FileOpen() should be used.
   * If uid/gid are zero then no real switch of uid/gid is done. */
  bool FileCreate(const std::string& filename, const std::string& data, uid_t uid=0, gid_t gid=0);

  /// Open a directory and return a pointer to a Dir object which can be iterated over
  Glib::Dir* DirOpen(const std::string& path);

  /// Open a directory using the specified uid and gid and return a pointer to a Dir object which can be iterated over
  Glib::Dir* DirOpen(const std::string& path,uid_t uid,gid_t gid);

  /// Stat a file and put info into the st struct
  bool FileStat(const std::string& path,struct stat *st,bool follow_symlinks);

  /// Stat a file using the specified uid and gid and put info into the st struct
  bool FileStat(const std::string& path,struct stat *st,uid_t uid,gid_t gid,bool follow_symlinks);

  /// Make symbolic or hard link of file
  bool FileLink(const std::string& oldpath,const std::string& newpath, bool symbolic);

  /// Make symbolic or hard link of file using the specified uid and gid
  bool FileLink(const std::string& oldpath,const std::string& newpath,uid_t uid,gid_t gid,bool symbolic);

  /// Returns path at which symbolic link is pointing
  std::string FileReadLink(const std::string& path);

  /// Returns path at which symbolic link is pointing using the specified uid and gid
  std::string FileReadLink(const std::string& path,uid_t uid,gid_t gid);

  /// Returns path at which symbolic link is pointing
  bool FileDelete(const std::string& path);

  /// Returns path at which symbolic link is pointing using the specified uid and gid
  bool FileDelete(const std::string& path,uid_t uid,gid_t gid);

  /// Create a new directory
  bool DirCreate(const std::string& path,mode_t mode,bool with_parents = false);

  /// Create a new directory using the specified uid and gid
  bool DirCreate(const std::string& path,uid_t uid,gid_t gid,mode_t mode,bool with_parents = false);

  /// Delete a directory. This functions does not do any user locking. Use it with care.
  bool DirDelete(const std::string& path);

  /// Delete a directory using the specified uid and gid
  bool DirDelete(const std::string& path,uid_t uid,gid_t gid);

  /// Create a temporary directory under the system defined temp location, and return its path
  /** Uses mkdtemp if available, and a combination of random parameters if not. This
   * latter method is not as safe as mkdtemp. */
  bool TmpDirCreate(std::string& path);

} // namespace Arc

