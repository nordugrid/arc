#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <sys/types.h>

#include <arc/XMLNode.h>
#include <arc/Thread.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>

#include "SimpleListPDP.h"

Arc::Logger ArcSec::SimpleListPDP::logger(Arc::Logger::rootLogger, "SimpleListPDP");

using namespace Arc;
using namespace ArcSec;

Plugin* SimpleListPDP::get_simplelist_pdp(PluginArgument* arg) {
    ArcSec::PDPPluginArgument* pdparg =
            arg?dynamic_cast<ArcSec::PDPPluginArgument*>(arg):NULL;
    if(!pdparg) return NULL;
    return new SimpleListPDP((Arc::Config*)(*pdparg));
}

SimpleListPDP::SimpleListPDP(Config* cfg):PDP(cfg){
  location = (std::string)(cfg->Attribute("location"));
  logger.msg(INFO, "Access list location: %s", location);
}

bool SimpleListPDP::isPermitted(Message *msg){
  std::string subject=msg->Attributes()->get("TLS:IDENTITYDN");
  std::string line;
  if(location.empty()) {
    logger.msg(ERROR, "No policy file setup for simplelist.pdp, please set location attribute for simplelist PDP node in service configuration");
    return false; 
  }
  std::ifstream fs(location.c_str());
  if(fs.fail()) {
    logger.msg(ERROR, "The policy file setup for simplelist.pdp does not exist, please check location attribute for simplelist PDP node in service configuration");
    return false;
  }   

  while (!fs.eof()) {
    std::string::size_type p;
    getline (fs, line);
    logger.msg(INFO, "policy line: %s", line);
    logger.msg(INFO, "subject: %s", subject);
    p=line.find_first_not_of(" \t"); line.erase(0,p);
    p=line.find_last_not_of(" \t"); if(p != std::string::npos) line.erase(p+1);
    if(!line.empty()) {
      if(line[0] == '"') {
        std::string::size_type p = line.find('"',1);
        if(p != std::string::npos) line=line.substr(1,p-1);
      };
    };
    if(!line.empty()) {
      if(!(line.compare(subject))) {
         fs.close();
         logger.msg(INFO, "Authorized from simplelist.pdp");
         return true;
      }
    }
  }
  fs.close();
  logger.msg(ERROR, "Not authorized from simplelist.pdp");
  return false;
}
