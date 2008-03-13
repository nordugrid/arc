#include <iostream>
#include <unistd.h>
#include <arc/ArcLocation.h>
#include <arc/Logger.h>

#include "ClientTool.h"

namespace Arc {

static char default_optstr[] = ":hd:l:";

ClientTool::ClientTool(const std::string& name):success_(false),logcerr_(std::cerr),logdest_(logfile_),name_(name),option_idx_(0) {
}

bool ClientTool::ProcessOptions(int argc,char* argv[],const std::string& optstr) {
  int optchar;
  std::string option_str(default_optstr);
  option_str+=optstr;
  if((argc > 0) && (argv[0] != NULL) && (argv[0][0] != 0)) {
    ArcLocation::Init(argv[0]);
  };
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
  return true;
}

bool ClientTool::ProcessOption(char option,char*) {
  std::cerr<<"Error processing option: "<<(char)option<<std::endl;
  PrintHelp();
  return false;
}

ClientTool::~ClientTool(void) {
  Logger::getRootLogger().removeDestinations();
}

void ClientTool::PrintHelp(void) {
  std::cout<<name_<<" [-h] [-d debug_level] [-l logfile]"<<std::endl;
  std::cout<<"\tPossible debug levels are VERBOSE, DEBUG, INFO, WARNING, ERROR and FATAL"<<std::endl;
}

}

