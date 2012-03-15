// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <algorithm>

#include <arc/client/ExecutionTarget.h>

#include "FastestQueueBroker.h"

namespace Arc {

  bool CompareExecutionTarget(const ExecutionTarget *T1,
                              const ExecutionTarget *T2) {
    //Scale queue to become cluster size independent
    float T1queue = (float)T1->ComputingShare->WaitingJobs / T1->ComputingManager->TotalSlots;
    float T2queue = (float)T2->ComputingShare->WaitingJobs / T2->ComputingManager->TotalSlots;
    return T1queue < T2queue;
  }

  FastestQueueBroker::FastestQueueBroker(const UserConfig& usercfg, PluginArgument* parg)
    : Broker(usercfg, parg) {}

  FastestQueueBroker::~FastestQueueBroker() {}

  Plugin* FastestQueueBroker::Instance(PluginArgument *arg) {
    BrokerPluginArgument *brokerarg = dynamic_cast<BrokerPluginArgument*>(arg);
    if (!brokerarg)
      return NULL;
    return new FastestQueueBroker(*brokerarg, arg);
  }

  void FastestQueueBroker::SortTargets() {

    logger.msg(VERBOSE, "FastestQueueBroker is filtering %d targets",
               PossibleTargets.size());

    //Remove clusters with incomplete information for target sorting
    std::list<ExecutionTarget*>::iterator iter = PossibleTargets.begin();
    while (iter != PossibleTargets.end()) {
      if ((*iter)->ComputingShare->WaitingJobs == -1 || (*iter)->ComputingManager->TotalSlots == -1 || (*iter)->ComputingShare->FreeSlots == -1) {
        if ((*iter)->ComputingShare->WaitingJobs == -1)
          logger.msg(VERBOSE, "Target %s removed by FastestQueueBroker, doesn't report number of waiting jobs", (*iter)->AdminDomain->Name);
        else if ((*iter)->ComputingManager->TotalSlots == -1)
          logger.msg(VERBOSE, "Target %s removed by FastestQueueBroker, doesn't report number of total slots", (*iter)->AdminDomain->Name);
        else if ((*iter)->ComputingShare->FreeSlots == -1)
          logger.msg(VERBOSE, "Target %s removed by FastestQueueBroker, doesn't report number of free slots", (*iter)->AdminDomain->Name);
        iter = PossibleTargets.erase(iter);
        continue;
      }
      iter++;
    }

    logger.msg(VERBOSE, "FastestQueueBroker will rank the following %d targets", PossibleTargets.size());
    iter = PossibleTargets.begin();
    for (int i = 1; iter != PossibleTargets.end(); iter++, i++)
      logger.msg(VERBOSE, "%d. Resource: %s; Queue: %s", i, (*iter)->AdminDomain->Name, (*iter)->ComputingShare->Name);

    //Sort the targets according to the number of waiting jobs (in % of the cluster size)
    PossibleTargets.sort(CompareExecutionTarget);

    //Check is several clusters(queues) have 0 waiting jobs
    int ZeroQueueCluster = 0;
    int TotalFreeCPUs = 0;
    for (iter = PossibleTargets.begin(); iter != PossibleTargets.end(); iter++)
      if ((*iter)->ComputingShare->WaitingJobs == 0) {
        ZeroQueueCluster++;
        TotalFreeCPUs += (*iter)->ComputingShare->FreeSlots / abs(job->Resources.SlotRequirement.NumberOfSlots);
      }

    //If several clusters(queues) have free slots (CPUs) do basic load balancing
    if (ZeroQueueCluster > 1)
      for (std::list<ExecutionTarget*>::iterator itN = PossibleTargets.begin();
           itN != PossibleTargets.end() && (*itN)->ComputingShare->WaitingJobs == 0;
           itN++) {
        double RandomCPU = rand() * TotalFreeCPUs;
        for (std::list<ExecutionTarget*>::iterator itJ = itN;
             itJ != PossibleTargets.end() && (*itJ)->ComputingShare->WaitingJobs == 0;
             itJ++) {
          if (((*itJ)->ComputingShare->FreeSlots / abs(job->Resources.SlotRequirement.NumberOfSlots)) > RandomCPU) {
            TotalFreeCPUs -= ((*itJ)->ComputingShare->FreeSlots / abs(job->Resources.SlotRequirement.NumberOfSlots));
            std::iter_swap(itJ, itN);
            break;
          }
          else
            RandomCPU -= ((*itJ)->ComputingShare->FreeSlots / abs(job->Resources.SlotRequirement.NumberOfSlots));
        }
      }

    logger.msg(VERBOSE, "Best targets are: %d", PossibleTargets.size());

    iter = PossibleTargets.begin();

    for (int i = 1; iter != PossibleTargets.end(); iter++, i++)
      logger.msg(VERBOSE, "%d. Resource: %s; Queue: %s", i, (*iter)->AdminDomain->Name, (*iter)->ComputingShare->Name);

    TargetSortingDone = true;

  }

} // namespace Arc
