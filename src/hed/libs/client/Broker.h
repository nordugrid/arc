// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_BROKER_H__
#define __ARC_BROKER_H__

#include <vector>

#include <arc/Logger.h>
#include <arc/client/ACC.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/JobDescription.h>
#include <arc/client/ExecutionTarget.h>

namespace Arc {

  class Broker
    : public ACC {

  public:
    /// Returns next target from the list of ExecutionTarget objects
    /** When first called this method will sort its list of ExecutionTarget
        objects, which have been filled by the PreFilterTargets method, and
        then the first target in the list will be returned.

        If this is not the first call then the next target in the list is
        simply returned.

        If there are no targets in the list or the end of the target list have
        been reached the NULL pointer is returned.
      
        \return The pointer to the next ExecutionTarget in the list is returned.
     */
    const ExecutionTarget* GetBestTarget();
    /// ExecutionTarget filtering, view-point: enought memory, diskspace, CPUs, etc.
    /** The "bad" targets will be ignored and only the good targets will be added to
        to the list of ExecutionTarget objects which be used for brokering.
        \param targets A list of ExecutionTarget objects to be considered for
               addition to the Broker.
        \param jd JobDescription object of the actual job.
     */
    void PreFilterTargets(std::list<ExecutionTarget>& targets,
                          const JobDescription& job);

    /// Register a job submission to the current target
    void RegisterJobsubmission();

  protected:
    Broker(Config *cfg);
    virtual ~Broker();
    /// Custom Brokers should implement this method
    /** The task is to sort the PossibleTargets list by "custom"
        way, for example: FastestQueueBroker, ExecutionTarget which has
        the shortest queue lenght will be at the begining of the PossibleTargets list
     */
    virtual void SortTargets() = 0;
    /// This content the Prefilteres ExecutionTargets
    /** If an Execution Tartget has enought memory, CPU, diskspace, etc. for the
        actual job requirement than it will be added to the PossibleTargets list
     */
    std::list<ExecutionTarget*> PossibleTargets;
    /// It is true if "custom" sorting is done
    bool TargetSortingDone;
    const JobDescription* job;

    static Logger logger;

  private:
    /// This is a pointer for the actual ExecutionTarget in the PossibleTargets list
    std::list<ExecutionTarget*>::iterator current;
  };

} // namespace Arc

#endif // __ARC_BROKER_H__
