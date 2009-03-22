#include "arexlutsclient.h"

//TODO cross-platform
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>

#include <string>
#include <sstream>
#include <map>
#include <list>
#include <iostream>

#include <arc/Logger.h>
#include <arc/ArcRegex.h>
#include <arc/XMLNode.h>

#ifdef WIN32
#include <arc/win32.h>
#endif

#include "ArexLogParser.h"
#include "LUTSClient.h"


/**
 * Logs usage record set in LUTS, and if succeeded, deletes
 * log files corresponding to the logged jobs
 */
inline static int log_and_delete(Arc::LUTSClient &lc, 
				 Arc::XMLNode &urset,
				 int urn,
				 Arc::Logger &logger,
				 jobmap_t &jobs,
				 joblist_t &deljobs,
				 std::string &logdir
				 )
{
  std::string urstr;
  urset.GetDoc(urstr,false);
  logger.msg(Arc::INFO, 
	     "Logging UR set of %d URs: %s",
	     urn, urstr.c_str()
	     );
  
  Arc::MCC_Status status=lc.log_urs(urstr);
  if (status.isOk())
    {
      // Delete log files
      for (joblist_t::iterator jp=deljobs.begin();
	   jp!=deljobs.end();
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
	      std::string fname=logdir+"/"+(*jp)+"."+(*hp);
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



int main(int argc, char **argv)
{

  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);

  //Initialize logger
  Arc::Logger logger(Arc::Logger::rootLogger, "Lutsclient");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);
  logger.msg(Arc::INFO, 
    "Accounting agent (Lutsclient) started.");

  //Set up client environment

  // First command line argument, required, is the config file name
  if (argc<2)
    {
      logger.msg(Arc::FATAL, 
		 "Configuration file path must be given in"
		 " first command line argument!");
      return -4;
    }
  std::string config_file=argv[1];
  logger.msg(Arc::VERBOSE, "Reading configuration file: %s",
	     config_file.c_str());
  Arc::Config config(config_file.c_str());

  // Default values:
  std::string arex_joblog_dir = AREXLUTSCLIENT_DEFAULT_JOBLOG_DIR;
  int max_ur_set_size = AREXLUTSCLIENT_DEFAULT_MAX_UR_SET_SIZE;
  int reporting_interval = AREXLUTSCLIENT_DEFAULT_REPORTING_INTERVAL;

  // Values from config:

  if (Arc::XMLNode lutsclientconfig=config["LutsClient"])
    {
      Arc::XMLNode logger_node=lutsclientconfig["Logger"];
      Arc::XMLNode joblog_dir_node=lutsclientconfig["ArexLogDir"];
      Arc::XMLNode max_ur_set_size_node=lutsclientconfig["MaxURSetSize"];
      Arc::XMLNode reporting_interval_node=
	lutsclientconfig["ReportingInterval"];

      // try logging to same file as server if no separate log file given
      if (!logger_node) logger_node=config["Server"]["Logger"];
      
      if (logger_node)
	{
	  
	  // (stolen from hed daemon's init_logger function)
	  Arc::LogStream* sd = NULL; 
	  std::string log_file = (std::string)logger_node;
	  std::string levelstr = (std::string)logger_node.Attribute("level");
	  if(!levelstr.empty()) {
	    Arc::LogLevel level = Arc::string_to_level(levelstr);
	    Arc::Logger::rootLogger.setThreshold(level); 
	  }
	  if(!log_file.empty()) {
	    std::fstream *dest = new std::fstream(log_file.c_str(), 
						  std::fstream::out | 
						  std::fstream::app
						  );
	    if(!(*dest)) {
	      logger.msg(Arc::FATAL,"Failed to open log file: %s",log_file);
	      return -5;
	    }
	    sd = new Arc::LogStream(*dest); 
	  }
	  logger.msg(Arc::VERBOSE,"Adding log destination: %s",log_file);
	  if(sd) Arc::Logger::rootLogger.addDestination(*sd);
	  
	}

      if (joblog_dir_node)
	arex_joblog_dir=(std::string)joblog_dir_node;

      if (max_ur_set_size_node)
	{
	  std::istringstream stream((std::string)max_ur_set_size_node);
	  if (!(stream>>max_ur_set_size))
	    {
	      logger.msg(Arc::FATAL, 
             "Could not parse integer value \"MaxURSetSize\" in config file %s",
			 config_file.c_str()
			 );
	      return -3;
	    }
	}

      if (reporting_interval_node)
	{
	  std::istringstream stream((std::string)reporting_interval_node);
	  if (!(stream>>reporting_interval))
	    {
	      logger.msg(Arc::FATAL, 
             "Could not parse integer value \"ReportingInterval\" in config file %s",
			 config_file.c_str()
			 );
	      return -3;
	    }
	}
    }
  logger.msg(Arc::VERBOSE, "A-REX job log dir is: %s",
	     arex_joblog_dir.c_str());
  logger.msg(Arc::VERBOSE, "Maximal UR Set size is: %d",
	     max_ur_set_size);
  logger.msg(Arc::VERBOSE, "Reporting interval is: %d s",
	     reporting_interval);

  //Client for LUTS:
  Arc::LUTSClient lutsclient(config);

  //Collect jobids and corresponding hash lists from log dir
  //(to know where to get usage data from)
  jobmap_t jobs;
  DIR *dirp;
  dirent *entp;
  errno=0;
  if ( (dirp=opendir(arex_joblog_dir.c_str()))==NULL )
    {
      logger.msg(Arc::FATAL, 
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
      logger.msg(Arc::FATAL, 
		 "No log files can be opened"
		 );
      return -2;
    }
  }


  //Now create usage record sets with root element UsageRecords
  Arc::XMLNode usagerecordset(Arc::NS("",
				      "http://schema.ogf.org/urf/2003/09/urf"),
			      "UsageRecords"
			      );
  //Fill the set with the given max. number of records,
  //(each corresponding to a job),
  //then log it in the LUTS, just like the original JARM
  Arc::ArexLogParser logparser;
  long ur_set_size=0;
  joblist_t deletable_jobs;  // the logs of which will be deleted
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
	  ur_set_size++;
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

      if (ur_set_size==max_ur_set_size)
	{
	  //No more URs in this set
	  //Connect LUTS and send usage record set.
	  //If succeeded, delete logs.
	  log_and_delete(lutsclient, 
			 usagerecordset,
			 ur_set_size,
			 logger,
			 jobs,
			 deletable_jobs,
			 arex_joblog_dir);

	  //Clear UR set (for the next run of UR creation & set filling)
	  usagerecordset.Replace(
		           Arc::XMLNode(
			     Arc::NS("",
				     "http://schema.ogf.org/urf/2003/09/urf"
				     ),
			     "UsageRecords"
					)
				 );
	  ur_set_size=0;
	  deletable_jobs.clear();

	  
	}
    } // for jp
  
  //Send remaining
  if (ur_set_size>0)
    {
      //Connect LUTS, send usage record set.
      //If succeeded, delete logs.
      log_and_delete(lutsclient, 
		     usagerecordset,
		     ur_set_size,
		     logger,
		     jobs,
		     deletable_jobs,
		     arex_joblog_dir);
      
    }
  
  logger.msg(Arc::INFO, 
	     "Finished logging Usage Records,"
	     " sleeping for %d seconds before exit.",
	     reporting_interval);
  sleep(reporting_interval);
  logger.msg(Arc::INFO, "Exiting.");
  
  return 0;
}
