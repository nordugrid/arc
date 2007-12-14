#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/PDPLoader.h>
#include "SimpleListPDP.h"
#include "ArcPDP.h"
#include "AllowPDP.h"
#include "DenyPDP.h"

using namespace ArcSec;

pdp_descriptors ARC_PDP_LOADER = {
    { "simplelist.pdp", 0, &ArcSec::SimpleListPDP::get_simplelist_pdp},
    { "arc.pdp", 0, &ArcSec::ArcPDP::get_arc_pdp},
    { "allow.pdp", 0, &ArcSec::AllowPDP::get_allow_pdp},
    { "deny.pdp", 0, &ArcSec::DenyPDP::get_deny_pdp},
    { NULL, 0, NULL }
};

