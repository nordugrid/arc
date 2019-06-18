#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "jura.h"
#include "UsageReporter.h"
#include "JobLogFile.h"

//TODO cross-platform
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sstream>

#include <glibmm.h>

#include <arc/ArcRegex.h>
#include <arc/Utils.h>
#include <arc/FileUtils.h>

namespace ArcJura
{

  /** Constructor. Pass name of directory containing job log files,
   *  expiration time of files in seconds, list of URLs in case of 
   *  interactive mode.
   */
  UsageReporter::UsageReporter(ArcJura::Config config_):
    logger(Arc::Logger::rootLogger, "JURA.UsageReporter"),
    dests(NULL),
    config(config_)
  {
    job_log_dir=config.getControlDir()+"/logs";
    expiration_time=config.getUrdeliveryKeepfaild()*(60*60*24);
    //urls(urls_),
    //topics(topics_),
    //vo_filters(vo_filters_),
    if (config.getArchiving()) {
      out_dir=config.getArchiveDir();
    }
    
    logger.msg(Arc::INFO, "Initialised, job log dir: %s",
               job_log_dir.c_str());
    logger.msg(Arc::VERBOSE, "Expiration time: %d seconds",
               expiration_time);
    if (!urls.empty() || !config.getSGAS().empty() || !config.getAPEL().empty())
      {
        logger.msg(Arc::VERBOSE, "Interactive mode.",
                   expiration_time);
      }
    //Collection of logging destinations:
    dests=new Destinations();
  }

