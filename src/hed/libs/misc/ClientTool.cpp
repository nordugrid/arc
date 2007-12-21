#include <iostream>
#include <unistd.h>
#include <arc/Logger.h>

#include "ClientTool.h"

namespace Arc {

static char default_optstr[] = ":hd:l:";

ClientTool::ClientTool(int argc,char* argv[],const std::string& name,const std::string& optstr):success_(false),logcerr_(std::cerr),logdest_(logfile_),option_idx_(0) {
  int optchar;
  std::string option_str(default_optstr);
  option_str+=optstr;
  while((optchar=getopt(argc,argv,option_str.c_str())) != -1) {
    switch(optchar) {
      case '?': {
        std::cerr<<"Unknown option: "<<(char)optopt<<std::endl;
        _exit(EXIT_FAILURE);
      }; break;
      case ':': {
        std::cerr<<"Missing argument in option: "<<(char)optopt<<std::endl;
        _exit(EXIT_FAILURE);
      }; break;
      case 'h': {
        PrintHelp(); _exit(EXIT_SUCCESS);
      }; break;
      case 'l': {
        logfile_.open(optarg);
      }; break;
      case 'd': {
        Logger::getRootLogger().setThreshold(string_to_level(optarg));
      }; break;
      default: {
        if(!ProcessOption(optchar,optarg)) _exit(EXIT_FAILURE);
      }; break;
    };
  };
  //Logger::getRootLogger().setDomain(name);
  if(logfile_.is_open()) {
    Logger::getRootLogger().addDestination(logdest_);
  } else {
    Logger::getRootLogger().addDestination(logcerr_);
  };
  option_idx_=optind;
  success_=true;
}

bool ClientTool::ProcessOption(char option,char* option_arg) {
  std::cerr<<"Error processing option: "<<(char)option<<std::endl;
  PrintHelp();
  return false;
}

ClientTool::~ClientTool(void) {
  Logger::getRootLogger().removeDestinations();
}

}

