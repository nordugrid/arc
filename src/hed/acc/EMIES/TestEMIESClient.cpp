// -*- indent-tabs-mode: nil -*-
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/UserConfig.h>
#include <arc/client/Job.h>
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

static void usage(void) {
  std::cout<<"test-emies-client sstat <ResourceInfo URL>"<<std::endl;
  std::cout<<"test-emies-client submit <Creation URL> <ADL file>"<<std::endl;
  std::cout<<"test-emies-client stat <ActivityManagement URL> <Job ID>"<<std::endl;
  std::cout<<"test-emies-client info <ActivityManagement URL> <Job ID>"<<std::endl;
  std::cout<<"test-emies-client clean <ActivityManagement URL> <Job ID>"<<std::endl;
  std::cout<<"test-emies-client kill <ActivityManagement URL> <Job ID>"<<std::endl;
  std::cout<<"test-emies-client list <ActivityInfo URL>"<<std::endl;
  exit(1);
}

void print(const EMIESJob& job) {
  std::cout<<"ID:        "<<job.id<<std::endl;
  std::cout<<"manager:   "<<job.manager.fullstr()<<std::endl;
  std::cout<<"stagein:   "<<job.stagein.fullstr()<<std::endl;
  std::cout<<"session:   "<<job.session.fullstr()<<std::endl;
  std::cout<<"stageout:  "<<job.stageout.fullstr()<<std::endl;
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

int main(int argc, char* argv[]) {
  Arc::LogStream logcerr(std::cerr);
  logcerr.setFormat(Arc::ShortFormat);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);

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
    EMIESJob job;
    EMIESJobState state;
    if(!ac.submit(adls,job,state,false)) {
      logger.msg(ERROR,"Submission failed");
      return 1;
    };
    print(job);
    print(state);
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
    std::cout<<"stagein:   "<<job.stagein.fullstr()<<std::endl;
    std::cout<<"session:   "<<job.session.fullstr()<<std::endl;
    std::cout<<"stageout:  "<<job.stageout.fullstr()<<std::endl;
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
  } else {
    logger.msg(ERROR,"Unsupported command: %s",command);
    return 1;
  } 

  return 0;
}


