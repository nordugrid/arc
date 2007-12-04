#ifndef __ARC_GM_DELETE_H__
#define __ARC_GM_DELETE_H__
#include <string>
#include <list>
#include "../files/info_types.h"

/*
  Delete all files and subdirectories in 'dir_base' which are or are not
  present in 'files' list.
  Accepts:
    dir_base - path to directory.
    files - list of files to delete/keep. Paths are relative to 'dir_base'.
    excl - if set to true all files excluding those in 'files' will be
      deleted. Otherwise - only 'files' will be deleted. If some of 
      'files' correspond to directories - whole directory will be
      deleted.
    lfn_exs, lfn_mis - if both set to true, whole 'files' list is
      used. If only one is true, then only entries of 'files' with
      lfn information available/not available will be used.
*/
int delete_all_files(const std::string &dir_base,const std::list<FileData> &files,
                     bool excl,bool lfn_exs = true,bool lfn_mis = true);
/*
  Delete all soft-links available in tree.
*/
int delete_all_links(const std::string &dir_base,const std::list<FileData> &files);

#endif 
