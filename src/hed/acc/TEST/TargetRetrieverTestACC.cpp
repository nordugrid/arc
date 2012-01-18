// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "TargetRetrieverTestACC.h"

namespace Arc {

Plugin* TargetRetrieverTestACC::GetInstance(PluginArgument *arg) {
  TargetRetrieverPluginArgument *trarg = dynamic_cast<TargetRetrieverPluginArgument*>(arg);
  if (!trarg) {
    return NULL;
  }
  return new TargetRetrieverTestACC(*trarg, *trarg, *trarg);
}

void TargetRetrieverTestACC::GetExecutionTargets(TargetGenerator& mom) {
  TargetRetrieverTestACCControl::tg = &mom;
  for (std::list<ExecutionTarget>::const_iterator it = TargetRetrieverTestACCControl::foundTargets.begin();
       it != TargetRetrieverTestACCControl::foundTargets.end(); ++it) {
    mom.AddTarget(*it);
  }
}

void TargetRetrieverTestACC::GetJobs(TargetGenerator& mom) {
  TargetRetrieverTestACCControl::tg = &mom;
  for (std::list<Job>::const_iterator it = TargetRetrieverTestACCControl::foundJobs.begin();
       it != TargetRetrieverTestACCControl::foundJobs.end(); ++it) {
    mom.AddJob(*it);
  }
}

}
