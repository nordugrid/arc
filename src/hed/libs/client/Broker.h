#ifndef __ARC_BROKER_H__
#define __ARC_BROKER_H__

#include <vector>

#include <arc/Logger.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/JobDescription.h>
#include <arc/client/ExecutionTarget.h>

namespace Arc {
  
  class Broker : public ACC {
    
  public:
    ExecutionTarget& GetBestTarget(bool &EndOfList);
    
  protected:
    Broker(Config *cfg);
    virtual ~Broker();
    void PreFilterTargets(Arc::TargetGenerator& targen, 
			  Arc::JobDescription jd);
    virtual void SortTargets() = 0;
    std::vector<Arc::ExecutionTarget> PossibleTargets;
    
  private:	
    std::vector<Arc::ExecutionTarget>::iterator current;
    bool PreFilteringDone;
    bool TargetSortingDone;
    
  };
  
} // namespace Arc

#endif // __ARC_BROKER_H__



