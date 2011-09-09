// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ARCJSDLParser.h"
#include "JDLParser.h"
#include "XRSLParser.h"
#include "ADLParser.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "ARCJSDLParser", "HED:JobDescriptionParser", "NorduGrid ARC JSDL, POSIX-JSDL, HPCP-JSDL (nordugrid:jsdl)", 0, &Arc::ARCJSDLParser::Instance },
  { "JDLParser", "HED:JobDescriptionParser", "CREAM JDL (egee:jdl)", 0, &Arc::JDLParser::Instance },
  { "XRSLParser", "HED:JobDescriptionParser", "NorduGrid xRSL (nordugrid:xrsl)", 0, &Arc::XRSLParser::Instance },
  { "EMIESADLParser", "HED:JobDescriptionParser", "EMI-ES ADL (emies:adl)", 0, &Arc::ADLParser::Instance },
  { NULL, NULL, NULL, 0, NULL }
};
