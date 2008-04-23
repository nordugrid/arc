#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/PDPLoader.h>
#include "simplelistpdp/SimpleListPDP.h"
#include "ArcPDP.h"
#include "countpdp/CountPDP.h"
#include "allowpdp/AllowPDP.h"
#include "denypdp/DenyPDP.h"

using namespace ArcSec;

pdp_descriptors ARC_PDP_LOADER = {
    { "simplelist.pdp", 0, &ArcSec::SimpleListPDP::get_simplelist_pdp},
    { "arc.pdp", 0, &ArcSec::ArcPDP::get_arc_pdp},
    { "count.pdp", 0, &ArcSec::CountPDP::get_count_pdp},
    { "allow.pdp", 0, &ArcSec::AllowPDP::get_allow_pdp},
    { "deny.pdp", 0, &ArcSec::DenyPDP::get_deny_pdp},
    { NULL, 0, NULL }
};

