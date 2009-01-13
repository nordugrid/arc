#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/XMLNode.h>
#include <arc/message/MCC.h>

#include "UNICOREClient.h"
#include "JobControllerUNICORE.h"

namespace Arc {

  Logger JobControllerUNICORE::logger(JobController::logger, "UNICORE");

  JobControllerUNICORE::JobControllerUNICORE(Config *cfg)
    : JobController(cfg, "UNICORE") {}

  JobControllerUNICORE::~JobControllerUNICORE() {}

  ACC* JobControllerUNICORE::Instance(Config *cfg, ChainContext*) {
    return new JobControllerUNICORE(cfg);
  }

  void JobControllerUNICORE::GetJobInformation() {
  }

  bool JobControllerUNICORE::GetJob(const Job& job,
				 const std::string& downloaddir) {
  }

  bool JobControllerUNICORE::CleanJob(const Job& job, bool force) {
  }

  bool JobControllerUNICORE::CancelJob(const Job& job) {
  }

  URL JobControllerUNICORE::GetFileUrlForJob(const Job& job,
					  const std::string& whichfile) {}

} // namespace Arc
