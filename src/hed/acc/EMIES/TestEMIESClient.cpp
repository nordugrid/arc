// -*- indent-tabs-mode: nil -*-
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcLocation.h>
#include <arc/UserConfig.h>
#include <arc/compute/Job.h>
#include "EMIESClient.h"

/*
    bool stat(const EMIESJob& job, XMLNode& state);
    bool info(const EMIESJob& job, Job& info);
    bool suspend(const EMIESJob& job);
    bool resume(const EMIESJob& job);
    bool restart(const EMIESJob& job);
*/

using namespace Arc;

static Logger logger(Logger::getRootLogger(), "emiestest");

#define GLUE2_NAMESPACE "http://schemas.ogf.org/glue/2009/03/spec_2.0_r1"

static void usage(void) {
  std::cout<<"arcemiestest sstat <ResourceInfo URL> - retrieve service description"<<std::endl;
  std::cout<<"arcemiestest submit <Creation URL> <ADL file> - submit job to service"<<std::endl;
  std::cout<<"arcemiestest stat <ActivityManagement URL> <Job ID> - check state of job"<<std::endl;
  std::cout<<"arcemiestest info <ActivityManagement URL> <Job ID> - obtain extended description of job"<<std::endl;
  std::cout<<"arcemiestest clean <ActivityManagement URL> <Job ID> - remove job from service"<<std::endl;
  std::cout<<"arcemiestest kill <ActivityManagement URL> <Job ID> - cancel job processing/execution"<<std::endl;
  std::cout<<"arcemiestest list <ActivityInfo URL> - list jobs available at service"<<std::endl;
  std::cout<<"arcemiestest ivalidate <ResourceInfo URL> - retrieve service description using all available methods and validate results"<<std::endl;
  exit(1);
}

void print(const EMIESJob& job) {
  std::cout<<"ID:        "<<job.id<<std::endl;
  std::cout<<"manager:   "<<job.manager.fullstr()<<std::endl;
  for(std::list<URL>::const_iterator s = job.stagein.begin();s!=job.stagein.end();++s) {
    std::cout<<"stagein:   "<<s->fullstr()<<std::endl;
  }
  for(std::list<URL>::const_iterator s = job.session.begin();s!=job.session.end();++s) {
    std::cout<<"session:   "<<s->fullstr()<<std::endl;
  }
  for(std::list<URL>::const_iterator s = job.stageout.begin();s!=job.stageout.end();++s) {
    std::cout<<"stageout:  "<<s->fullstr()<<std::endl;
  }
}

void print(const EMIESJobState& state) {
  std::cout<<"state:     "<<state.state<<std::endl;
  for(std::list<std::string>::const_iterator attr = state.attributes.begin();
                       attr != state.attributes.end();++attr) {
    std::cout<<"attribute: "<<*attr<<std::endl;
  };
}

void print(const std::list<EMIESJob>& jobs) {
  for(std::list<EMIESJob>::const_iterator job = jobs.begin();job != jobs.end();++job) {
    std::cout<<"ID:        "<<job->id<<std::endl;
  };
}

void FillJob(EMIESJob& job, int argc, char* argv[]) {
    if(argc < 4) usage();
    URL url(argv[2]);
    std::string id = argv[3];
    job.manager = url;
    job.id = id;
}

bool CheckComputingService(XMLNode result) {
  if(result.Namespace() != GLUE2_NAMESPACE) {
    logger.msg(ERROR,"Query returned unexpected element: %s:%s",result.Namespace(),result.Name());
    return false;
  };
  if(result.Name() != "ComputingService") {
    logger.msg(ERROR,"Query returned unexpected element: %s:%s",result.Namespace(),result.Name());
    return false;
  };
  std::string errstr;
  std::string glue2_schema = ArcLocation::GetDataDir()+G_DIR_SEPARATOR_S+"schema"+G_DIR_SEPARATOR_S+"GLUE2.xsd";
  if(!result.Validate(glue2_schema,errstr)) {
    logger.msg(ERROR,"Element validation according to GLUE2 schema failed: %s",errstr);
    return false;
  };
  return true;
}

