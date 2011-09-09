#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Plugin.h>

#include "LegacySecHandler.h"
#include "LegacyPDP.h"
#include "LegacyMap.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "arclegacy.handler", "HED:SHC", NULL, 0,
                  &ArcSHCLegacy::LegacySecHandler::get_sechandler},
    { "arclegacy.pdp", "HED:PDP", NULL, 0,
                  &ArcSHCLegacy::LegacyPDP::get_pdp},
    { "arclegacy.map", "HED:SHC", NULL, 0,
                  &ArcSHCLegacy::LegacyMap::get_sechandler},
    { NULL, NULL, NULL, 0, NULL }
};

