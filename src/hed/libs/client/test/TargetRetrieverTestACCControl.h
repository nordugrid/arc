#ifndef __ARC_TARGETRETRIEVERTESTACCCONTROL_H__
#define __ARC_TARGETRETRIEVERTESTACCCONTROL_H__

#include <arc/client/Job.h>

class TargetRetrieverTestACCControl {
  public:
    static Arc::TargetGenerator* tg;
};

Arc::TargetGenerator* TargetRetrieverTestACCControl::tg = NULL;

#endif // __ARC_TARGETRETRIEVERTESTACCCONTROL_H__
