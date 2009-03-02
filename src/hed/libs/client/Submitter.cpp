#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Thread.h>
#include <arc/client/JobDescription.h>
#include <arc/client/Submitter.h>
#include <arc/data/FileCache.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataHandle.h>
#include <arc/data/URLMap.h>


namespace Arc {

  Logger Submitter::logger(Logger::getRootLogger(), "Submitter");

  Submitter::Submitter(Config *cfg, const std::string& flavour)
    : ACC(cfg, flavour) {
    submissionEndpoint = (std::string)(*cfg)["SubmissionEndpoint"];
    cluster = (std::string)(*cfg)["Cluster"];
    queue = (std::string)(*cfg)["Queue"];
  }

  Submitter::~Submitter() {}

  bool Submitter::PutFiles(const JobDescription& jobdesc, const URL& url) {

    FileCache cache;
    DataMover mover;
    mover.retry(true);
    mover.secure(false);
    mover.passive(true);
    mover.verbose(false);

    //std::vector<std::pair<std::string, std::string> > fileList =
    //  jobdesc.getUploadableFiles();
    std::vector<std::pair<std::string, std::string> > fileList;
    if ( !jobdesc.getUploadableFiles( fileList ) ){
	std::cerr << "No uploadable files." << std::endl;
        return false;
    }

    std::vector<std::pair<std::string, std::string> >::iterator file;

    for (file = fileList.begin(); file != fileList.end(); file++) {
      std::string src = file->second;
      std::string dst = url.str() + '/' + file->first;
      DataHandle source(src);
      source->AssignCredentials(proxyPath, certificatePath,
				keyPath, caCertificatesDir);
      DataHandle destination(dst);
      destination->AssignCredentials(proxyPath, certificatePath,
				     keyPath, caCertificatesDir);

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
