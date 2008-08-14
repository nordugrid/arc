#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
#include <arc/data/DataBufferPar.h>
#include <arc/data/DataHandle.h>
#include <arc/StringConv.h>

#include "JobControllerARC1.h"
#include "arex_client.h"

#include <map>
#include <iostream>
#include <algorithm>

namespace Arc {

  JobControllerARC1::JobControllerARC1(Arc::Config *cfg)
    : JobController(cfg, "ARC1") {}
  
  JobControllerARC1::~JobControllerARC1() {}

  ACC *JobControllerARC1::Instance(Config *cfg, ChainContext *) {
    return new JobControllerARC1(cfg);
  }
  
  void JobControllerARC1::GetJobInformation() {
    for (std::list<Job>::iterator iter = JobStore.begin();
	 iter != JobStore.end(); iter++) {
      MCCConfig cfg;
      PathIterator pi(iter->JobID.Path(), true);
      URL url(iter->JobID);
      url.ChangePath(*pi);
      AREXClient ac(url, cfg);
      NS ns;
      ns["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
      ns["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
      ns["wsa"]="http://www.w3.org/2005/08/addressing";
      ns["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";
      ns["jsdl-posix"]="http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
      ns["jsdl-arc"]="http://www.nordugrid.org/ws/schemas/jsdl-arc";
      ns["jsdl-hpcpa"]="http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
      XMLNode id(ns, "ActivityIdentifier");
      id.NewChild("wsa:Address") = url.str();
      id.NewChild("wsa:ReferenceParameters").NewChild("a-rex:JobID") = pi.Rest();
      std::string idstr;
      id.GetXML(idstr);
      iter->State = ac.stat(idstr);
    }
  }

  bool JobControllerARC1::GetThisJob(Job ThisJob, std::string downloaddir) {}

  bool JobControllerARC1::CleanThisJob(Job ThisJob, bool force) {
    MCCConfig cfg;
    PathIterator pi(ThisJob.JobID.Path(), true);
    URL url(ThisJob.JobID);
    url.ChangePath(*pi);
    AREXClient ac(url, cfg);
    NS ns;
    ns["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
    ns["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
    ns["wsa"]="http://www.w3.org/2005/08/addressing";
    ns["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";
    ns["jsdl-posix"]="http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
    ns["jsdl-arc"]="http://www.nordugrid.org/ws/schemas/jsdl-arc";
    ns["jsdl-hpcpa"]="http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
    XMLNode id(ns, "ActivityIdentifier");
    id.NewChild("wsa:Address") = url.str();
    id.NewChild("wsa:ReferenceParameters").NewChild("a-rex:JobID") = pi.Rest();
    std::string idstr;
    id.GetXML(idstr);
    ac.clean(idstr);
    return true;
  }

  bool JobControllerARC1::CancelThisJob(Job ThisJob) {
    MCCConfig cfg;
    PathIterator pi(ThisJob.JobID.Path(), true);
    URL url(ThisJob.JobID);
    url.ChangePath(*pi);
    AREXClient ac(url, cfg);
    NS ns;
    ns["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
    ns["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
    ns["wsa"]="http://www.w3.org/2005/08/addressing";
    ns["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";
    ns["jsdl-posix"]="http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
    ns["jsdl-arc"]="http://www.nordugrid.org/ws/schemas/jsdl-arc";
    ns["jsdl-hpcpa"]="http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
    XMLNode id(ns, "ActivityIdentifier");
    id.NewChild("wsa:Address") = url.str();
    id.NewChild("wsa:ReferenceParameters").NewChild("a-rex:JobID") = pi.Rest();
    std::string idstr;
    id.GetXML(idstr);
    ac.kill(idstr);
    return true;
  }

  URL JobControllerARC1::GetFileUrlThisJob(Job ThisJob,
					   std::string whichfile) {}

} // namespace Arc
