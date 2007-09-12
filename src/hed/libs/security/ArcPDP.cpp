#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/PDPLoader.h>
#include <arc/XMLNode.h>
#include <arc/Thread.h>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>

#include "ArcPDP.h"
/*
static Arc::PDP* get_pdp(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new Arc::ArcPDP(cfg);
}

pdp_descriptors ARC_PDP_LOADER = {
    { "arc.pdp", 0, &get_pdp},
    { NULL, 0, NULL }
};
*/
using namespace Arc;

PDP* ArcPDP::get_arc_pdp(Config *cfg,ChainContext *ctx __attribute__((unused))) {
    return new ArcPDP(cfg);
}

ArcPDP::ArcPDP(Config* cfg):PDP(cfg){
  location = (std::string)(cfg->Attribute("location"));
  logger.msg(INFO, "Arc access list location: %s", location.c_str());
}

bool ArcPDP::isPermitted(std::string subject __attribute__((unused))){
  return false;
}
