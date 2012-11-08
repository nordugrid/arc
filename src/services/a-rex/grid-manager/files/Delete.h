#ifndef __ARC_GM_DELETE_H__
#define __ARC_GM_DELETE_H__
#include <string>
#include <list>
#include "../files/ControlFileContent.h"

namespace ARex {

/**
  Delete all files and subdirectories in 'dir_base' which are or are not
  present in 'files' list.
  Accepts:
    dir_base - path to directory.
    files - list of files to delete/keep. Paths are relative to 'dir_base'.
    excl - if set to true all files excluding those in 'files' will be
      deleted. Otherwise - only files in 'files' which have LFN information
      will be deleted. If some of 'files' correspond to directories - whole
      directory will be deleted.
    uid - uid under which to perform file system operations
    gid - gid under which to perform file system operations
*/
int delete_all_files(const std::string &dir_base, const std::list<FileData> &files,
                     bool excl, uid_t uid = 0, gid_t gid = 0);

} // namespace ARex

#endif 