  /**
   *  Parse usage data and publish it via the appropriate destination adapter.
   */
  int UsageReporter::report()
  {
    //ngjobid->url mapping to keep track of which loggerurl is replaced
    //by the '-u' options
    std::map<std::string,std::string> dest_to_duplicate;
    //Collect job log file names from job log dir
    //(to know where to get usage data from)
    bool rescan = false;
    DIR *dirp = NULL;
    dirent *entp = NULL;
    errno=0;
    if ( (dirp=opendir(job_log_dir.c_str()))==NULL )
      {
        logger.msg(Arc::ERROR, 
                   "Could not open log directory \"%s\": %s",
                   job_log_dir.c_str(),
                   Arc::StrError(errno)
                   );
        return -1;
      }

    DIR *odirp = NULL;
    errno=0;
    if ( !out_dir.empty() && (odirp=opendir(out_dir.c_str()))==NULL )
      {
        if (errno != ENOENT) {
          logger.msg(Arc::ERROR, "Could not open output directory \"%s\": %s", out_dir, Arc::StrError(errno));
          closedir(dirp);
          return -1;
        } else {
          logger.msg(Arc::INFO,"Creating the output directory \"%s\"", out_dir.c_str());
          if (!Arc::DirCreate(out_dir, S_IRWXU, true)) {
            logger.msg(Arc::ERROR,"Failed to create output directory \"%s\": %s", out_dir, Arc::StrError(errno));
            return -1;
          }
        }
      }
    if (odirp != NULL) {
      closedir(odirp);
    }

    // Seek "<jobnumber>.<randomstring>" files.
    Arc::RegularExpression logfilepattern("^[0-9A-Za-z]+\\.[^.]+$");
    errno = 0;
    while ((entp = readdir(dirp)) != NULL || rescan)
      {
        if (entp == NULL) // rescan
          {
            rewinddir(dirp);
            rescan = false;
            continue;
          }
        if (logfilepattern.match(entp->d_name))
          {
            //Parse log file
            JobLogFile *logfile;
            //TODO handle DOS-style path separator!
            std::string fname=job_log_dir+"/"+entp->d_name;
            logfile=new JobLogFile(fname);
            if (!out_dir.empty())
              {
                if ((*logfile)["jobreport_option_archiving"] == "")
                  {
                    (*logfile)["jobreport_option_archiving"] = out_dir;
                  } else {
                    (*logfile)["outputdir"] = out_dir;
                  }
              }

            if (!config.getVOMSlessVO().empty()) {
              (*logfile)["jobreport_option_vomsless_vo"] = config.getVOMSlessVO();
            }
            if (!config.getVOMSlessIssuer().empty()) {
              (*logfile)["jobreport_option_vomsless_issuer"] = config.getVOMSlessIssuer();
            }
            if (!config.getVOGroup().empty()) {
              (*logfile)["jobreport_option_vo_group"] = config.getVOGroup();
            }
            if (!config.getHostKey().empty()) {
              (*logfile)["key_path"] = config.getHostKey();
            }
            if (!config.getHostCert().empty()) {
              (*logfile)["certificate_path"] = config.getHostCert();
            }
            if (!config.getCADir().empty()) {
              (*logfile)["ca_certificates_dir"] = config.getCADir();
            }
            //A. Non-interactive mode: each jlf is parsed, and if valid, 
            //   submitted to the destination given  by "loggerurl=..."
            if (urls.empty())
              {
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
                    //TODO: handle logfile removing problem by multiple destination
                    //      it is not remove jet only when expired
                  // if it is virgin logfile from a-rex
                  if ( (*logfile)["loggerurl"].empty() )
                  {

                    //Create SGAS reports
                    std::vector<Config::SGAS> const & sgases = config.getSGAS();
                    for (int i=0; i<(int)sgases.size(); i++) {
                      JobLogFile *dupl_logfile=
                        new JobLogFile(*logfile);
                      std::string tmpfilename = fname + "_XXXXXX";
                      Glib::mkstemp(tmpfilename);
                      //dupl_logfile->allowRemove(false);

                      // Set only loggerurl option
                      (*dupl_logfile)["loggerurl"] = sgases[i].targeturl.fullstr();
                      (*dupl_logfile)["jobreport_option_localid_prefix"] = sgases[i].localid_prefix;
                      
                      std::string vo_filters;
                      for (int j=0; j<(int)sgases[i].vofilters.size(); j++) {
                        if (!vo_filters.empty()) {
                          vo_filters += ",";
                        }
                        vo_filters += sgases[i].vofilters[j] + " " + sgases[i].targeturl.fullstr();
                      }
                      if ( vo_filters != "")
                      {
                          (*dupl_logfile)["vo_filters"] = vo_filters;
                      }
                      //Write
                      dupl_logfile->write(tmpfilename);
                      //Pass job log file content to the appropriate 
                      //logging destination
                      //dests->report(*dupl_logfile,sgases[i]);
                      //(deep copy performed)
                      delete dupl_logfile;
                    }

                    // Create APEL reports
                    std::vector<Config::APEL> const & apels = config.getAPEL();
                    for (int i=0; i<(int)apels.size(); i++) {
                      JobLogFile *dupl_logfile=
                        new JobLogFile(*logfile);
                      std::string tmpfilename = fname + "_XXXXXX";
                      Glib::mkstemp(tmpfilename);
                      //dupl_logfile->allowRemove(false);
                      // Set loggerurl and topic option
                      (*dupl_logfile)["loggerurl"] = "APEL:" + apels[i].targeturl.fullstr();
                      (*dupl_logfile)["topic"] = apels[i].topic;
                      if (!apels[i].gocdb_name.empty()) {
                        (*dupl_logfile)["jobreport_option_gocdb_name"] = apels[i].gocdb_name;
                      }
                      if (!apels[i].benchmark_type.empty()) {
                        (*dupl_logfile)["jobreport_option_benchmark_type"] = apels[i].benchmark_type;
                        std::ostringstream buff;
                        buff<<apels[i].benchmark_value;
                        (*dupl_logfile)["jobreport_option_benchmark_value"] = buff.str();
                      }
                      if (!apels[i].benchmark_description.empty()) {
                        (*dupl_logfile)["jobreport_option_benchmark_description"] = apels[i].benchmark_description;
                      }
                      //Write
                      dupl_logfile->write(tmpfilename);

                      delete dupl_logfile;
                    }

                    // delete a-rex log file after all child logfiles were created
                    logfile->remove();
                    // new files need to be processed
                    rescan = true;
                  }
                  else // if it is our remade one for specific source
                  {
                    std::string loggerurl = (*logfile)["loggerurl"];
                    const Config::ACCOUNTING *aconf = NULL;
                    if ( !(*logfile)["topic"].empty() || loggerurl.substr(0,4) == "APEL")
                    { //Search APEL configs
                      loggerurl.erase(0, 5); // cut "APEL:"
                      std::vector<Config::APEL> const & apels = config.getAPEL();
                      for (int i=0; i<(int)apels.size(); i++) {
                        if (apels[i].targeturl == loggerurl) {
                          aconf = new Config::APEL(apels[i]);
                          break;
                        }
                      }
                    }
                    else //Search SGAS configs
                    {
                      std::vector<Config::SGAS> const & sgases = config.getSGAS();
                      for (int i=0; i<(int)sgases.size(); i++) {
                        if (sgases[i].targeturl == loggerurl) {
                          aconf = new Config::SGAS(sgases[i]);
                          break;
                        }
                      }
                    }

                    if (aconf != NULL) {
                      //Pass job log file content to the appropriate 
                      //logging destination
                      dests->report(*logfile, *aconf);
                      //(deep copy performed)
                      delete aconf;
                    }
                    else
                    {
                      // remove logfile because it has no reporter configuration
                      logfile->remove();
                    }
                  }
                }
              }

            //B. Interactive mode: submit only to services specified by
            //   command line option '-u'. Avoid repetition if several jlfs
            //   are created with same content and different destination.
            //   Keep all jlfs on disk.
            else
              {
                if ( dest_to_duplicate.find( (*logfile)["ngjobid"] ) ==
                     dest_to_duplicate.end() )
                  {
                    dest_to_duplicate[ (*logfile)["ngjobid"] ]=
                      (*logfile)["loggerurl"];
                  }

                //submit only 1x to each!
                if ( dest_to_duplicate[ (*logfile)["ngjobid"] ] ==
                     (*logfile)["loggerurl"] )
                  {
                    //Duplicate content of log file, overwriting URL with
                    //each '-u' command line option, disabling file deletion
                    JobLogFile *dupl_logfile=
                      new JobLogFile(*logfile);
                    dupl_logfile->allowRemove(false);

                    for (int it=0; it<(int)urls.size(); it++)
                      {
                        (*dupl_logfile)["loggerurl"] = urls[it];
                        if (!topics[it].empty())
                          {
                            (*dupl_logfile)["topic"] = topics[it];
                          }

                        //Pass duplicated job log content to the appropriate 
                        //logging destination
                 //       dests->report(*dupl_logfile);
                        //(deep copy performed)

                      }
                    delete dupl_logfile;
                  }
              }

            delete logfile;
          }
        errno = 0;
      }
    closedir(dirp);

    if (errno!=0)
      {
        logger.msg(Arc::ERROR, 
                   "Error reading log directory \"%s\": %s",
                   job_log_dir.c_str(),
                   Arc::StrError(errno)
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
