// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/FileLock.h>
#include <arc/Thread.h>
#include <arc/client/JobDescription.h>
#include <arc/client/Submitter.h>
#include <arc/data/FileCache.h>
#include <arc/data/CheckSum.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataHandle.h>
#include <arc/data/URLMap.h>


namespace Arc {

  Logger Submitter::logger(Logger::getRootLogger(), "Submitter");

  Submitter::Submitter(Config *cfg, const std::string& flavour)
    : ACC(cfg, flavour) {
    submissionEndpoint = (std::string)(*cfg)["SubmissionEndpoint"];
    cluster = (std::string)(*cfg)["Cluster"];
  }

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
          std::string dst = url.str() + '/' + it->Name;
          DataHandle source(src);
          source->AssignCredentials(proxyPath, certificatePath,
                                    keyPath, caCertificatesDir);
          DataHandle destination(dst);
          destination->AssignCredentials(proxyPath, certificatePath,
                                         keyPath, caCertificatesDir);

          DataStatus res = mover.Transfer(*source, *destination, cache,
                                          URLMap(), 0, 0, 0, timeout);
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

  void Submitter::AddJob(const JobDescription& job, const URL& jobid,
                         const URL& infoendpoint,
                         const std::string& joblistfile) const {
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

    FileLock lock(joblistfile);
    Config jobstorage;
    jobstorage.ReadFromFile(joblistfile);
    jobstorage.NewChild(info);
    jobstorage.SaveToFile(joblistfile);
  }

  std::string Submitter::GetCksum(const std::string& file) {

    DataHandle source(file);
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

} // namespace Arc
