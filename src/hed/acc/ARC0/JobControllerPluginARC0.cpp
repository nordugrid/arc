// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <map>
#include <glibmm.h>

#include <arc/compute/JobDescription.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataHandle.h>
#include <arc/data/DataMover.h>
#include <arc/data/URLMap.h>
#include <arc/StringConv.h>
#include <arc/XMLNode.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>

#ifdef WIN32
#include <fcntl.h>
#endif

#include "JobStateARC0.h"
#include "JobControllerPluginARC0.h"
#include "FTPControl.h"

namespace Arc {

  Logger JobControllerPluginARC0::logger(Logger::getRootLogger(), "JobControllerPlugin.ARC0");

  bool JobControllerPluginARC0::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "gsiftp";
  }

  Plugin* JobControllerPluginARC0::Instance(PluginArgument *arg) {
    JobControllerPluginArgument *jcarg =
      dynamic_cast<JobControllerPluginArgument*>(arg);
    if (!jcarg)
      return NULL;
    Glib::Module* module = jcarg->get_module();
    PluginsFactory* factory = jcarg->get_factory();
    if(!(factory && module)) {
      logger.msg(ERROR, "Missing reference to factory and/or module. It is unsafe to use Globus in non-persistent mode - SubmitterPlugin for ARC0 is disabled. Report to developers.");
      return NULL;
    }
    factory->makePersistent(module);
    return new JobControllerPluginARC0(*jcarg, arg);
  }

  void JobControllerPluginARC0::UpdateJobs(std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    std::map<std::string, std::list<Job*> > jobsbyhost;
    for (std::list<Job*>::iterator it = jobs.begin(); it != jobs.end(); ++it) {
      if (!(*it)->JobStatusURL) {
        (*it)->JobStatusURL = (*it)->ServiceInformationURL;
      }
      URL infoEndpoint = (*it)->JobStatusURL;
      jobsbyhost[infoEndpoint.ConnectionURL() +
                 infoEndpoint.Path()].push_back(*it);
    }

    for (std::map<std::string, std::list<Job*> >::iterator hostit =
           jobsbyhost.begin(); hostit != jobsbyhost.end(); ++hostit) {
             
      std::list<Job*> &jobsOnHost = hostit->second;
      while (!jobsOnHost.empty()) {
        logger.msg(DEBUG, "Jobs left to query: %d", jobsOnHost.size());
        // Move the first 1000 element of jobsOnHost into batch
        std::list<Job*> batch = std::list<Job*>();
        for (int i = 0; i < 1000; i++) {
          if (!jobsOnHost.empty()) {
            batch.push_back(jobsOnHost.front());
            jobsOnHost.pop_front();
          }
        }
        logger.msg(DEBUG, "Querying batch with %d jobs", batch.size());
      URL infourl = batch.front()->JobStatusURL;

      // merge filters
      std::string filter = "(|";
      for (std::list<Job*>::iterator it = batch.begin(); it != batch.end(); ++it) {
        filter += (*it)->JobStatusURL.LDAPFilter();
      }
      filter += ")";

      infourl.ChangeLDAPFilter(filter);

      DataBuffer buffer;
      DataHandle handler(infourl, usercfg);

      if (!handler) {
        logger.msg(INFO, "Can't create information handle - is the ARC LDAP DMC plugin available?");
        return;
      }

      if (!handler->StartReading(buffer))
        continue;

      int handle;
      unsigned int length;
      unsigned long long int offset;
      std::string result;

      while (buffer.for_write() || !buffer.eof_read())
        if (buffer.for_write(handle, length, offset, true)) {
          result.append(buffer[handle], length);
          buffer.is_written(handle);
        }

      if (!handler->StopReading())
        continue;

      XMLNode xmlresult(result);
      XMLNodeList jobinfolist =
        xmlresult.Path("o/Mds-Vo-name/nordugrid-cluster-name/nordugrid-queue-name/nordugrid-info-group-name/nordugrid-job-globalid");

      for (std::list<Job*>::iterator jit = batch.begin();
           jit != batch.end(); ++jit) {
        XMLNodeList::iterator xit = jobinfolist.begin();
        for (; xit != jobinfolist.end(); ++xit) {
          if ((std::string)(*xit)["nordugrid-job-globalid"] == (*jit)->JobID) {
            break;
          }
        }

        if (xit == jobinfolist.end()) {
          logger.msg(WARNING, "Job information not found in the information system: %s", (*jit)->JobID);
          if (Time() - (*jit)->LocalSubmissionTime < 90)
            logger.msg(WARNING, "This job was very recently "
                       "submitted and might not yet "
                       "have reached the information system");
          IDsNotProcessed.push_back((*jit)->JobID);
          continue;
        }

        if ((*xit)["nordugrid-job-status"])
          (*jit)->State = JobStateARC0((std::string)(*xit)["nordugrid-job-status"]);
        if ((*xit)["nordugrid-job-globalowner"])
          (*jit)->Owner = (std::string)(*xit)["nordugrid-job-globalowner"];
        if ((*xit)["nordugrid-job-execqueue"])
          (*jit)->Queue = (std::string)(*xit)["nordugrid-job-execqueue"];
        if ((*xit)["nordugrid-job-submissionui"])
          (*jit)->SubmissionHost =
            (std::string)(*xit)["nordugrid-job-submissionui"];
        if ((*xit)["nordugrid-job-submissiontime"])
          (*jit)->SubmissionTime =
            (std::string)(*xit)["nordugrid-job-submissiontime"];
        if ((*xit)["nordugrid-job-sessiondirerasetime"])
          (*jit)->WorkingAreaEraseTime =
            (std::string)(*xit)["nordugrid-job-sessiondirerasetime"];
        if ((*xit)["nordugrid-job-proxyexpirationtime"])
          (*jit)->ProxyExpirationTime =
            (std::string)(*xit)["nordugrid-job-proxyexpirationtime"];
        if ((*xit)["nordugrid-job-completiontime"])
          (*jit)->EndTime =
            (std::string)(*xit)["nordugrid-job-completiontime"];
        if ((*xit)["nordugrid-job-cpucount"])
          (*jit)->RequestedSlots = stringtoi((*xit)["nordugrid-job-cpucount"]);
        if ((*xit)["nordugrid-job-usedcputime"])
          (*jit)->UsedTotalCPUTime =
            Period((std::string)(*xit)["nordugrid-job-usedcputime"],
                   PeriodMinutes);
        if ((*xit)["nordugrid-job-usedwalltime"])
          (*jit)->UsedTotalWallTime =
            Period((std::string)(*xit)["nordugrid-job-usedwalltime"],
                   PeriodMinutes);
        if ((*xit)["nordugrid-job-exitcode"])
          (*jit)->ExitCode = stringtoi((*xit)["nordugrid-job-exitcode"]);
        if ((*xit)["Mds-validfrom"]) {
          (*jit)->CreationTime = (std::string)((*xit)["Mds-validfrom"]);
          if ((*xit)["Mds-validto"]) {
            Time Validto = (std::string)((*xit)["Mds-validto"]);
            (*jit)->Validity = Validto - (*jit)->CreationTime;
          }
        }
        if ((*xit)["nordugrid-job-stdout"])
          (*jit)->StdOut = (std::string)((*xit)["nordugrid-job-stdout"]);
        if ((*xit)["nordugrid-job-stderr"])
          (*jit)->StdErr = (std::string)((*xit)["nordugrid-job-stderr"]);
        if ((*xit)["nordugrid-job-stdin"])
          (*jit)->StdIn = (std::string)((*xit)["nordugrid-job-stdin"]);
        if ((*xit)["nordugrid-job-reqcputime"])
          (*jit)->RequestedTotalCPUTime =
            Period((std::string)((*xit)["nordugrid-job-reqcputime"]),
                   PeriodMinutes);
        if ((*xit)["nordugrid-job-reqwalltime"])
          (*jit)->RequestedTotalWallTime =
            Period((std::string)((*xit)["nordugrid-job-reqwalltime"]),
                   PeriodMinutes);
        if ((*xit)["nordugrid-job-rerunable"])
          (*jit)->RestartState =
            JobStateARC0((std::string)((*xit)["nordugrid-job-rerunable"]));
        if ((*xit)["nordugrid-job-queuerank"])
          (*jit)->WaitingPosition =
            stringtoi((*xit)["nordugrid-job-queuerank"]);
        if ((*xit)["nordugrid-job-comment"])
          (*jit)->OtherMessages.push_back(
            (std::string)((*xit)["nordugrid-job-comment"]));
        if ((*xit)["nordugrid-job-usedmem"])
          (*jit)->UsedMainMemory =
            stringtoi((*xit)["nordugrid-job-usedmem"]);
        if ((*xit)["nordugrid-job-errors"])
          for (XMLNode n = (*xit)["nordugrid-job-errors"]; n; ++n)
            (*jit)->Error.push_back((std::string)n);
        if ((*xit)["nordugrid-job-jobname"])
          (*jit)->Name = (std::string)((*xit)["nordugrid-job-jobname"]);
        if ((*xit)["nordugrid-job-gmlog"])
          (*jit)->LogDir = (std::string)((*xit)["nordugrid-job-gmlog"]);
        if ((*xit)["nordugrid-job-clientsofware"])
          (*jit)->SubmissionClientName =
            (std::string)((*xit)["nordugrid-job-clientsoftware"]);
        if ((*xit)["nordugrid-job-executionnodes"])
          for (XMLNode n = (*xit)["nordugrid-job-executionnodes"]; n; ++n)
            (*jit)->ExecutionNode.push_back((std::string)n);
        if ((*xit)["nordugrid-job-runtimeenvironment"])
          for (XMLNode n = (*xit)["nordugrid-job-runtimeenvironment"]; n; ++n)
            (*jit)->RequestedApplicationEnvironment.push_back((std::string)n);

        jobinfolist.erase(xit);
        IDsProcessed.push_back((*jit)->JobID);
        }   
      }
    }
  }

  bool JobControllerPluginARC0::CleanJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
      
      logger.msg(VERBOSE, "Cleaning job: %s", job.JobID);
  
      FTPControl ctrl;
      if (!ctrl.Connect(URL(job.JobID), usercfg.ProxyPath(), usercfg.CertificatePath(),
                        usercfg.KeyPath(), usercfg.Timeout())) {
        logger.msg(INFO, "Failed to connect for job cleaning");
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
  
      std::string path = URL(job.JobID).Path();
      std::string::size_type pos = path.rfind('/');
      std::string jobpath = path.substr(0, pos);
      std::string jobidnum = path.substr(pos + 1);
  
      if (!ctrl.SendCommand("CWD " + jobpath, usercfg.Timeout())) {
        logger.msg(INFO, "Failed sending CWD command for job cleaning");
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
  
      if (!ctrl.SendCommand("RMD " + jobidnum, usercfg.Timeout())) {
        logger.msg(INFO, "Failed sending RMD command for job cleaning");
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
  
      if (!ctrl.Disconnect(usercfg.Timeout())) {
        logger.msg(INFO, "Failed to disconnect after job cleaning");
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
  
      IDsProcessed.push_back(job.JobID);
      logger.msg(VERBOSE, "Job cleaning successful");
    }

    return ok;
  }

  bool JobControllerPluginARC0::CancelJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;

      logger.msg(VERBOSE, "Cancelling job: %s", job.JobID);
  
      FTPControl ctrl;
      if (!ctrl.Connect(URL(job.JobID), usercfg.ProxyPath(), usercfg.CertificatePath(),
                        usercfg.KeyPath(), usercfg.Timeout())) {
        logger.msg(INFO, "Failed to connect for job cancelling");
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
  
      std::string path = URL(job.JobID).Path();
      std::string::size_type pos = path.rfind('/');
      std::string jobpath = path.substr(0, pos);
      std::string jobidnum = path.substr(pos + 1);
  
      if (!ctrl.SendCommand("CWD " + jobpath, usercfg.Timeout())) {
        logger.msg(INFO, "Failed sending CWD command for job cancelling");
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
  
      if (!ctrl.SendCommand("DELE " + jobidnum, usercfg.Timeout())) {
        logger.msg(INFO, "Failed sending DELE command for job cancelling");
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
  
      if (!ctrl.Disconnect(usercfg.Timeout())) {
        logger.msg(INFO, "Failed to disconnect after job cancelling");
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
  
      IDsProcessed.push_back(job.JobID);
      job.State = JobStateARC0("KILLED");
      logger.msg(VERBOSE, "Job cancelling successful");
    }

    return ok;
  }

  bool JobControllerPluginARC0::RenewJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
  
      logger.msg(VERBOSE, "Renewing credentials for job: %s", job.JobID);
  
      FTPControl ctrl;
      if (!ctrl.Connect(URL(job.JobID), usercfg.ProxyPath(), usercfg.CertificatePath(),
                        usercfg.KeyPath(), usercfg.Timeout())) {
        logger.msg(INFO, "Failed to connect for credential renewal");
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
  
      std::string path = URL(job.JobID).Path();
      std::string::size_type pos = path.rfind('/');
      std::string jobpath = path.substr(0, pos);
      std::string jobidnum = path.substr(pos + 1);
  
      if (!ctrl.SendCommand("CWD " + jobpath, usercfg.Timeout())) {
        logger.msg(INFO, "Failed sending CWD command for credentials renewal");
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
  
      if (!ctrl.SendCommand("CWD " + jobidnum, usercfg.Timeout())) {
        logger.msg(INFO, "Failed sending CWD command for credentials renewal");
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
      }
  
      if (!ctrl.Disconnect(usercfg.Timeout())) {
        logger.msg(INFO, "Failed to disconnect after credentials renewal");
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
  
      IDsProcessed.push_back(job.JobID);
      logger.msg(VERBOSE, "Renewal of credentials was successful");
    }

    return ok;
  }

  bool JobControllerPluginARC0::ResumeJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    bool ok = true;
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      Job& job = **it;
  
      if (!job.RestartState) {
        logger.msg(INFO, "Job %s does not report a resumable state", job.JobID);
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
  
      // dump rsl into temporary file
      std::string urlstr = job.JobID;
      std::string::size_type pos = urlstr.rfind('/');
      if (pos == std::string::npos || pos == 0) {
        logger.msg(INFO, "Illegal jobID specified (%s)", job.JobID);
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
      std::string jobnr = urlstr.substr(pos + 1);
      urlstr = urlstr.substr(0, pos) + "/new/action";
      logger.msg(VERBOSE, "HER: %s", urlstr);
  
      std::string rsl("&(action=restart)(jobid=" + jobnr + ")");
  
      std::string filename = Glib::build_filename(Glib::get_tmp_dir(), "arcresume.XXXXXX");
      int tmp_h = Glib::mkstemp(filename);
      if (tmp_h == -1) {
        logger.msg(INFO, "Could not create temporary file: %s", filename);
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
      std::ofstream outfile(filename.c_str(), std::ofstream::binary);
      outfile.write(rsl.c_str(), rsl.size());
      if (outfile.fail()) {
        logger.msg(INFO, "Could not write temporary file: %s", filename);
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
      outfile.close();
  
      // Send temporary file to cluster
      DataMover mover;
      FileCache cache;
      URL source_url(filename);
      URL dest_url(urlstr);
      DataHandle source(source_url, usercfg);
      DataHandle destination(dest_url, usercfg);
      source->SetTries(1);
      destination->SetTries(1);
      DataStatus res = mover.Transfer(*source, *destination, cache, URLMap(),
                                      0, 0, 0, usercfg.Timeout());
      if (!res.Passed()) {
        logger.msg(INFO, "Current transfer FAILED: %s", std::string(res));
        mover.Delete(*destination);
        ok = false;
        IDsNotProcessed.push_back(job.JobID);
        continue;
      }
      else {
        logger.msg(INFO, "Current transfer complete");
      }
  
      //Cleaning up
      source->Remove();
  
      IDsProcessed.push_back(job.JobID);
      logger.msg(VERBOSE, "Job resumed successful");
    }
  
    return ok;
  }

  bool JobControllerPluginARC0::GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const {
    url = URL(job.JobID);
    switch (resource) {
    case Job::STDIN:
      url.ChangePath(url.Path() + '/' + job.StdIn);
      break;
    case Job::STDOUT:
      url.ChangePath(url.Path() + '/' + job.StdOut);
      break;
    case Job::STDERR:
      url.ChangePath(url.Path() + '/' + job.StdErr);
      break;
    case Job::STAGEINDIR:
    case Job::STAGEOUTDIR:
    case Job::SESSIONDIR:
      break;
    case Job::JOBLOG:
    case Job::JOBDESCRIPTION:
      std::string path = url.Path();
      path.insert(path.rfind('/'), "/info");
      url.ChangePath(path + (Job::JOBLOG ? "/errors" : "/description"));
      break;
    }

    return true;
  }

  bool JobControllerPluginARC0::GetJobDescription(const Job& job,
                                            std::string& desc_str) const {
    std::string jobid = job.JobID;
    logger.msg(VERBOSE, "Trying to retrieve job description of %s from "
               "computing resource", jobid);

    std::string::size_type pos = jobid.rfind("/");
    if (pos == std::string::npos) {
      logger.msg(INFO, "invalid jobID: %s", jobid);
      return false;
    }
    std::string cluster = jobid.substr(0, pos);
    std::string shortid = jobid.substr(pos + 1);

    // Transfer job description
    URL source_url;
    GetURLToJobResource(job, Job::JOBDESCRIPTION, source_url);
    std::string tmpfile = shortid + G_DIR_SEPARATOR_S + "description";
    std::string localfile = Glib::build_filename(Glib::get_tmp_dir(), tmpfile);
    URL dest_url(localfile);

    if (!Job::CopyJobFile(usercfg, source_url, dest_url)) {
      return false;
    }

    // Read job description from file
    std::ifstream descriptionfile(localfile.c_str());

    if (!descriptionfile) {
      logger.msg(INFO, "Can not open job description file: %s", localfile);
      return false;
    }

    descriptionfile.seekg(0, std::ios::end);
    std::streamsize length = descriptionfile.tellg();
    descriptionfile.seekg(0, std::ios::beg);

    char *buffer = new char[length + 1];
    descriptionfile.read(buffer, length);
    descriptionfile.close();

    buffer[length] = '\0';

    desc_str = (std::string)buffer;
    //Cleaning up
    delete[] buffer;

    // Extracting original client xrsl
    pos = desc_str.find("clientxrsl");
    if (pos != std::string::npos) {
      logger.msg(VERBOSE, "clientxrsl found");
      std::string::size_type pos1 = desc_str.find("&", pos);
      if (pos1 == std::string::npos) {
        logger.msg(INFO, "could not find start of clientxrsl");
        return false;
      }
      std::string::size_type pos2 = desc_str.find(")\"", pos1);
      if (pos2 == std::string::npos) {
        logger.msg(INFO, "could not find end of clientxrsl");
        return false;
      }
      desc_str.erase(pos2 + 1);
      desc_str.erase(0, pos1);
      for (std::string::size_type i = 0; i != std::string::npos;) {
        i = desc_str.find("\"\"", i);
        if (i != std::string::npos) {
          desc_str.erase(i, 1);
          // let's step over the second doubleqoute in order not to reduce """" to "
          if (i != std::string::npos) i++;
        }
      }
      logger.msg(DEBUG, "Job description: %s", desc_str);
    }
    else {
      logger.msg(INFO, "clientxrsl not found");
      return false;
    }

    std::list<JobDescription> descs;
    if (!JobDescription::Parse(desc_str, descs) || descs.empty()) {
      logger.msg(INFO, "Invalid JobDescription: %s", desc_str);
      return false;
    }
    logger.msg(VERBOSE, "Valid JobDescription found");
    return true;
  }

} // namespace Arc
