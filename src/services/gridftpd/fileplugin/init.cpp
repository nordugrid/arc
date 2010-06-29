#include <iostream>

#define GRIDFTP_PLUGIN
#include "fileplugin.h"


static FilePlugin* init_cpp(std::istream &cfile,userspec_t &user) {
  DirectFilePlugin* arg = new DirectFilePlugin(cfile,user);
  return arg;
}

extern "C" {
  FilePlugin* init(std::istream &cfile,userspec_t &user) {
    return init_cpp(cfile,user);
  }
}

