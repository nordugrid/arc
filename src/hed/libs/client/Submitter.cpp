#include "Submitter.h"
#include <arc/ArcConfig.h>

namespace Arc {

  Submitter::Submitter(Config *cfg) : ACC() {
    endpoint = (std::string)(*cfg)["Endpoint"];
  }

  Submitter::~Submitter() {}

} // namespace Arc