int main(int argc, char* argv[]) {
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::VERBOSE);

  if(argc < 3) usage();

  std::string command(argv[1]);

  URL url(argv[2]);
  if(!url) exit(1);

  std::string conffile;
  Arc::UserConfig usercfg(conffile,initializeCredentialsType::RequireCredentials);
  MCCConfig cfg;
  usercfg.ApplyToConfig(cfg);

  EMIESClient ac(url,cfg,30);

  if(command == "sstat") {
    XMLNode info;
    if(!ac.sstat(info)) {
      logger.msg(ERROR,"Resource query failed");
      return 1;
    };
    std::string s;
    info.GetXML(s,true);
    std::cout<<s<<std::endl;
  } else if(command == "submit") {
    if(argc < 4) usage();
    XMLNode adl;
    adl.ReadFromFile(argv[3]);
    if(!adl) usage();
    std::string adls;
    adl.GetDoc(adls);
    EMIESResponse *response = NULL;
    ac.submit(adls,&response);
    EMIESJob *job = dynamic_cast<EMIESJob*>(response);
    if(!job) {
      delete response;
      logger.msg(ERROR,"Submission failed");
      return 1;
    };
    print(*job);
    print(job->state);
    delete response;
  } else if(command == "stat") {
    EMIESJob job;
    FillJob(job,argc,argv);
    EMIESJobState state;
    if(!ac.stat(job,state)) {
      logger.msg(ERROR,"Obtaining status failed");
      return 1;
    };
    print(job);
    print(state);
  } else if(command == "info") {
    EMIESJob job;
    FillJob(job,argc,argv);
    Job info;
    if(!ac.info(job,info)) {
      logger.msg(ERROR,"Obtaining information failed");
      return 1;
    };
    print(job);
    for(std::list<URL>::const_iterator s = job.stagein.begin();s!=job.stagein.end();++s) {
      std::cout<<"stagein:   "<<s->fullstr()<<std::endl;
    }
    for(std::list<URL>::const_iterator s = job.session.begin();s!=job.session.end();++s) {
      std::cout<<"session:   "<<s->fullstr()<<std::endl;
    }
    for(std::list<URL>::const_iterator s = job.stageout.begin();s!=job.stageout.end();++s) {
      std::cout<<"stageout:  "<<s->fullstr()<<std::endl;
    }
  } else if(command == "clean") {
    EMIESJob job;
    FillJob(job,argc,argv);
    if(!ac.clean(job)) {
      logger.msg(ERROR,"Cleaning failed");
      return 1;
    };
  } else if(command == "notify") {
    EMIESJob job;
    FillJob(job,argc,argv);
    if(!ac.notify(job)) {
      logger.msg(ERROR,"Notify failed");
      return 1;
    };
  } else if(command == "kill") {
    EMIESJob job;
    FillJob(job,argc,argv);
    if(!ac.kill(job)) {
      logger.msg(ERROR,"Kill failed");
      return 1;
    };
  } else if(command == "list") {
    std::list<EMIESJob> jobs;
    if(!ac.list(jobs)) {
      logger.msg(ERROR,"List failed");
      return 1;
    }
    print(jobs);
  } else if(command == "ivalidate") {
    std::map<std::string,std::string> interfaces;
    interfaces["org.ogf.glue.emies.activitycreation"] = "";
    interfaces["org.ogf.glue.emies.activitymanagement"] = "";
    interfaces["org.ogf.glue.emies.activityinfo"] = "";
    interfaces["org.ogf.glue.emies.resourceinfo"] = "";
    interfaces["org.ogf.glue.emies.delegation"] = "";
    logger.msg(INFO,"Fetching resource description from %s",url.str());
    XMLNode info;
    if(!ac.sstat(info,false)) {
      logger.msg(ERROR,"Failed to obtain resource description: %s",ac.failure());
      return 1;
    };
    int n = 0;
    int cnum1 = 0;
    for(;;++n) {
      XMLNode node = info.Child(n);
      if(!node) break;
      if(node.Namespace() != GLUE2_NAMESPACE) {
        logger.msg(ERROR,"Resource description contains unexpected element: %s:%s",node.Namespace(),node.Name());
        return 1;
      };
      if((node.Name() != "ComputingService") && (node.Name() != "Service")) {
        logger.msg(ERROR,"Resource description contains unexpected element: %s:%s",node.Namespace(),node.Name());
        return 1;
      };
      std::string errstr;
      std::string glue2_schema = ArcLocation::GetDataDir()+G_DIR_SEPARATOR_S+"schema"+G_DIR_SEPARATOR_S+"GLUE2.xsd";
      if(!node.Validate(glue2_schema,errstr)) {
        logger.msg(ERROR,"Resource description validation according to GLUE2 schema failed: ");
        for(std::string::size_type p = 0;;) {
          if(p >= errstr.length()) break;
          std::string::size_type e = errstr.find('\n',p);
          if(e == std::string::npos) e = errstr.length();
          logger.msg(ERROR,"%s", errstr.substr(p,e-p));
          p = e + 1;
        };
        return 1;
      };
      if(node.Name() == "ComputingService") {
        ++cnum1;
        XMLNode endpoint = node["ComputingEndpoint"];
        for(;(bool)endpoint;++endpoint) {
          XMLNode name = endpoint["InterfaceName"];
          for(;(bool)name;++name) {
            std::string iname = (std::string)name;
            if((bool)endpoint["URL"]) {
              if(interfaces.find(iname) != interfaces.end()) interfaces[iname] = (std::string)endpoint["URL"];
            };
          };
        };
      };
    };
    if(n == 0) {
      logger.msg(ERROR,"Resource description is empty");
      return 1;
    };
    int inum = 0;
    for(std::map<std::string,std::string>::iterator i = interfaces.begin();
                                    i != interfaces.end(); ++i) {
      if(!i->second.empty()) {
        logger.msg(INFO,"Resource description provides URL for interface %s: %s",i->first,i->second);
        ++inum;
      };
    };
    if(inum == 0) {
      logger.msg(ERROR,"Resource description provides no URLs for interfaces");
      return 1;
    };
    logger.msg(INFO,"Resource description validation passed");

    logger.msg(INFO,"Requesting ComputingService elements of resource description at %s",url.str());
    XMLNodeContainer items;
    bool query_passed = false;
    bool all_elements = false;
    if(!query_passed) {
      logger.msg(INFO,"Performing /Services/ComputingService query");
      if(!ac.squery("/Services/ComputingService",items,false)) {
        logger.msg(INFO,"Failed to obtain resource description: %s",ac.failure());
      } else if(items.Size() <= 0) {
        logger.msg(INFO,"Query returned no elements.");
      } else {
        query_passed = true;
      };
    };
    if(!query_passed) {
      logger.msg(INFO,"Performing /ComputingService query");
      if(!ac.squery("/ComputingService",items,false)) {
        logger.msg(INFO,"Failed to obtain resource description: %s",ac.failure());
      } else if(items.Size() <= 0) {
        logger.msg(INFO,"Query returned no elements.");
      } else {
        query_passed = true;
      };
    };
    if(!query_passed) {
      all_elements = true;
      logger.msg(INFO,"Performing /* query");
      if(!ac.squery("/*",items,false)) {
        logger.msg(INFO,"Failed to obtain resource description: %s",ac.failure());
      } else if(items.Size() <= 0) {
        logger.msg(INFO,"Query returned no elements.");
      } else {
        query_passed = true;
      };
    };
    if(!query_passed) {
      logger.msg(ERROR,"All queries failed");
      return 1;
    };
    // In current implementation we can have different response
    // 1. ComputingService elements inside every Item element (ARC)
    // 2. Content of ComputingService elements inside every Item element (UNICORE)
    // 3. All elements inside every Item element
    // 4. Content of all elements inside every Item element
    int cnum2 = 0;
    for(int n = 0; n < items.Size(); ++n) {
      if((items[n].Size() > 0) && (items[n]["ComputingService"])) {
        // Case 1 and 3.
        for(int nn = 0; nn < items[n].Size(); ++nn) {
          if((all_elements) && (items[n].Name() != "ComputingService")) continue; // case 3
          if(!CheckComputingService(items[n].Child(nn))) return 1;
          ++cnum2;
        };
      } else {
        // Assuming 2 and 4. Because 4 can't be reliably recognised
        // just assume it never happens.
        XMLNode result;
        NS ns("glue2arc",GLUE2_NAMESPACE);
        items[n].New(result);
        result.Namespaces(ns,true,0);
        result.Name(result.NamespacePrefix(GLUE2_NAMESPACE)+":ComputingService");
        if(!CheckComputingService(result)) return 1;
        ++cnum2;
      };
    };
    if(cnum1 != cnum2) {
      logger.msg(ERROR,"Number of ComputingService elements obtained from full document and XPath qury do not match: %d != %d",cnum1,cnum2);
      return 1;
    };
    logger.msg(INFO,"Resource description query validation passed");
  } else {
    logger.msg(ERROR,"Unsupported command: %s",command);
    return 1;
  } 

  return 0;
}


