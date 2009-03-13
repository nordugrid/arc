#ifndef __ARC_BROKER_H__
#define __ARC_BROKER_H__

#include <vector>

#include <arc/Logger.h>
#include <arc/client/ACC.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/JobDescription.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/JobInnerRepresentation.h>

namespace Arc {
  
  class Broker : public ACC {
    
  public:
    /// Returns next target from Broker's list of Targets
    /** If there are no more targets then EndOfList is set to false and 
       reference points to invalid object - do not use it.
       If valid object is returned then EndOfList is set to true. 
       \param EndOfList The value is false if the current iterator value equal with PossibleTargets.end()
       \return That ExecutionTarget which are pointed by current iterator
    */
    ExecutionTarget& GetBestTarget(bool &EndOfList);
    /// ExecutionTarget filtering, view-point: enought memory, diskspace, CPUs, etc.
    /** The "bad" targets will be ignored and only the good targets will be added to
        to the PossibleTargets vector.
        \param targen TargetGenerator content the Execution Target candidates 
        \param jd Job Description of the actual job
    */
    void PreFilterTargets(const Arc::TargetGenerator& targen, 
			  Arc::JobDescription jd);
  protected:
    Broker(Config *cfg);
    virtual ~Broker();
    /// Custom Brokers should implement this method
    /** The task is to sort the PossibleTargets vector by "custom"
        way, for example: FastestQueueBroker, ExecutionTarget which has
        the shortest queue lenght will be at the begining of the PossibleTargets vector
    */
    virtual void SortTargets() = 0;
    /// This content the Prefilteres ExecutionTargets
    /** If an Execution Tartget has enought memory, CPU, diskspace, etc. for the 
        actual job requirement than it will be added to the PossibleTargets vector
    */
    std::vector<Arc::ExecutionTarget> PossibleTargets;
    /// It is true if "custom" sorting is done
    bool TargetSortingDone;
    Arc::JobInnerRepresentation jir;

    static Logger logger;

  private:	
    /// This is a pointer for the actual Execution Target in the PossibleTargets vector
    std::vector<Arc::ExecutionTarget>::iterator current;
    /// It is true if the prefiltering is ready
    bool PreFilteringDone;
  };
  
} // namespace Arc

#endif // __ARC_BROKER_H__



