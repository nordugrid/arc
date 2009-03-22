// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>

#include "JobDescriptionParser.h"

namespace Arc {

  Logger JobDescriptionParser::logger(Logger::getRootLogger(),
                                      "JobDescriptionParser");

  JobDescriptionParser::JobDescriptionParser() {}

  JobDescriptionParser::~JobDescriptionParser() {}

} // namespace Arc
