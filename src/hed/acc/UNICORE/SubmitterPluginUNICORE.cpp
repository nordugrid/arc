// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/client/JobDescription.h>
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
  
  bool SubmitterPluginUNICORE::Submit(const std::list<JobDescription>& jobdescs, const std::string& endpoint, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted, const URL& jobInformationEndpoint) {
    return false;
  }
  
  bool SubmitterPluginUNICORE::Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted) {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    bool ok = true;
    for (std::list<JobDescription>::const_iterator it = jobdescs.begin(); it != jobdescs.end(); ++it) {
      UNICOREClient uc(URL(et.ComputingEndpoint->URLString), cfg, usercfg.Timeout());
  
      XMLNode id;
      if (!uc.submit(*it, id)){
        ok = false;
        notSubmitted.push_back(&*it);
        continue;
      }
  
      Job j;
      id.GetDoc(j.IDFromEndpoint);
      AddJobDetails(*it, (std::string)id["Address"], et.ComputingService->Cluster, j);
      jc.addEntity(j);
    }
  
    return ok;
  }
} // namespace Arc
