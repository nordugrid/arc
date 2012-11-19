// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobDescription.h>
#include <arc/compute/SubmissionStatus.h>
#include <arc/message/MCC.h>
#include <arc/ws-addressing/WSA.h>

#include "SubmitterPluginUNICORE.h"
#include "UNICOREClient.h"

namespace Arc {

  Logger SubmitterPluginUNICORE::logger(Logger::getRootLogger(), "SubmitterPlugin.UNICORE");

  bool SubmitterPluginUNICORE::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }
  
  SubmissionStatus SubmitterPluginUNICORE::Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted) {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    SubmissionStatus retval;
    for (std::list<JobDescription>::const_iterator it = jobdescs.begin(); it != jobdescs.end(); ++it) {
      UNICOREClient uc(URL(et.ComputingEndpoint->URLString), cfg, usercfg.Timeout());
  
      XMLNode id;
      if (!uc.submit(*it, id)){
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        notSubmitted.push_back(&*it);
        continue;
      }
  
      Job j;
      id.GetDoc(j.IDFromEndpoint);
      j.JobID = (std::string)id["Address"];
      AddJobDetails(*it, et.ComputingService->Cluster, j);
      jc.addEntity(j);
    }
  
    return retval;
  }
} // namespace Arc
