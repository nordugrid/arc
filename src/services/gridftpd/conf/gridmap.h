#ifndef __GRIDFTPD_GRIDMAP_H__
#define __GRIDFTPD_GRIDMAP_H__

#include <string>
#include <list>

namespace gridftpd {

  /*
    Read file specified by path argument.
    Returns:
      true - success
      false - error (most probaly file is missing)
      'ulist' contains unix usernames found in file - one per line or
      in gridmap-like format - separted by blank spaces.
  */
  //bool file_user_list(const std::string& path,std::string &ulist);
  //bool file_user_list(const std::string& path,std::list<std::string> &ulist);

} // namespace gridftpd

#endif // __GRIDFTPD_GRIDMAP_H__
