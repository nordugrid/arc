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

#include "AllowPDP.h"

using namespace Arc;
using namespace ArcSec;

PDP* AllowPDP::get_allow_pdp(Config *cfg,ChainContext*) {
    return new AllowPDP(cfg);
}

AllowPDP::AllowPDP(Config* cfg):PDP(cfg){
}

bool AllowPDP::isPermitted(Message *msg){
  return true;
}

