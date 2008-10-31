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
    /// Returns next target from Broker's list of Targets
    /** If there are no more targets then EndOfList is set to false and 
       reference points to invalid object - do not use it.
       If valid object is returned then EndOfList is set to true. */
    ExecutionTarget& GetBestTarget(bool &EndOfList);
    void PreFilterTargets(Arc::TargetGenerator& targen, 
			  Arc::JobDescription jd);
    
  protected:
    Broker(Config *cfg);
    virtual ~Broker();
    virtual void SortTargets() = 0;
    std::vector<Arc::ExecutionTarget> PossibleTargets;
    bool TargetSortingDone;
    
  private:	
    std::vector<Arc::ExecutionTarget>::iterator current;
    bool PreFilteringDone;
    
  };
  
} // namespace Arc

#endif // __ARC_BROKER_H__



