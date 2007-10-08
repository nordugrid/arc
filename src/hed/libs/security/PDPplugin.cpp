#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/PDPLoader.h>
#include "SimpleListPDP.h"
#include "ArcPDP.h"

using namespace ArcSec;

pdp_descriptors ARC_PDP_LOADER = {
    { "simplelist.pdp", 0, &ArcSec::SimpleListPDP::get_simplelist_pdp},
    { "arc.pdp", 0, &ArcSec::ArcPDP::get_arc_pdp},
    { NULL, 0, NULL }
};

