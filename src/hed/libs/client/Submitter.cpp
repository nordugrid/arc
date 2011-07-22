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
#include <arc/data/CheckSum.h>
#include <arc/data/DataBuffer.h>
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
          teststring = "&(executable='/bin/cp')(arguments=in.html out.html)(stdout='stdout')(stderr='stderr')(inputfiles=(in.html http://www.nordugrid.org/data/in.html))(outputfiles=(out.html ''))(jobname='arctest3')";
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
          URL dst(std::string(url.str() + '/' + it->Name));
          DataHandle source(src, usercfg);
          DataHandle destination(dst, usercfg);
          DataStatus res =
            mover.Transfer(*source, *destination, cache, URLMap(), 0, 0, 0,
                           usercfg.Timeout());
          if (!res.Passed()) {
            if (!res.GetDesc().empty())
              logger.msg(ERROR, "Failed uploading file: %s - %s",
                         std::string(res), res.GetDesc());
            else
              logger.msg(ERROR, "Failed uploading file: %s", std::string(res));
            return false;
          }
        }
      }

    return true;
  }

  // TODO: This method is not scaling well in case of many submitted jobs.
  // For few thousands it may take tens of second to finish. Taking into
  // account it is called after every job submission it needs rewriting.
  void Submitter::AddJob(const JobDescription& job, const URL& jobid,
                         const URL& cluster,
                         const URL& infoendpoint,
                         const std::map<std::string, std::string>& additionalInfo) const {
    NS ns;
    XMLNode info(ns, "Job");
    info.NewChild("JobID") = jobid.str();
    if (!job.Identification.JobName.empty())
      info.NewChild("Name") = job.Identification.JobName;
    info.NewChild("Flavour") = flavour;
    info.NewChild("Cluster") = cluster.str();
    info.NewChild("InfoEndpoint") = infoendpoint.str();
    info.NewChild("LocalSubmissionTime") = (std::string)Arc::Time();

    for (std::map<std::string, std::string>::const_iterator it = additionalInfo.begin();
         it != additionalInfo.end(); it++)
      info.NewChild(it->first) = it->second;

    for (std::list<std::string>::const_iterator
           it = job.Identification.ActivityOldId.begin();
         it != job.Identification.ActivityOldId.end(); it++)
      info.NewChild("ActivityOldID") = *it;

    std::string rep;
    job.UnParse(rep, "nordugrid:jsdl"); // Assuming job description is valid.
    info.NewChild("JobDescription") = (std::string)rep;

    for (std::list<FileType>::const_iterator it = job.Files.begin();
         it != job.Files.end(); it++)
      if (!it->Source.empty())
        if (it->Source.front().Protocol() == "file") {
          if (!info["LocalInputFiles"]) {
            info.NewChild("LocalInputFiles");
          }
          XMLNode File = info["LocalInputFiles"].NewChild("File");
          File.NewChild("Source") = it->Name;
          File.NewChild("CheckSum") = GetCksum(it->Source.front().Path());
        }

    // lock job list file
    FileLock lock(usercfg.JobListFile());
    bool acquired = false;
    for (int tries = 10; tries > 0; --tries) {
      acquired = lock.acquire();
      if (acquired) {
        Config jobstorage;
        jobstorage.ReadFromFile(usercfg.JobListFile());
        jobstorage.NewChild(info);
        jobstorage.SaveToFile(usercfg.JobListFile());
        lock.release();
        break;
      }
      logger.msg(VERBOSE, "Waiting for lock on job list file %s", usercfg.JobListFile());
      Glib::usleep(500000);
    }
    if (!acquired)
      logger.msg(ERROR, "Failed to lock job list file %s. Job list will be out of sync", usercfg.JobListFile());
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

    job.ActivityOldID = jobdesc.Identification.ActivityOldId;

    jobdesc.UnParse(job.JobDescriptionDocument, "nordugrid:jsdl"); // Assuming job description is valid.

    job.LocalInputFiles.clear();
    for (std::list<FileType>::const_iterator it = jobdesc.Files.begin();
         it != jobdesc.Files.end(); it++) {
      if (!it->Source.empty() && it->Source.front().Protocol() == "file") {
        job.LocalInputFiles[it->Name] = GetCksum(it->Source.front().Path());
      }
    }
  }

  // This method is not scaling well in case of many submitted jobs.
  URL Submitter::Submit(const JobDescription& jobdesc,
                        const ExecutionTarget& starget) {
    logger.msg(WARNING, "The Submitter::Submit(const Jobdescription&, const ExecutionTarget&) method is DEPRECATED, use one of the Submit methods.");

    Job j;
    if (!Submit(jobdesc, starget, j)) {
      return URL();
    }

    // Add job.
    // lock job list file
    FileLock lock(usercfg.JobListFile());
    bool acquired = false;
    for (int tries = 10; tries > 0; --tries) {
      acquired = lock.acquire();
      if (acquired) {
        Config jobstorage;
        jobstorage.ReadFromFile(usercfg.JobListFile());
        XMLNode xmljob = jobstorage.NewChild("Job");
        j.ToXML(xmljob);
        lock.release();
        break;
      }
      logger.msg(VERBOSE, "Waiting for lock on job list file %s", usercfg.JobListFile());
      Glib::usleep(500000);
    }
    if (!acquired) {
      logger.msg(ERROR, "Failed to lock job list file %s", usercfg.JobListFile());
      return URL();
    }

    return j.JobID;
  }

  // This method is not scaling well in case of many submitted jobs.
  URL Submitter::Migrate(const URL& jobid, const JobDescription& jobdesc,
                         const ExecutionTarget& starget, bool forcemigration) {
    logger.msg(WARNING, "The Submitter::Migrate(const URL&, const Jobdescription&, const ExecutionTarget&, bool) method is DEPRECATED, use one of the Migrate methods.");

    Job j;
    if (!Migrate(jobid, jobdesc, starget, forcemigration, j)) {
      return URL();
    }

    // Add job.
    // lock job list file
    FileLock lock(usercfg.JobListFile());
    bool acquired = false;
    for (int tries = 10; tries > 0; --tries) {
      acquired = lock.acquire();
      if (acquired) {
        Config jobstorage;
        jobstorage.ReadFromFile(usercfg.JobListFile());
        XMLNode xmljob = jobstorage.NewChild("Job");
        j.ToXML(xmljob);
        jobstorage.SaveToFile(usercfg.JobListFile());
        lock.release();
        break;
      }
      logger.msg(VERBOSE, "Waiting for lock on job list file %s", usercfg.JobListFile());
      Glib::usleep(500000);
    }
    if (!acquired) {
      logger.msg(ERROR, "Failed to lock job list file %s", usercfg.JobListFile());
      return URL();
    }

    return j.JobID;
  }

  std::string Submitter::GetCksum(const std::string& file, const UserConfig& usercfg) {
    DataHandle source(file, usercfg);
    DataBuffer buffer;

    MD5Sum md5sum;
    buffer.set(&md5sum);

    if (!source->StartReading(buffer))
      return "";

    int handle;
    unsigned int length;
    unsigned long long int offset;

    while (buffer.for_write() || !buffer.eof_read())
      if (buffer.for_write(handle, length, offset, true))
        buffer.is_written(handle);

    if (!source->StopReading())
      return "";

    if (!buffer.checksum_valid())
      return "";

    char buf[100];
    md5sum.print(buf, 100);
    return buf;
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
