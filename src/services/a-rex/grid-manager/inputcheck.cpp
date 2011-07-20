#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <list>

#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/Utils.h>
#include <arc/UserConfig.h>
#include <arc/data/DataHandle.h>

#include "files/info_types.h"
#include "jobs/job_request.h"
#include "misc/proxy.h"

static Arc::SimpleCondition cond;

static Arc::Logger logger(Arc::Logger::rootLogger, "inputcheck");

class lfn_t {
 public:
  const char* lfn;
  bool done;
  bool failed;
  lfn_t(const char* l):lfn(l),done(false),failed(false) { };
};

void check_url(void *arg) {
  lfn_t* lfn = (lfn_t*)arg;
  logger.msg(Arc::INFO,"%s",lfn->lfn);

  Arc::UserConfig usercfg;
  Arc::DataHandle source(Arc::URL(lfn->lfn),usercfg);
  if(!source) {
    logger.msg(Arc::ERROR,"Failed to acquire source: %s",lfn->lfn);
    lfn->failed=true; lfn->done=true; cond.signal();
    return;
  };
  if(!source->Resolve(true).Passed()) {
    logger.msg(Arc::ERROR,"Failed to resolve %s",lfn->lfn);
    lfn->failed=true; lfn->done=true; cond.signal();
    return;
  };
  source->SetTries(1);
  // TODO. Run every URL in separate thread.
  // TODO. Do only connection (optionally)
  bool check_passed = false;
  if(source->HaveLocations()) {
    do {
      if(source->Check().Passed()) {
        check_passed=true;
        break;
      }
    } while (source->NextLocation());
  };
  if(!check_passed) {
    logger.msg(Arc::ERROR,"Failed to check %s",lfn->lfn);
    lfn->failed=true; lfn->done=true; cond.signal();
    return;
  };
  lfn->done=true; cond.signal();
  return;
}

static void usage(void) {
  logger.msg(Arc::INFO,"Usage: inputcheck [-h] [-d debug_level] RSL_file [proxy_file]");
}

int main(int argc,char* argv[]) {
  unsigned int n;

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  while((n=getopt(argc,argv,"hd:")) != -1) {
    switch(n) {
      case ':': { logger.msg(Arc::ERROR,"Missing argument"); return -1; };
      case '?': { logger.msg(Arc::ERROR,"Unrecognized option"); return -1; };
      case '.': { return -1; };
      case 'h': {
        usage();
        return 0;
      };
      case 'd': {
        Arc::Logger::getRootLogger().setThreshold(Arc::string_to_level(optarg));
      }; break;
      default: { logger.msg(Arc::ERROR,"Option processing error"); return -1; };
    };
  };
  if(optind >= argc) {
    usage();
    return -1;
  };
  std::string rsl = argv[optind];
  const char* proxy = NULL;
  if((optind+1) < argc) proxy=argv[optind+1];
  JobLocalDescription job;

  if(parse_job_req(rsl,job) != JobReqSuccess) return 1;

  if(proxy) {
    Arc::SetEnv("X509_USER_PROXY",proxy,true);
    Arc::SetEnv("X509_USER_CERT",proxy,true);
    Arc::SetEnv("X509_USER_KEY",proxy,true);
  };
  prepare_proxy();
  
  std::list<FileData>::iterator file;
  bool has_lfns = false;
  std::list<lfn_t*> lfns;
  for(file=job.inputdata.begin();file!=job.inputdata.end();++file) {
    if(file->has_lfn()) {
      lfn_t* lfn = new lfn_t(file->lfn.c_str());
      lfns.push_back(lfn);
      Arc::CreateThreadFunction(&check_url,lfn);
      has_lfns=true;
    };
  };
  for(;has_lfns;) {
    cond.wait();
    has_lfns=false;
    for(std::list<lfn_t*>::iterator l = lfns.begin();l!=lfns.end();++l) {
      if((*l)->done) {
        if((*l)->failed) {
          remove_proxy();
          exit(1);
        };
      } else {
        has_lfns=true; 
      };
    };
  };
  remove_proxy();
  exit(0);
}


