#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <sys/types.h>

#include <arc/loader/PDPLoader.h>
#include <arc/XMLNode.h>
#include <arc/Thread.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>

#include "SimpleListPDP.h"

//Arc::Logger ArcSec::SimpleListPDP::logger(ArcSec::PDP::logger,"SimpleListPDP");
Arc::Logger ArcSec::SimpleListPDP::logger(Arc::Logger::rootLogger, "SimpleListPDP");

/*static ArcSec::PDP* get_pdp(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new ArcSec::SimpleListPDP(cfg);
}

pdp_descriptors ARC_PDP_LOADER = {
    { "simplelist.pdp", 0, &get_pdp},
    { NULL, 0, NULL }
};
*/
using namespace Arc;
using namespace ArcSec;

PDP* SimpleListPDP::get_simplelist_pdp(Config *cfg,ChainContext*) {
    return new SimpleListPDP(cfg);
}

SimpleListPDP::SimpleListPDP(Config* cfg):PDP(cfg){
  location = (std::string)(cfg->Attribute("location"));
  logger.msg(INFO, "Access list location: %s", location.c_str());
}

bool SimpleListPDP::isPermitted(Message *msg){
  std::string subject=msg->Attributes()->get("TLS:PEERDN");
  std::string line;
  std::ifstream fs(location.c_str());
  
  while (!fs.eof()) {
     getline (fs, line);
     logger.msg(INFO, "policy line: %s", line.c_str());
     logger.msg(INFO, "subject: %s", subject.c_str());
     line.erase(0,line.find_first_not_of(" \t"));
     line.erase(line.find_last_not_of(" \t"));
     if(!line.empty()) {
       if(line[0] == '"') {
         std::string::size_type p = line.find('"',1);
         if(p != std::string::npos) line=line.substr(1,p-1);
       };
     };
     logger.msg(INFO, "policy subject: %s", line.c_str());
     if(!(line.compare(subject))){
        fs.close();
        logger.msg(INFO, "Authorized from simplelist.pdp!!!");
        return true;
     }
  }
  fs.close();
  logger.msg(ERROR, "UnAuthorized from simplelist.pdp!!!");
  return false;
}
