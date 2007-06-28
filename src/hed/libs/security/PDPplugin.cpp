#include "../../libs/loader/PDPLoader.h"
#include "SimpleListPDP.h"
#include "ArcPDP.h"

using namespace Arc;

pdp_descriptor __arc_pdp_modules__[] = {
    { "simplelist.pdp", 0, &Arc::SimpleListPDP::get_simplelist_pdp},
    { "arc.pdp", 0, &Arc::ArcPDP::get_arc_pdp},
    { NULL, 0, NULL }
};

