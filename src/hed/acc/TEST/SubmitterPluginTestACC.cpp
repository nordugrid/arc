// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SubmitterPluginTestACC.h"

namespace Arc {

  bool SubmitterPluginTestACC::Submit(const std::list<JobDescription>& jobdescs,
                                      const ExecutionTarget& et,
                                      EntityConsumer<Job>& jc,
                                      std::list<const JobDescription*>& notSubmitted) {
    if (SubmitterPluginTestACCControl::submitStatus) {
      jc.addEntity(SubmitterPluginTestACCControl::submitJob);
    }
    else for (std::list<JobDescription>::const_iterator it = jobdescs.begin();
              it != jobdescs.end(); ++it) {
      notSubmitted.push_back(&*it);
    }
    return SubmitterPluginTestACCControl::submitStatus;
  }

  bool SubmitterPluginTestACC::Submit(const std::list<JobDescription>& jobdescs,
                                      const std::string& endpoint,
                                      EntityConsumer<Job>& jc,
                                      std::list<const JobDescription*>& notSubmitted,
                                      const URL& jobInformationEndpoint) {
    if (SubmitterPluginTestACCControl::submitStatus) {
      jc.addEntity(SubmitterPluginTestACCControl::submitJob);
    }
    else for (std::list<JobDescription>::const_iterator it = jobdescs.begin();
              it != jobdescs.end(); ++it) {
      notSubmitted.push_back(&*it);
    }
    return SubmitterPluginTestACCControl::submitStatus;
  }

}
