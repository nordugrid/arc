#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Thread.h>
#include <arc/client/JobDescription.h>
#include <arc/client/Submitter.h>
#include <arc/data/DataCache.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataHandle.h>
#include <arc/data/URLMap.h>


namespace Arc {

  Logger Submitter::logger(Logger::getRootLogger(), "Submitter");

  Submitter::Submitter(Config *cfg)
    : ACC(cfg) {
    submissionEndpoint = (std::string)(*cfg)["Endpoint"];
    infoEndpoint = (std::string)(*cfg)["Source"];
    queue = (std::string)(*cfg)["MappingQueue"];
  }

  Submitter::~Submitter() {}

  bool Submitter::PutFiles(JobDescription& jobdesc, const URL& url) {

    DataCache cache;
    DataMover mover;
    mover.retry(true);
    mover.secure(false);
    mover.passive(true);
    mover.verbose(false);

    std::vector<std::pair<std::string, std::string> > fileList =
      jobdesc.getUploadableFiles();

    std::vector<std::pair<std::string, std::string> >::iterator file;

    for (file = fileList.begin(); file != fileList.end(); file++) {
      std::string src = file->second;
      std::string dst = url.str() + '/' + file->first;
      DataHandle source(src);
      DataHandle destination(dst);
      
      std::string failure;
      int timeout = 300;
      if (!mover.Transfer(*source, *destination, cache, URLMap(),
			  0, 0, 0, timeout, failure)) {
	if (!failure.empty())
	  logger.msg(ERROR, "Failed uploading file: %s", failure); 
	else
	  logger.msg(ERROR, "Failed uploading file");
	return false;
      } 
    }
    
    return true;
  }

} // namespace Arc
