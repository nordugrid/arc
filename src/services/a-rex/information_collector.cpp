#include <sstream>
#include <fstream>
#include <unistd.h>

#include <arc/Run.h>

#include "ldif/LDIFtoXML.h"
#include "config/ngconfig.h"
#include "grid-manager/conf/environment.h"
#include "job.h"
#include "arex.h"

namespace ARex {

static void GetGlueStates(Arc::XMLNode infodoc,std::map<std::string,std::string>& states);

void ARexService::InformationCollector(void) {
  for(;;) {
    std::string lrms;
    std::list<std::string> queues;

    // Parse configuration to construct command lines for information providers
    if(!ARexGMConfig::InitEnvironment(gmconfig_)) {
      logger_.msg(Arc::ERROR,"Failed to initialize GM environment");
      sleep(60); continue;
    };
    std::ifstream f(nordugrid_config_loc.c_str());
    if(!f) {
      logger_.msg(Arc::ERROR,"Failed to read GM configuation file at %s",nordugrid_config_loc);
      sleep(60); continue;
    };
    try {
      Config cfg = NGConfig().Read(f);
      const std::list<ConfGrp>& groups = cfg.GetConfigs();
      for(std::list<ConfGrp>::const_iterator g = groups.begin();g!=groups.end();++g) {
        std::string section = g->GetSection();
        std::string subsection;
        std::string::size_type p = section.find('/');
        if(p != std::string::npos) {
          subsection=section.substr(p+1);
          section.resize(p);
        };
        logger_.msg(Arc::DEBUG,"Configuration: Section: %s, Subsection: %s",section,subsection);
        if((section == "common") || (section == "grid-manager")) {
          std::list<std::string> values = g->FindOptionValue("lrms");
          if(values.size() > 0) lrms=*(values.begin());
          std::string::size_type p = lrms.find(' ');
          if(p != std::string::npos) lrms.resize(p);
          logger_.msg(Arc::DEBUG,"Configuration: LRMS: %s",lrms);
        } else if(section == "queue") {
          std::string queue = g->GetID();
          if(queue.empty()) queue=subsection;
          logger_.msg(Arc::DEBUG,"Configuration: Queue: %s",queue);
          if(!queue.empty()) queues.push_back(queue);
        };
      };
    } catch(ConfigError& e) {
    };
    // Run information provider
    std::string xml_str;
    {
      std::string cmd;
      cmd=nordugrid_libexec_loc+"/CEinfo.pl --config "+nordugrid_config_loc;
      Arc::Run run(cmd);
      std::string stdin_str;
      std::string stderr_str;
      run.AssignStdin(stdin_str);
      run.AssignStdout(xml_str);
      run.AssignStderr(stderr_str);
      logger_.msg(Arc::DEBUG,"Cluster information provider: %s",cmd);
      if(!run.Start()) {
      };
      if(!run.Wait(60)) {
      };
      int r = run.Result();
      logger_.msg(Arc::DEBUG,"Cluster information provider result: %i",r);
      logger_.msg(Arc::DEBUG,"Cluster information provider error: %s",stderr_str);
    };
    logger_.msg(Arc::VERBOSE,"Obtained XML: %s",xml_str);
    Arc::XMLNode root(xml_str);
    if(root) {
      // Collect job states
      GetGlueStates(root,glue_states_);
      // Put result into container
      infodoc_.Assign(root,true);
      logger_.msg(Arc::INFO,"Assigned new informational document");
    } else {
      logger_.msg(Arc::ERROR,"Failed to create informational document");
    };
    sleep(60);
  };
}

bool ARexService::RegistrationCollector(Arc::XMLNode &doc) {
  Arc::XMLNode root = infodoc_.Acquire();
  logger_.msg(Arc::VERBOSE,"Passing service's information from collector to registrator");
  // TODO: filter information here.
  Arc::XMLNode regdoc("<Service/>");
  regdoc.New(doc);
  doc.NewChild(root);
  infodoc_.Release();
}

std::string ARexService::getID() {
  return "ARC:AREX";
}

static void GetGlueStates(Arc::XMLNode infodoc,std::map<std::string,std::string>& states) {
  std::string path = "Domains/AdminDomain/Services/Service/ComputingActivities/ComputingActivity";
  // Obtaining all job descriptions
  Arc::XMLNodeList nodes = infodoc.Path(path);
  // Pulling ids and states
  for(Arc::XMLNodeList::iterator node = nodes.begin();node!=nodes.end();++node) {
    // Exract ID of job
    std::string id = (*node)["IDFromEndpoint"];
    if(id.empty()) id = (std::string)((*node)["ID"]);
    if(id.empty()) continue;
    std::string::size_type p = id.rfind('/');
    if(p != std::string::npos) id.erase(0,p+1);
    if(id.empty()) continue;
    std::string state  = (std::string)((*node)["State"]);
    if(state.empty()) continue;
    // Store state under id
    states[id] = state;
  };
}

}

