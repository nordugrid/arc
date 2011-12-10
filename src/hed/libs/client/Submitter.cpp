// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/FileLock.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/client/JobDescription.h>
#include <arc/client/Submitter.h>
#include <arc/UserConfig.h>
#include <arc/data/FileCache.h>
#include <arc/CheckSum.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataHandle.h>
#include <arc/data/URLMap.h>
#include <arc/loader/FinderLoader.h>


namespace Arc {

  Logger Submitter::logger(Logger::getRootLogger(), "Submitter");

  Submitter::Submitter(const UserConfig& usercfg,
                       const std::string& flavour)
    : flavour(flavour),
      usercfg(usercfg) {}

  Submitter::~Submitter() {}

  bool Submitter::GetTestJob(const int& testid, JobDescription& jobdescription) {
    std::string teststring;
    switch (testid) {
        case 1:
          teststring = "&(executable='/bin/echo')(arguments='hello, grid')(jobname='arctest1')(stdout='stdout')";
          break;
        case 2:
          teststring = "&(executable='/bin/env')(jobname='arctest2')(stdout='stdout')(join='yes')";
          break;
        case 3:
          teststring = "&(executable='/bin/cp')(arguments=in.html out.html)(stdout='stdout')(stderr='stderr')(inputfiles=(in.html http://www.nordugrid.org/data/in.html))(outputfiles=(out.html ''))(jobname='arctest3')(gmlog='gmlog')";
          break;
        default:
          return false;
    }
    std::list<JobDescription> jobdescs;
    if (!JobDescription::Parse(teststring, jobdescs, "nordugrid:xrsl")) {
      logger.msg(ERROR, "Test was defined with id %d, but some error occured during parsing it.", testid);
      return false;
    }
    if (jobdescs.size() < 1) {
      logger.msg(ERROR, "No jobdescription resulted at %d. test", testid);
      return false;
    }
    jobdescription = (*(jobdescs.begin()));
    return true;
  }


  bool Submitter::PutFiles(const JobDescription& job, const URL& url) const {

    FileCache cache;
    DataMover mover;
    mover.retry(true);
    mover.secure(false);
    mover.passive(true);
    mover.verbose(false);

    for (std::list<FileType>::const_iterator it = job.Files.begin();
         it != job.Files.end(); it++)
      if (!it->Source.empty()) {
        const URL& src = it->Source.front();
        if (src.Protocol() == "file") {
          if(!url) {
            logger.msg(ERROR, "No stagein URL is provided");
            return false;
          };
          URL dst(std::string(url.str() + '/' + it->Name));
          DataHandle source(src, usercfg);
          DataHandle destination(dst, usercfg);
          DataStatus res =
            mover.Transfer(*source, *destination, cache, URLMap(), 0, 0, 0,
                           usercfg.Timeout());
          if (!res.Passed()) {
            if (!res.GetDesc().empty()) {
              logger.msg(ERROR, "Failed uploading file: %s - %s",
                         std::string(res), res.GetDesc());
            } else {
              logger.msg(ERROR, "Failed uploading file: %s", std::string(res));
            }
            return false;
          }
        }
      }

    return true;
  }

  void Submitter::AddJobDetails(const JobDescription& jobdesc, const URL& jobid,
                                const URL& cluster, const URL& infoendpoint,
                                Job& job) const {
    job.JobID = jobid;
    if (!jobdesc.Identification.JobName.empty()) {
      job.Name = jobdesc.Identification.JobName;
    }
    job.Flavour = flavour;
    job.Cluster = cluster;
    job.InfoEndpoint = infoendpoint;
    job.LocalSubmissionTime = Arc::Time().str(UTCTime);

    job.ActivityOldID = jobdesc.Identification.ActivityOldID;

    jobdesc.UnParse(job.JobDescriptionDocument, "nordugrid:jsdl"); // Assuming job description is valid.

    job.LocalInputFiles.clear();
    for (std::list<FileType>::const_iterator it = jobdesc.Files.begin();
         it != jobdesc.Files.end(); it++) {
      if (!it->Source.empty() && it->Source.front().Protocol() == "file") {
        if (!it->Checksum.empty()) {
          job.LocalInputFiles[it->Name] = it->Checksum;
        }
        else {
          job.LocalInputFiles[it->Name] = CheckSumAny::FileChecksum(it->Source.front().Path(), CheckSumAny::cksum, true);
        }
      }
    }
  }

  SubmitterLoader::SubmitterLoader()
    : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}

  SubmitterLoader::~SubmitterLoader() {
    for (std::list<Submitter*>::iterator it = submitters.begin();
         it != submitters.end(); it++)
      delete *it;
  }

  Submitter* SubmitterLoader::load(const std::string& name,
                                   const UserConfig& usercfg) {
    if (name.empty())
      return NULL;

    if(!factory_->load(FinderLoader::GetLibrariesList(),
                       "HED:Submitter", name)) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "Submitter plugin \"%s\" not found.", name);
      return NULL;
    }

    SubmitterPluginArgument arg(usercfg);
    Submitter *submitter =
      factory_->GetInstance<Submitter>("HED:Submitter", name, &arg, false);

    if (!submitter) {
      logger.msg(ERROR, "Unable to locate the \"%s\" plugin. Please refer to installation instructions and check if package providing support for \"%s\" plugin is installed", name, name);
      logger.msg(DEBUG, "Submitter %s could not be created", name);
      return NULL;
    }

    submitters.push_back(submitter);
    logger.msg(DEBUG, "Loaded Submitter %s", name);
    return submitter;
  }

} // namespace Arc
