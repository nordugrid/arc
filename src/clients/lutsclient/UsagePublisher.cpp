#include "arexlutsclient.h"
#include "UsagePublisher.h"
#include "ArexLogParser.h"

//TODO cross-platform
#include <dirent.h>
#include <signal.h>
#include <errno.h>

#include <sstream>

namespace Arc
{

  /** Constructor. Pass reference to root of Config XML and Logger object.
   *  These must exist at least until this object does!
   */
  UsagePublisher::UsagePublisher(Config &_config, Logger &_logger):
    config(_config), logger(_logger), lutsclient(_config),
    usagerecordset(Arc::NS("","http://schema.ogf.org/urf/2003/09/urf"),
		   "UsageRecords")
  {
    // Default values:
    arex_joblog_dir = AREXLUTSCLIENT_DEFAULT_JOBLOG_DIR;
    max_ur_set_size = AREXLUTSCLIENT_DEFAULT_MAX_UR_SET_SIZE;

    // Values from config:

    if (Arc::XMLNode lutsclientconfig=config["LutsClient"])
      {
	Arc::XMLNode joblog_dir_node=lutsclientconfig["ArexLogDir"];
	Arc::XMLNode max_ur_set_size_node=lutsclientconfig["MaxURSetSize"];
	Arc::XMLNode reporting_interval_node=
	  lutsclientconfig["ReportingInterval"];

	if (joblog_dir_node)
	  arex_joblog_dir=(std::string)joblog_dir_node;

	if (max_ur_set_size_node)
	  {
	    std::istringstream stream((std::string)max_ur_set_size_node);
	    if (!(stream>>max_ur_set_size))
	      {
		logger.msg(Arc::ERROR, 
			   "Could not parse integer value \"MaxURSetSize\" "
			   " in config file %s",
			   config.getFileName().c_str()
			   );
	      }
	  }

      }
    logger.msg(Arc::VERBOSE, "A-REX job log dir is: %s",
	       arex_joblog_dir.c_str());
    logger.msg(Arc::VERBOSE, "Maximal UR Set size is: %d",
	       max_ur_set_size);

  }

