#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <list>

#include <arc/OptionParser.h>
#include <arc/Thread.h>
#include <arc/Logger.h>
#include <arc/Utils.h>
#include <arc/UserConfig.h>
#include <arc/data/DataHandle.h>

#include "conf/GMConfig.h"
#include "files/ControlFileContent.h"
#include "jobs/JobDescriptionHandler.h"
#include "misc/proxy.h"

static Arc::SimpleCondition cond;
static Arc::Logger logger(Arc::Logger::rootLogger, "inputcheck");

class lfn_t {
 public:
  std::string lfn;
  bool done;
  bool failed;
  lfn_t(const std::string& l):lfn(l),done(false),failed(false) { };
};

void check_url(void *arg) {
  lfn_t* lfn = (lfn_t*)arg;
  logger.msg(Arc::INFO,"%s",lfn->lfn);

  Arc::UserConfig usercfg;
  Arc::DataHandle source(Arc::URL(lfn->lfn),usercfg);
  source->SetSecure(false);
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
      if(source->CurrentLocationHandle()->Check(false).Passed()) {
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

int main(int argc,char* argv[]) {

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Arc::OptionParser options(istring("job_description_file [proxy_file]"),
                            istring("inputcheck checks that input files specified "
                                "in the job description are available and accessible "
                                "using the credentials in the given proxy file."));
  std::string debug;
  options.AddOption('d', "debug",
                    istring("FATAL, ERROR, WARNING, INFO, VERBOSE or DEBUG"),
                    istring("debuglevel"), debug);

  std::list<std::string> params = options.Parse(argc, argv);

  if (!debug.empty()) Arc::Logger::getRootLogger().setThreshold(Arc::istring_to_level(debug));
  if (params.size() != 1 && params.size() != 2) {
    logger.msg(Arc::ERROR, "Wrong number of arguments given");
    return -1;
  }
  std::string desc = params.front();
  std::string proxy;
  if (params.size() == 2) proxy = params.back();

  // TODO It would be better to use Arc::JobDescription::Parse(desc)
  ARex::GMConfig config;
  ARex::JobDescriptionHandler job_desc_handler(config);
  ARex::JobLocalDescription job;
  Arc::JobDescription arc_job_desc;

  if(job_desc_handler.parse_job_req_from_file(job,arc_job_desc,desc) != ARex::JobReqSuccess) return 1;

  if(!proxy.empty()) {
    Arc::SetEnv("X509_USER_PROXY",proxy,true);
    Arc::SetEnv("X509_USER_CERT",proxy,true);
    Arc::SetEnv("X509_USER_KEY",proxy,true);
  };
  ARex::prepare_proxy();
  
  std::list<ARex::FileData>::iterator file;
  bool has_lfns = false;
  std::list<lfn_t*> lfns;
  for(file=job.inputdata.begin();file!=job.inputdata.end();++file) {
    if(file->has_lfn()) {
      lfn_t* lfn = new lfn_t(file->lfn);
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
          ARex::remove_proxy();
          exit(1);
        };
      } else {
        has_lfns=true; 
      };
    };
  };
  ARex::remove_proxy();
  exit(0);
}
