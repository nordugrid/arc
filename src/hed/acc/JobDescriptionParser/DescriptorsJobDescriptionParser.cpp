// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "XRSLParser.h"
#include "ADLParser.h"

extern Arc::PluginDescriptor const ARC_PLUGINS_TABLE_NAME[] = {
  { "XRSLParser", "HED:JobDescriptionParserPlugin", "NorduGrid xRSL (nordugrid:xrsl)", 0, &Arc::XRSLParser::Instance },
  { "EMIESADLParser", "HED:JobDescriptionParserPlugin", "EMI-ES ADL (emies:adl)", 0, &Arc::ADLParser::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
