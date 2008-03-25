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
    std::string ldif_str;
    {
      std::string cmd;
      cmd=nordugrid_libexec_loc+"/cluster.pl -valid-to 120 -config "+nordugrid_config_loc+" -dn o=grid -l 0";
      Arc::Run run(cmd);
      std::string stdin_str;
      std::string stderr_str;
      run.AssignStdin(stdin_str);
      run.AssignStdout(ldif_str);
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
    for(std::list<std::string>::iterator q = queues.begin();q!=queues.end();++q) {
      std::string cmd;
      cmd=nordugrid_libexec_loc+"/qju.pl -valid-to 120 -config "+nordugrid_config_loc+" -dn nordugrid-queue-name="+(*q)+",o=grid -queue "+(*q)+" -l 0";
      Arc::Run run(cmd);
      std::string stdin_str;
      std::string stderr_str;
      run.AssignStdin(stdin_str);
      run.AssignStdout(ldif_str);
      run.AssignStderr(stderr_str);
      logger_.msg(Arc::DEBUG,"Queue information provider: %s",cmd);
      if(!run.Start()) {
      };
      if(!run.Wait(60)) {
      };
      int r = run.Result();
      logger_.msg(Arc::DEBUG,"Queue information provider result: %i",r);
      logger_.msg(Arc::DEBUG,"Queue information provider error: %s",stderr_str);
    };
    logger_.msg(Arc::DEBUG,"Obtained LDIF: %s",ldif_str);
    // Convert result to XML
    std::istringstream ldif(ldif_str);
    Arc::NS ns;
    ns["n"]="urn:nordugrid";
    Arc::XMLNode root(ns,"n:nordugrid");
    if(LDIFtoXML(ldif,"o=grid",root)) {
      // Put result into container
      infodoc_.Assign(root,true);
      logger_.msg(Arc::INFO,"Assigned new informational document");
      root.GetXML(ldif_str);
      logger_.msg(Arc::DEBUG,"Obtained XML: %s",ldif_str);
    } else {
      logger_.msg(Arc::ERROR,"Failed to create informational document");
    };
    sleep(60);
  };
}

}

