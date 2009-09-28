// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/FileLock.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>
#include <arc/client/ExecutionTarget.h>
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

  bool Submitter::PutFiles(const JobDescription& job, const URL& url) const {

    FileCache cache;
    DataMover mover;
    mover.retry(true);
    mover.secure(false);
    mover.passive(true);
    mover.verbose(false);

    for (std::list<FileType>::const_iterator it = job.DataStaging.File.begin();
         it != job.DataStaging.File.end(); it++)
      if (!it->Source.empty()) {
        const URL& src = it->Source.begin()->URI;
        if (src.Protocol() == "file") {
          URL dst(std::string(url.str() + '/' + it->Name));
          DataHandle source(src, usercfg);
          DataHandle destination(dst, usercfg);
          DataStatus res =
            mover.Transfer(*source, *destination, cache, URLMap(), 0, 0, 0,
                           stringtoi(usercfg.ConfTree()["TimeOut"]));
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
  // accoutn it is called after every job submission it needs rewriting.
  void Submitter::AddJob(const JobDescription& job, const URL& jobid,
                         const URL& cluster,
                         const URL& infoendpoint) const {
    NS ns;
    XMLNode info(ns, "Job");
    info.NewChild("JobID") = jobid.str();
    if (!job.Identification.JobName.empty())
      info.NewChild("Name") = job.Identification.JobName;
    info.NewChild("Flavour") = flavour;
    info.NewChild("Cluster") = cluster.str();
    info.NewChild("InfoEndpoint") = infoendpoint.str();
    info.NewChild("LocalSubmissionTime") = (std::string)Arc::Time();

    for (std::list<std::string>::const_iterator
           it = job.Identification.ActivityOldId.begin();
         it != job.Identification.ActivityOldId.end(); it++)
      info.NewChild("OldJobID") = *it;

    std::string rep = job.UnParse("arcjsdl");
    info.NewChild("JobDescription") = (std::string)rep;

    for (std::list<FileType>::const_iterator it = job.DataStaging.File.begin();
         it != job.DataStaging.File.end(); it++)
      if (!it->Source.empty())
        if (it->Source.begin()->URI.Protocol() == "file") {
          if (!info["LocalInputFiles"])
            info.NewChild("LocalInputFiles");
          XMLNode File = info["LocalInputFiles"].NewChild("File");
          File.NewChild("Source") = it->Name;
          File.NewChild("CheckSum") = GetCksum(it->Source.begin()->URI.Path());
        }

    FileLock lock(usercfg.JobListFile());
    Config jobstorage;
    jobstorage.ReadFromFile(usercfg.JobListFile());
    jobstorage.NewChild(info);
    jobstorage.SaveToFile(usercfg.JobListFile());
  }

  std::string Submitter::GetCksum(const std::string& file) const {
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

    PluginList list = FinderLoader::GetPluginList("HED:Submitter");
    factory_->load(list[name], "HED:Submitter");

    SubmitterPluginArgument arg(usercfg);
    Submitter *submitter =
      factory_->GetInstance<Submitter>("HED:Submitter", name, &arg);

    if (!submitter) {
      logger.msg(ERROR, "Submitter %s could not be created", name);
      return NULL;
    }

    submitters.push_back(submitter);
    logger.msg(INFO, "Loaded Submitter %s", name);
    return submitter;
  }

} // namespace Arc