  /**
   * Generate URs and publish them in sets to LUTS server.
   */
  int UsagePublisher::publish()
  {

    //Collect jobids and corresponding hash lists from log dir
    //(to know where to get usage data from)
    jobs.clear();
    DIR *dirp;
    dirent *entp;
    errno=0;
    if ( (dirp=opendir(arex_joblog_dir.c_str()))==NULL )
      {
	logger.msg(Arc::ERROR, 
		   "Could not open log directory \"%s\": %s",
		   arex_joblog_dir.c_str(),
		   strerror(errno)
		   );
	return -1;
      }
    // Seek "<jobnumber>.<hash>" files. 
    // jobnumber may repeat with different hash.
    // Put hashes pertaining to a given job number into a list.
    Arc::RegularExpression logfilepattern("^[0-9]+\\.[^.]+$");
    errno = 0;
    while ((entp = readdir(dirp)) != NULL) 
      {
	if (logfilepattern.match(entp->d_name))
	  {
	    std::string fname=entp->d_name;
	    int dotp=fname.find('.');  // MUST CONTAIN exactly 1 dot
	    std::string jobnum=fname.substr(0, dotp);
	    std::string hash=fname.substr(dotp+1, std::string::npos);
	    /*logger.msg(Arc::VERBOSE, "job number: %s hash: %s",
	      jobnum.c_str(),
	      hash.c_str()
	      );*/
	    //Insert:
	    jobs[jobnum].push_back(hash);
	  }
	errno = 0;
      }

    if (errno!=0)
      {
	logger.msg(Arc::ERROR, 
		   "Error reading log directory \"%s\": %s",
		   arex_joblog_dir.c_str(),
		   strerror(errno)
		   );
	if (jobs.empty())
	  {
	    logger.msg(Arc::WARNING, 
		       "No log files can be opened"
		       );
	    return -2;
	  }
      }

    //Fill the set with the given max. number of records,
    //(each corresponding to a job),
    //then log it in the LUTS, just like the original JARM
    Arc::ArexLogParser logparser;

    //Clear UR set
    usagerecordset.Replace(
	Arc::XMLNode(Arc::NS("",
			     "http://schema.ogf.org/urf/2003/09/urf"
			     ),
		     "UsageRecords")
			   );
    urn=0;

    deletable_jobs.clear();  // here we collect jobs that have been 
                             // successfully logged
    for (jobmap_t::iterator jp=jobs.begin();
	 jp!=jobs.end();
	 jp++
	 )
      {
	//Go through logs corresponding to this job, and parse them
	for(hashlist_t::iterator hp=(*jp).second.begin();
	    hp!=(*jp).second.end();
	    hp++
	    )
	  {
	    //TODO there must be a nicer way:
	    std::string fname=arex_joblog_dir+"/"+(*jp).first+"."+(*hp);
	    int nl=logparser.parse( fname.c_str() );

	    if (nl==0)
	      logger.msg(Arc::WARNING, 
			 "No lines could be parsed in file %s",
			 fname.c_str()
			 );
	  }
      
	//create UR if can
	try
	  {
	    Arc::XMLNode usagerecord=logparser.createUsageRecord();
	    usagerecordset.NewChild(usagerecord);
	    urn++;
	    //will delete logs if LUTS-logging is successful
	    deletable_jobs.push_back((*jp).first); 
	  }
	catch (std::runtime_error e)
	  {
	    logger.msg(Arc::WARNING, 
		       "Could not create UR for job %s: %s",
		       (*jp).first.c_str(),
		       e.what()
		       );
	  }
	logparser.clear();

	if (urn==max_ur_set_size)
	  {
	    //No more URs in this set
	    //Connect LUTS and send usage record set.
	    //If succeeded, delete logs.
	    log_and_delete();

	    //Clear UR set (for the next run of UR creation & set filling)
	    usagerecordset.Replace(
	      Arc::XMLNode(Arc::NS("",
				   "http://schema.ogf.org/urf/2003/09/urf"
				   ),
			   "UsageRecords")
				   );
	    urn=0;

	    deletable_jobs.clear();

	  
	  }
      } // for jp
  
    //Send remaining
    if (urn>0)
      {
	//Connect LUTS, send usage record set.
	//If succeeded, delete logs.
	log_and_delete();
      
      }


  }

  /**
   * Logs usage record set in LUTS, and if succeeded, deletes
   * log files corresponding to the logged jobs
   */
  int UsagePublisher::log_and_delete()
  {
    std::string urstr;
    usagerecordset.GetDoc(urstr,false);

    logger.msg(Arc::INFO, 
	       "Logging UR set of %d URs.",
	       urn);
    logger.msg(Arc::VERBOSE, 
	       "UR set dump: %s",
	       urstr.c_str());
  
    Arc::MCC_Status status=lutsclient.log_urs(urstr);
    if (status.isOk())
      {
	// Delete log files
	for (joblist_t::iterator jp=deletable_jobs.begin();
	     jp!=deletable_jobs.end();
	     jp++
	     )
	  {
	    hashlist_t &hashes=jobs[*jp];
	    for (hashlist_t::iterator hp=hashes.begin();
		 hp!=hashes.end();
		 hp++
		 )
	      {
		//TODO there must be a nicer way:
		std::string fname=arex_joblog_dir+"/"+(*jp)+"."+(*hp);
		logger.msg(Arc::INFO, 
			   "Deleting %s",
			   fname.c_str()
			   );
		errno=0;
		if (unlink(fname.c_str()) != 0)
		  {
		    logger.msg(Arc::ERROR, 
			       "Failed to delete %s: %s",
			       fname.c_str(),
			       strerror(errno)
			       );
		  }

	      }
	  
	  }

	return 0;
      }
    else // status.isnotOk
      {
	logger.msg(Arc::ERROR, 
		   "%s: %s",
		   status.getOrigin().c_str(),
		   status.getExplanation().c_str()
		   );
	return -1;
      }
  }

}
