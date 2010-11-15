// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ARCJSDLParser.h"
#include "JDLParser.h"
#include "XRSLParser.h"

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
  { "ARCJSDLParser", "HED:JobDescriptionParser", 0, &Arc::ARCJSDLParser::Instance },
  { "JDLParser", "HED:JobDescriptionParser", 0, &Arc::JDLParser::Instance },
  { "XRSLParser", "HED:JobDescriptionParser", 0, &Arc::XRSLParser::Instance },
  { NULL, NULL, 0, NULL }
};
