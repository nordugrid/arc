// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/ExecutionTarget.h>

#include "FastestQueueBrokerPlugin.h"

namespace Arc {

  bool FastestQueueBrokerPlugin::operator()(const ExecutionTarget& lhs, const ExecutionTarget& rhs) const {
    if (lhs.ComputingShare->WaitingJobs == 0 && rhs.ComputingShare->WaitingJobs == 0) {
      return lhs.ComputingShare->FreeSlots <= rhs.ComputingShare->FreeSlots;
    }
    return lhs.ComputingShare->WaitingJobs*rhs.ComputingManager->TotalSlots <= lhs.ComputingManager->TotalSlots*rhs.ComputingShare->WaitingJobs;
  }

  bool FastestQueueBrokerPlugin::match(const ExecutionTarget& et) const {
    if (et.ComputingShare->WaitingJobs <= -1 || et.ComputingManager->TotalSlots <= -1 || et.ComputingShare->FreeSlots <= -1) {
      if (et.ComputingShare->WaitingJobs <= -1) {
        logger.msg(VERBOSE, "Target %s removed by FastestQueueBroker, doesn't report number of waiting jobs", et.AdminDomain->Name);
      }
      if (et.ComputingManager->TotalSlots <= -1) {
        logger.msg(VERBOSE, "Target %s removed by FastestQueueBroker, doesn't report number of total slots", et.AdminDomain->Name);
      }
      if (et.ComputingShare->FreeSlots <= -1) {
        logger.msg(VERBOSE, "Target %s removed by FastestQueueBroker, doesn't report number of free slots", et.AdminDomain->Name);
      }
      return false;
    }
    return true;
  }
} // namespace Arc
