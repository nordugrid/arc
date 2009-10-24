#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

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
  thread_count_.RegisterThread();
  for(;;) {
    // Run information provider
    std::string xml_str;
    int r = -1;
    {
      std::string cmd;
      cmd=nordugrid_libexec_loc()+"/CEinfo.pl --config "+nordugrid_config_loc();
      std::string stdin_str;
      std::string stderr_str;
      Arc::Run run(cmd);
      run.AssignStdin(stdin_str);
      run.AssignStdout(xml_str);
      run.AssignStderr(stderr_str);
      logger_.msg(Arc::DEBUG,"Cluster information provider: %s",cmd);
      if(!run.Start()) {
      };
      if(!run.Wait(infoprovider_wakeup_period_)) {
        logger_.msg(Arc::WARNING,"Cluster information provider timeout: %u seconds",infoprovider_wakeup_period_);
      } else {
        r = run.Result();
        if (r!=0) logger_.msg(Arc::WARNING,"Cluster information provider failed with exit status: %i",r);
      };
      logger_.msg(Arc::DEBUG,"Cluster information provider log:\n%s",stderr_str);
    };
    if (r!=0) {
      logger_.msg(Arc::DEBUG,"No new informational document assigned");
    } else {
      logger_.msg(Arc::VERBOSE,"Obtained XML: %s",xml_str);
      Arc::XMLNode root(xml_str);
      if(root) {
        // Collect job states
        GetGlueStates(root,glue_states_);
        // Put result into container
        infodoc_.Assign(root,true);
        logger_.msg(Arc::DEBUG,"Assigned new informational document");
      } else {
        logger_.msg(Arc::ERROR,"Failed to create informational document");
      };
    }
    if(thread_count_.WaitOrCancel(infoprovider_wakeup_period_*1000)) break;
  };
  thread_count_.UnregisterThread();
}

bool ARexService::RegistrationCollector(Arc::XMLNode &doc) {
  //Arc::XMLNode root = infodoc_.Acquire();
  logger_.msg(Arc::VERBOSE,"Passing service's information from collector to registrator");
  Arc::XMLNode empty(ns_, "RegEntry");
  empty.New(doc);

  doc.NewChild("SrcAdv");
  doc.NewChild("MetaSrcAdv");

  doc["SrcAdv"].NewChild("Type") = "org.nordugrid.execution.arex";
  doc["SrcAdv"].NewChild("EPR").NewChild("Address") = endpoint_;
  //doc["SrcAdv"].NewChild("SSPair");

  return true;
  //
  // TODO: filter information here.
  //Arc::XMLNode regdoc("<Service/>");
  //regdoc.New(doc);
  //doc.NewChild(root);
  //infodoc_.Release();
}

std::string ARexService::getID() {
  return "ARC:AREX";
}

static void GetGlueStates(Arc::XMLNode infodoc,std::map<std::string,std::string>& states) {
  std::string path = "Domains/AdminDomain/Services/ComputingService/ComputingEndpoint/ComputingActivities/ComputingActivity";
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
    Arc::XMLNode state_node = (*node)["State"];
    for(;(bool)state_node;++state_node) {
      std::string state  = (std::string)state_node;
      if(state.empty()) continue;
      // Look for nordugrid prefix
      if(strncmp("nordugrid:",state.c_str(),10) == 0) {
        // Remove prefix
        state.erase(0,10);
        // Store state under id
        states[id] = state;
      };
    };
  };
}

}
