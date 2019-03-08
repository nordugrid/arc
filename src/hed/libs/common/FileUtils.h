#ifndef __ARC_FILEUTILS_H__
#define __ARC_FILEUTILS_H__

#include <glibmm.h>

#include <arc/Thread.h>


namespace Arc {

  // Utility functions for handling files and directories.
  // These functions offer possibility to access files and directories
  // under user and group ids different from those of current user.
  // Id switching is done in a safe way for multi-threaded applications.
  // If any of specified ids is 0 then such id is not switched and
  // current id is used instead.

  /** \addtogroup common
   *  @{ */

  /// Copy file source_path to file destination_path.
  /** Specified uid and gid are used for accessing filesystem. */
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
  /** Specified uid and gid are used for accessing filesystem. The content is
   * split into lines with the new line character removed, and the lines are
   * returned in the data list. If protected access is required, FileLock
   * should be used in addition to FileRead. */
  bool FileRead(const std::string& filename, std::list<std::string>& data, uid_t uid=0, gid_t gid=0);

  /// Simple method to read whole file content from filename.
  /** Specified uid and gid are used for accessing filesystem. */
  bool FileRead(const std::string& filename, std::string& data, uid_t uid=0, gid_t gid=0);

  /// Simple method to create a new file containing given data.
  /** Specified uid and gid are used for accessing filesystem. An existing file
   * is overwritten with the new data. Overwriting is performed atomically so
   * the file is guaranteed to exist throughout the duration of this method.
   * Permissions of the created file are determined by mode, the default is 644
   * or 600 if uid and gid are non-zero. If protected access is required,
   * FileLock should be used in addition to FileCreate. If uid/gid are zero
   * then no real switch of uid/gid is done. */
  bool FileCreate(const std::string& filename, const std::string& data, uid_t uid=0, gid_t gid=0, mode_t mode = 0);

  /// Stat a file and put info into the st struct
  bool FileStat(const std::string& path,struct stat *st,bool follow_symlinks);

  /// Stat a file using the specified uid and gid and put info into the st struct.
  /** Specified uid and gid are used for accessing filesystem. */
  bool FileStat(const std::string& path,struct stat *st,uid_t uid,gid_t gid,bool follow_symlinks);

  /// Make symbolic or hard link of file.
  bool FileLink(const std::string& oldpath,const std::string& newpath, bool symbolic);

  /// Make symbolic or hard link of file using the specified uid and gid.
  /** Specified uid and gid are used for accessing filesystem. */
  bool FileLink(const std::string& oldpath,const std::string& newpath,uid_t uid,gid_t gid,bool symbolic);

  /// Returns path at which symbolic link is pointing.
  std::string FileReadLink(const std::string& path);

  /// Returns path at which symbolic link is pointing using the specified uid and gid.
  /** Specified uid and gid are used for accessing filesystem. */
  std::string FileReadLink(const std::string& path,uid_t uid,gid_t gid);

  /// Deletes file at path.
  bool FileDelete(const std::string& path);

  /// Deletes file at path using the specified uid and gid.
  /** Specified uid and gid are used for accessing filesystem. */
  bool FileDelete(const std::string& path,uid_t uid,gid_t gid);

  /// Create a new directory.
  bool DirCreate(const std::string& path,mode_t mode,bool with_parents = false);

  /// Create a new directory using the specified uid and gid.
  /** Specified uid and gid are used for accessing filesystem. */
  bool DirCreate(const std::string& path,uid_t uid,gid_t gid,mode_t mode,bool with_parents = false);

  /// Delete a directory, and its content if recursive is true.
  /** If the directory is not empty and recursive is false DirDelete will fail. */
  bool DirDelete(const std::string& path, bool recursive = true);

  /// Delete a directory, and its content if recursive is true.
  /** If the directory is not empty and recursive is false DirDelete will fail.
      Specified uid and gid are used for accessing filesystem. */
  bool DirDelete(const std::string& path, bool recursive, uid_t uid, gid_t gid);

  /// List all entries in a directory.
  /** If path is not a directory or cannot be listed then false is returned. On
      success entries is filled with the list of entries in the directory. The
      entries are appended to path. */
  bool DirList(const std::string& path, std::list<std::string>& entries, bool recursive);

  /// List all entries in a directory using the specified uid and gid.
  /** If path is not a directory or cannot be listed then false is returned. On
      success entries is filled with the list of entries in the directory. The
      entries are appended to path. */
  bool DirList(const std::string& path, std::list<std::string>& entries, bool recursive, uid_t uid, gid_t gid);

  /// Create a temporary directory under the system defined temp location, and return its path.
  /** Uses mkdtemp if available, and a combination of random parameters if not.
      This latter method is not as safe as mkdtemp. */
  bool TmpDirCreate(std::string& path);

  /// Simple method to create a temporary file containing given data.
  /** Specified uid and gid are used for accessing filesystem. Permissions of
   * the created file are determined by mode, with default being read/write
   * only by owner. If uid/gid are zero then no real switch of uid/gid is done.
   * If the parameter filename ends with "XXXXXX" then the file created has the
   * same path as filename with these characters replaced by random values. If
   * filename has any other value or is empty then the file is created in the
   * system defined temp location. On success filename contains the name of the
   * temporary file. The content of the data argument is written into this
   * file. This method returns true if data was successfully written to the
   * temporary file, false otherwise. */
  bool TmpFileCreate(std::string& filename, const std::string& data, uid_t uid=0, gid_t gid=0, mode_t mode = 0);

  /// Removes /../ from 'name'.
  /** If leading_slash=true '/' will be added at the beginning of 'name' if
   * missing. Otherwise it will be removed. The directory separator used here
   * depends on the platform. If trailing_slash=true a trailing slash will not
   * be removed.
   * \return false if it is not possible to remove all the ../ */
  bool CanonicalDir(std::string& name, bool leading_slash = true, bool trailing_slash = false);

  /** @} */
} // namespace Arc

#endif // __ARC_FILEUTILS_H__
