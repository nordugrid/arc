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

/*static Arc::PDP* get_pdp(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new Arc::SimpleListPDP(cfg);
}

pdp_descriptors ARC_PDP_LOADER = {
    { "simplelist.pdp", 0, &get_pdp},
    { NULL, 0, NULL }
};
*/
using namespace Arc;

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
     if(!(line.compare(subject))){
        fs.close();
        logger.msg(INFO, "Authorized from arc.pdp!!!");
        return true;
     }
  }
  fs.close();
  logger.msg(ERROR, "UnAuthorized from simplelist.pdp!!!");
  return false;
}
