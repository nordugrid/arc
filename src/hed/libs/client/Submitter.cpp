#include <arc/ArcConfig.h>
#include <arc/client/Submitter.h>

namespace Arc {

  Submitter::Submitter(Config *cfg)
    : ACC() {
    SubmissionEndpoint = (std::string)(*cfg)["Endpoint"];
    InfoEndpoint = (std::string)(*cfg)["Source"];
    MappingQueue = (std::string)(*cfg)["MappingQueue"];
  }

  Submitter::~Submitter() {}

} // namespace Arc
