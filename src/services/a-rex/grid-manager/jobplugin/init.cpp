#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#define GRIDFTP_PLUGIN
#include "jobplugin.h"


static FilePlugin* init_cpp(std::istream &cfile,userspec_t const &user,FileNode &node) {
  JobPlugin* arg = new JobPlugin(cfile,user,node);
  return arg;
}

extern "C" {
  FilePlugin* init(std::istream &cfile,userspec_t const &user,FileNode &node) {
    return init_cpp(cfile,user,node);
  }
}

