#include <iostream>
#include <fstream>
#include <sys/types.h>

#include "../../libs/loader/PDPLoader.h"
#include "../../../libs/common/XMLNode.h"
#include "../../../libs/common/Thread.h"
#include "../../../libs/common/Logger.h"
#include "../../../libs/common/ArcConfig.h"

#include "SimpleListPDP.h"

//Arc::Logger &l = Arc::Logger::rootLogger;

static Arc::PDP* get_pdp(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new Arc::SimpleListPDP(cfg);
}

pdp_descriptor __arc_pdp_modules__[] = {
    { "simplelist.pdp", 0, &get_pdp},
    { NULL, 0, NULL }
};

using namespace Arc;

SimpleListPDP::SimpleListPDP(Config* cfg):PDP(cfg){
  location = (std::string)(cfg->Attribute("location"));
  std::cerr<<"Access list location:\n"<<location<<std::endl;
}

bool SimpleListPDP::isPermitted(std::string subject){
  std::string line;
  std::ifstream fs(location.c_str());
  
  while (!fs.eof()) {
     getline (fs, line);
     std::cerr <<"policy line:"<<line<<std::endl;
     std::cerr<<"subject:"<<subject<<std::endl;
     if(!(line.compare(subject))){
	fs.close();
        return true;
     }
  }
  fs.close();
  std::cerr <<"UnAuthorized!!!"<<std::endl;
  return false;
}
