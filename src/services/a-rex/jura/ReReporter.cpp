#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "jura.h"
#include "ReReporter.h"
#include "ApelDestination.h"
#include "LutsDestination.h"

//TODO cross-platform
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>
#include <sstream>
#include <string.h>

#include <arc/ArcRegex.h>
#include <arc/Utils.h>

namespace Arc
{
std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

  /** Constructor. Pass name of directory containing job log files,
   *  expiration time of files in seconds, list of URLs in case of 
   *  interactive mode.
   */
  ReReporter::ReReporter(std::string archivedjob_log_dir_, std::string time_range_,
                               std::vector<std::string> urls_, std::vector<std::string> topics_,
                               std::string vo_filters_):
    logger(Arc::Logger::rootLogger, "JURA.ReReporter"),
    dest(NULL),
    archivedjob_log_dir(archivedjob_log_dir_),
    urls(urls_),
    topics(topics_),
    vo_filters(vo_filters_)
  {
    logger.msg(Arc::INFO, "Initialised, archived job log dir: %s",
               archivedjob_log_dir.c_str());

    time_t currentTime = time(NULL);
    struct tm *aTime = localtime(&currentTime);
    
    start = new tm();
    start->tm_year = aTime->tm_year + 1900;
    start->tm_mon  = aTime->tm_mon + 1;
    start->tm_mday = 1;
    end = new tm();
    end->tm_year   = start->tm_year;
    end->tm_mon    = start->tm_mon;
    end->tm_mday   = 31;
    if (!time_range_.empty())
      {
        logger.msg(Arc::VERBOSE, "Incoming time range: %s", time_range_);
        std::vector<std::string> start_stop = split(time_range_, '-');
        for (unsigned i=0; i<start_stop.size(); i++) {
            std::string element = start_stop.at(i);
            std::vector<std::string> date = split(element, '.');
            switch(i) {
                case 0:
                        if ( date.size() > 2 ) { start->tm_mday = atoi(date[2].c_str()); }
                        if ( date.size() > 1 ) { start->tm_mon  = atoi(date[1].c_str()); }
                        if ( date.size() > 0 ) { start->tm_year = atoi(date[0].c_str()); }
                        break;
                case 1:
                        if ( date.size() > 2 ) { end->tm_mday = atoi(date[2].c_str()); }
                        if ( date.size() > 1 ) { end->tm_mon  = atoi(date[1].c_str()); }
                        if ( date.size() > 0 ) { end->tm_year = atoi(date[0].c_str()); }
                        break;
            } 
        }
      }
    logger.msg(Arc::VERBOSE, "Requested time range: %d.%d.%d-%d.%d.%d ",
               start->tm_year, start->tm_mon, start->tm_mday,
               end->tm_year, end->tm_mon, end->tm_mday);
    //Collection of logging destinations:
    if (!urls.empty())
      {
        logger.msg(Arc::VERBOSE, "Interactive mode.");
        std::string url=urls.at(0);
        std::string topic=topics.at(0);

        if ( !topic.empty() ||
              url.substr(0,4) == "APEL"){
            //dest = new ApelReReporter(url, topic);
            dest = new ApelDestination(url, topic);
            regexp = "usagerecordCAR.[0-9A-Za-z]+\\.[^.]+$";
        }else{
            dest = new LutsDestination(url, vo_filters_);
            regexp = "usagerecord.[0-9A-Za-z]+\\.[^.]+$";
        }
      }
  }

  /**
   *  Parse usage data and publish it via the appropriate destination adapter.
   */
  int ReReporter::report()
  {
    //Collect job log file names from job log dir
    //(to know where to get usage data from)
    DIR *dirp = NULL;
    dirent *entp = NULL;
    errno=0;
    if ( (dirp=opendir(archivedjob_log_dir.c_str()))==NULL )
      {
        logger.msg(Arc::ERROR, 
                   "Could not open log directory \"%s\": %s",
                   archivedjob_log_dir.c_str(),
                   StrError(errno)
                   );
        return -1;
      }

    // Seek "usagerecord[CAR].<jobnumber>.<randomstring>" files.
    Arc::RegularExpression logfilepattern(regexp);
    errno = 0;
    while ((entp = readdir(dirp)) != NULL)
      {
        if (logfilepattern.match(entp->d_name))
          {
            //TODO handle DOS-style path separator!
            std::string fname=archivedjob_log_dir+"/"+entp->d_name;
            struct tm* clock;               // create a time structure
            struct stat attrib;             // create a file attribute structure
            stat(fname.c_str(), &attrib);       // get the attributes of file
            clock = gmtime(&(attrib.st_mtime)); // Get the last modified time and put it into the time structure
            clock->tm_year+=1900;
            clock->tm_mon+=1;

            // checking it is in the given range
            if ( mktime(start) <= mktime(clock) &&
                 mktime(clock) <= mktime(end)) {
                if ( vo_filters != "")
                  {
     // TODO: if we want
                  }
                dest->report(fname);
            }
          }
        errno = 0;
      }
    closedir(dirp);

    if (errno!=0)
      {
        logger.msg(Arc::ERROR, 
                   "Error reading log directory \"%s\": %s",
                   archivedjob_log_dir.c_str(),
                   StrError(errno)
                   );
        return -2;
      }
    return 0;
  }
  
  ReReporter::~ReReporter()
  {
    delete dest;
    logger.msg(Arc::INFO, "Finished, job log dir: %s",
               archivedjob_log_dir.c_str());
  }
  
}
