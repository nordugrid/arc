// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "TargetRetrieverTestACC.h"

Arc::Plugin* TargetRetrieverTestACC::GetInstance(Arc::PluginArgument *arg) {
  Arc::TargetRetrieverPluginArgument *trarg = dynamic_cast<Arc::TargetRetrieverPluginArgument*>(arg);
  if (!trarg) {
    return NULL;
  }
  return new TargetRetrieverTestACC(*trarg, *trarg, *trarg);
}

void TargetRetrieverTestACC::GetTargets(Arc::TargetGenerator& mom, int targetType, int /*detailLevel*/) {
  TargetRetrieverTestACCControl::tg = &mom;
  if (targetType == 0 && TargetRetrieverTestACCControl::foundTargets) {
    for (std::list<Arc::ExecutionTarget>::const_iterator it = TargetRetrieverTestACCControl::foundTargets->begin();
         it != TargetRetrieverTestACCControl::foundTargets->end(); ++it) {
      mom.AddTarget(*it);
    }
  }
  else if (targetType == 1 && TargetRetrieverTestACCControl::foundJobs) {
    for (std::list<Arc::Job>::const_iterator it = TargetRetrieverTestACCControl::foundJobs->begin();
         it != TargetRetrieverTestACCControl::foundJobs->end(); ++it) {
      mom.AddJob(*it);
    }
  }
}

void TargetRetrieverTestACC::GetExecutionTargets(Arc::TargetGenerator& mom) {
  TargetRetrieverTestACCControl::tg = &mom;
  if (TargetRetrieverTestACCControl::foundTargets) {
    for (std::list<Arc::ExecutionTarget>::const_iterator it = TargetRetrieverTestACCControl::foundTargets->begin();
         it != TargetRetrieverTestACCControl::foundTargets->end(); ++it) {
      mom.AddTarget(*it);
    }
  }
}

void TargetRetrieverTestACC::GetJobs(Arc::TargetGenerator& mom) {
  TargetRetrieverTestACCControl::tg = &mom;
  if (TargetRetrieverTestACCControl::foundJobs) {
    for (std::list<Arc::Job>::const_iterator it = TargetRetrieverTestACCControl::foundJobs->begin();
         it != TargetRetrieverTestACCControl::foundJobs->end(); ++it) {
      mom.AddJob(*it);
    }
  }
}
