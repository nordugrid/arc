// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>

#include <arc/compute/TestACCControl.h>

#include "JobListRetrieverPluginTEST.h"

namespace Arc {

EndpointQueryingStatus JobListRetrieverPluginTEST::Query(const UserConfig&,
                                                          const Endpoint&,
                                                          std::list<Job>& jobs,
                                                          const EndpointQueryOptions<Job>&) const {
  usleep(JobListRetrieverPluginTESTControl::delay*1000000);
  jobs = JobListRetrieverPluginTESTControl::jobs;
  return JobListRetrieverPluginTESTControl::status;
};


}
