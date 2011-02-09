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

  /// Open a directory and return a pointer to a Dir object which can be iterated over
  Glib::Dir* DirOpen(const std::string& path);

  /// Open a directory using the specified uid and gid and return a pointer to a Dir object which can be iterated over
  Glib::Dir* DirOpen(const std::string& path,uid_t uid,gid_t gid);

  /// Stat a file and put info into the st struct
  bool FileStat(const std::string& path,struct stat *st,bool follow_symlinks);

  /// Stat a file using the specified uid and gid and put info into the st struct
  bool FileStat(const std::string& path,struct stat *st,uid_t uid,gid_t gid,bool follow_symlinks);

  /// Create a new directory
  bool DirCreate(const std::string& path,mode_t mode,bool with_parents = false);

  /// Create a new directory using the specified uid and gid
  bool DirCreate(const std::string& path,uid_t uid,gid_t gid,mode_t mode,bool with_parents = false);

  /// Delete a directory
  bool DirDelete(const std::string& path);

  /// Delete a directory using the specified uid and gid
  bool DirDelete(const std::string& path,uid_t uid,gid_t gid);

  /// Create a temporary directory under the system defined temp location, and return its path
  bool TmpDirCreate(std::string& path);

} // namespace Arc

