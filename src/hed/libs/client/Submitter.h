#ifndef __ARC_SUBMITTER_H__
#define __ARC_SUBMITTER_H__

#include <string>

#include <arc/client/ACC.h>
#include <arc/client/JobDescription.h>
#include <arc/Logger.h>
#include <arc/URL.h>

namespace Arc {

  class Config;

  class Submitter
    : public ACC {
  protected:
    Submitter(Config *cfg);
  public:
    virtual ~Submitter();
    virtual std::pair<URL, URL> Submit(Arc::JobDescription& jobdesc) = 0;
    bool putFiles(const std::vector< std::pair< std::string, std::string> >& fileList, std::string jobid);
    
  protected:
    URL SubmissionEndpoint;
    URL InfoEndpoint;
    std::string MappingQueue;
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_SUBMITTER_H__
