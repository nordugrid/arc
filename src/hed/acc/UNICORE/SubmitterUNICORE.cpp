#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/client/JobDescription.h>

#include "SubmitterUNICORE.h"
#include "UNICOREClient.h"

namespace Arc {

  Logger SubmitterUNICORE::logger(Submitter::logger, "UNICORE");

  SubmitterUNICORE::SubmitterUNICORE(Config *cfg)
    : Submitter(cfg, "UNICORE") {}

  SubmitterUNICORE::~SubmitterUNICORE() {}

  ACC* SubmitterUNICORE::Instance(Config *cfg, ChainContext*) {
    return new SubmitterUNICORE(cfg);
  }

  bool SubmitterUNICORE::Submit(JobDescription& jobdesc, XMLNode& info) {
  }

} // namespace Arc
