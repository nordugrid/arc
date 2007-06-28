#include "../../libs/loader/PDPLoader.h"
#include "../../../libs/common/XMLNode.h"
#include "../../../libs/common/Thread.h"
#include "../../../libs/common/ArcConfig.h"

#include "ArcPDP.h"
/*
static Arc::PDP* get_pdp(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new Arc::ArcPDP(cfg);
}

pdp_descriptor __arc_pdp_modules__[] = {
    { "arc.pdp", 0, &get_pdp},
    { NULL, 0, NULL }
};
*/
using namespace Arc;

PDP* ArcPDP::get_arc_pdp(Config *cfg,ChainContext *ctx) {
    return new ArcPDP(cfg);
}

ArcPDP::ArcPDP(Config* cfg):PDP(cfg){
  location = (std::string)(cfg->Attribute("location"));
  logger.msg(LogMessage(INFO, "Arc access list location: "+location));
}

bool ArcPDP::isPermitted(std::string subject){
  return false;
}
