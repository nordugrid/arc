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

#include "DenyPDP.h"

using namespace Arc;
using namespace ArcSec;

PDP* DenyPDP::get_deny_pdp(Config *cfg,ChainContext*) {
    return new DenyPDP(cfg);
}

DenyPDP::DenyPDP(Config* cfg):PDP(cfg){
}

bool DenyPDP::isPermitted(Message *msg){
  return false;
}

