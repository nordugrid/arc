// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/compute/SubmissionStatus.h>

#include "SubmitterPluginTestACC.h"

namespace Arc {

  SubmissionStatus SubmitterPluginTestACC::Submit(const std::list<JobDescription>& jobdescs,
                                                  const ExecutionTarget& et,
                                                  EntityConsumer<Job>& jc,
                                                  std::list<const JobDescription*>& notSubmitted) {
    SubmissionStatus retval = SubmitterPluginTestACCControl::submitStatus;
    if (SubmitterPluginTestACCControl::submitStatus) {
      jc.addEntity(SubmitterPluginTestACCControl::submitJob);
    }
    else for (std::list<JobDescription>::const_iterator it = jobdescs.begin();
              it != jobdescs.end(); ++it) {
      notSubmitted.push_back(&*it);
      retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
    }
    return retval;
  }

  SubmissionStatus SubmitterPluginTestACC::Submit(const std::list<JobDescription>& jobdescs,
                                                  const std::string& endpoint,
                                                  EntityConsumer<Job>& jc,
                                                  std::list<const JobDescription*>& notSubmitted) {
    SubmissionStatus retval = SubmitterPluginTestACCControl::submitStatus;
    if (retval) {
      jc.addEntity(SubmitterPluginTestACCControl::submitJob);
    }
    else for (std::list<JobDescription>::const_iterator it = jobdescs.begin();
              it != jobdescs.end(); ++it) {
      notSubmitted.push_back(&*it);
      retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
    }
    return retval;
  }

}
