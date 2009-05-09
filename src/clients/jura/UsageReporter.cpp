#include "jura.h"
#include "UsageReporter.h"
#include "JobLogFile.h"

//TODO cross-platform
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sstream>

#include "arc/ArcRegex.h"

namespace Arc
{

  /** Constructor. Pass name of directory containing job log files
   *  and expiration time of files in seconds.
   */
  UsageReporter::UsageReporter(std::string job_log_dir_, time_t expiration_time_):
    logger(Arc::Logger::rootLogger, "JURA.UsageReporter"),
    job_log_dir(job_log_dir_),
    expiration_time(expiration_time_)
  {
    logger.msg(Arc::INFO, "Initialised, job log dir: %s",
	       job_log_dir.c_str());
    logger.msg(Arc::DEBUG, "Expiration time: %d seconds",
	       expiration_time);
    //Collection of logging destinations:
    dests=new Arc::Destinations();
  }

  /**
   *  Parse usage data and publish it via the appropriate destination adapter.
   */
  int UsageReporter::report()
  {
    //Collect job log file names from job log dir
    //(to know where to get usage data from)
    DIR *dirp;
    dirent *entp;
    errno=0;
    if ( (dirp=opendir(job_log_dir.c_str()))==NULL )
      {
	logger.msg(Arc::ERROR, 
		   "Could not open log directory \"%s\": %s",
		   job_log_dir.c_str(),
		   strerror(errno)
		   );
	return -1;
      }

    // Seek "<jobnumber>.<randomstring>" files.
    Arc::RegularExpression logfilepattern("^[0-9]+\\.[^.]+$");
    errno = 0;
    while ((entp = readdir(dirp)) != NULL)
      {
	if (logfilepattern.match(entp->d_name))
	  {
	    //Parse log file
	    Arc::JobLogFile *logfile;
	    //TODO handle DOS-style path separator!
	    std::string fname=job_log_dir+"/"+entp->d_name;
	    logfile=new Arc::JobLogFile(fname);

	    // Check creation time and remove it if really too old
	    if( expiration_time>0 && logfile->olderThan(expiration_time) )
	      {
		logger.msg(Arc::INFO,
			   "Removing outdated job log file %s",
			   logfile->getFilename().c_str()
			   );
		logfile->remove();
	      } 
	    else
	      {
		//Pass job log file content to the appropriate 
		//logging destination
		dests->report(*logfile);
		//(deep copy performed)
	      }

	    delete logfile;
	  }
	errno = 0;
      }

    if (errno!=0)
      {
	logger.msg(Arc::ERROR, 
		   "Error reading log directory \"%s\": %s",
		   job_log_dir.c_str(),
		   strerror(errno)
		   );
	return -2;
      }
    return 0;
  }
  
  UsageReporter::~UsageReporter()
  {
    delete dests;
    logger.msg(Arc::INFO, "Finished, job log dir: %s",
	       job_log_dir.c_str());
  }
  
}
