#include <arc/ArcConfig.h>
#include <arc/client/Submitter.h>

#include <glibmm/miscutils.h>

#include <arc/Thread.h>
#include <arc/data/DMC.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataPoint.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataCache.h>
#include <arc/data/URLMap.h>


namespace Arc {

  Logger Submitter::logger(Logger::getRootLogger(), "Submitter");

  Submitter::Submitter(Config *cfg)
    : ACC() {
    SubmissionEndpoint = (std::string)(*cfg)["Endpoint"];
    InfoEndpoint = (std::string)(*cfg)["Source"];
    MappingQueue = (std::string)(*cfg)["MappingQueue"];
  }

  Submitter::~Submitter() {}
  
  static void progress(FILE *o, const char *, unsigned int,
		       unsigned long long int all, unsigned long long int max,
		       double, double) {
    static int rs = 0;
    const char rs_[4] = {'|', '/', '-', '\\'};
    if (max) {
      fprintf(o, "\r|");
      unsigned int l = (74 * all + 37) / max;
      if (l > 74) l = 74;
      unsigned int i = 0;
      for (; i < l; i++) fprintf(o, "=");
      fprintf(o, "%c", rs_[rs++]);
      if (rs > 3) rs = 0;
      for (; i < 74; i++) fprintf(o, " ");
      fprintf(o, "|\r");
      fflush(o);
      return;
    }
    fprintf(o, "\r%llu kB                    \r", all / 1024);
  }

  bool Submitter::putFiles(const std::vector< std::pair< std::string, std::string> >& fileList, std::string jobid){
    
    bool uploaded(true);

    logger.msg(DEBUG, "Uploading %d job input files to cluster", fileList.size());
    
    // Create mover
    Arc::DataMover mover;
    mover.retry(true);
    mover.secure(false);
    mover.passive(false);
    mover.verbose(true);
    mover.set_progress_indicator(&progress);
    
    // Create cache
    Arc::User cache_user;
    std::string cache_path2;
    std::string cache_data_path;
    std::string id = "<ngcp>";
    Arc::DataCache cache(cache_path2, cache_data_path, "", id, cache_user);
    std::vector< std::pair< std::string, std::string > >::const_iterator file;
    
    // Loop over files and upload
    for (file = fileList.begin(); file != fileList.end(); file++) {
      std::string src = file->second;
      std::string dst = Glib::build_filename(jobid, file->first);
      std::cout << src << " -> " << dst << std::endl;
      Arc::DataHandle source(src);
      Arc::DataHandle destination(dst);
      
      std::string failure;
      int timeout = 300;
      if (!mover.Transfer(*source, *destination, cache, Arc::URLMap(), 0, 0, 0, timeout, failure)) {
	if (!failure.empty()) logger.msg(ERROR, "Failed uploading file: %s", failure); 
	else logger.msg(ERROR, "Failed uploading file: %s", failure);
	uploaded = false;
      } 
    }
    
    return uploaded;

  } // putFiles()


} // namespace Arc
