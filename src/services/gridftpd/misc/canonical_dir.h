#ifndef __ARC_GRIDFTPD_CANONICAL_DIR__
#define __ARC_GRIDFTPD_CANONICAL_DIR__
#include <string>

namespace gridftpd {

  /*
    Removes /../ from 'name'. If leading_slash=true '/' will be added
    at the beginning of 'name' if missing. Otherwise it will be removed.
    Returns:
      0 - success
      1 - failure, if impossible to remove all /../
    todo: move to bool return type (??).
  */
  int canonical_dir(std::string &name,bool leading_slash = true);

} // namespace gridftpd

#endif // __ARC_GRIDFTPD_CANONICAL_DIR__
