#include <sstream>
#include <fstream>
#include <unistd.h>

#include <arc/Run.h>

#include "ldif/LDIFtoXML.h"
#include "config/ngconfig.h"
#include "grid-manager/conf/environment.h"
#include "arex.h"

namespace ARex {

void ARexService::InformationCollector(void) {
  nordugrid_config_loc=gmconfig_;
std::cerr<<"GM configuration file: "<<nordugrid_config_loc<<std::endl;
  read_env_vars();
  for(;;) {
    std::string lrms;
    std::list<std::string> queues;

    // Parse configuration to construct command lines for information providers
    std::ifstream f(nordugrid_config_loc.c_str());
    if(!f) {
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
        logger_.msg(Arc::DEBUG,"Configuration: Section: %s, Subsection: %s",section.c_str(),subsection.c_str());
        if((section == "common") || (section == "grid-manager")) {
          std::list<std::string> values = g->FindOptionValue("lrms");
          if(values.size() > 0) lrms=*(values.begin());
          std::string::size_type p = lrms.find(' ');
          if(p != std::string::npos) lrms.resize(p);
          logger_.msg(Arc::DEBUG,"Configuration: LRMS: %s",lrms.c_str());
        } else if(section == "queue") {
          std::string queue = g->GetID();
          if(queue.empty()) queue=subsection;
          logger_.msg(Arc::DEBUG,"Configuration: Queue: %s",queue.c_str());
          if(!queue.empty()) queues.push_back(queue);
        };
      };
    } catch(ConfigError& e) {
    };
    // Run information provider
    std::string ldif_str;
    {
      std::string cmd;
      cmd=nordugrid_libexec_loc+"/cluster-"+lrms+".pl -valid-to 120 -config "+nordugrid_config_loc+" -dn o=grid -l 2";
      std::cerr<<"cmd: "<<cmd<<std::endl;
      Arc::Run run(cmd);
      std::string stdin_str;
      std::string stderr_str;
      run.AssignStdin(stdin_str);
      run.AssignStdout(ldif_str);
      run.AssignStderr(stderr_str);
      if(!run.Start()) {
      };
      if(!run.Wait(60)) {
      };
      int r = run.Result();
      logger_.msg(Arc::DEBUG,"Cluster information provider result: %i",r);
      logger_.msg(Arc::DEBUG,"Cluster information provider error: %s",stderr_str.c_str());
    };
    for(std::list<std::string>::iterator q = queues.begin();q!=queues.end();++q) {
      std::string cmd;
      cmd=nordugrid_libexec_loc+"/queue+jobs+users-"+lrms+".pl -valid-to 120 -config "+nordugrid_config_loc+" -dn nordugrid-queue-name="+(*q)+",o=grid -queue "+(*q)+" -l 2";
      std::cerr<<"cmd: "<<cmd<<std::endl;
      Arc::Run run(cmd);
      std::string stdin_str;
      std::string stderr_str;
      run.AssignStdin(stdin_str);
      run.AssignStdout(ldif_str);
      run.AssignStderr(stderr_str);
      if(!run.Start()) {
      };
      if(!run.Wait(60)) {
      };
      int r = run.Result();
      logger_.msg(Arc::DEBUG,"Queue information provider result: %i",r);
      logger_.msg(Arc::DEBUG,"Queue information provider error: %s",stderr_str.c_str());
    };
    logger_.msg(Arc::DEBUG,"Obtained LDIF: %s",ldif_str.c_str());
    // Convert result to XML
    std::istringstream ldif(ldif_str);
    Arc::NS ns;
    ns["n"]="urn:nordugrid";
    Arc::XMLNode root(ns,"n:nordugrid");
    if(LDIFtoXML(ldif,"o=grid",root)) {
      // Put result into container
      infodoc_.Assign(root);
      logger_.msg(Arc::INFO,"Assigned new informational document");
      root.GetXML(ldif_str);
      logger_.msg(Arc::DEBUG,"Obtained XML: %s",ldif_str.c_str());
    } else {
      logger_.msg(Arc::ERROR,"Failed to create informational document");
    };
    sleep(60);
  };
}

}

