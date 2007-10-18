#include <sys/stat.h>
#include <sys/types.h>
#include <string>

/*
  make directory 'path' with all underlying directories (if needed)
  down to 'base_path'. 
*/
int mkdir_recursive(const std::string& base_path,const std::string& path,
                                            mode_t mode,uid_t uid,gid_t gid);

