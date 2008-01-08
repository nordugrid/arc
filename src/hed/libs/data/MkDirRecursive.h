#ifndef __ARC_DATA2_MKDIR_REC_H__
#define __ARC_DATA2_MKDIR_REC_H__  
#include <sys/stat.h>
#include <sys/types.h>
#include <string>
#include <arc/User.h>

/*
  make directory 'path' with all underlying directories (if needed)
  down to 'base_path'. 
*/
int mkdir_recursive(const std::string& base_path,const std::string& path,
                                            mode_t mode, const Arc::User &user);

#endif
