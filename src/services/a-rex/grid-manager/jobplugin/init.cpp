#include <iostream>

#define GRIDFTP_PLUGIN
#include "jobplugin.h"


static FilePlugin* init_cpp(std::istream &cfile,userspec_t &user) {
  JobPlugin* arg = new JobPlugin(cfile,user);
  return arg;
}

extern "C" {
  FilePlugin* init(std::istream &cfile,userspec_t &user) {
    return init_cpp(cfile,user);
  }
}

