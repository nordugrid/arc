#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#define GRIDFTP_PLUGIN
#include "gaclplugin.h"


static FilePlugin* init_cpp(std::istream &cfile,userspec_t &user) {
  GACLPlugin* arg = new GACLPlugin(cfile,user);
  return arg;
}

extern "C" {
  FilePlugin* init(std::istream &cfile,userspec_t &user,FileNode &node) {
    return init_cpp(cfile,user);
  }
}

