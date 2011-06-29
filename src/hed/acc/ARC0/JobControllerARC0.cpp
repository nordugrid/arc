// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <map>
#include <glibmm.h>

#include <arc/client/JobDescription.h>
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
#include "JobControllerARC0.h"
#include "FTPControl.h"

namespace Arc {

  Logger JobControllerARC0::logger(Logger::getRootLogger(),
                                   "JobController.ARC0");

  JobControllerARC0::JobControllerARC0(const UserConfig& usercfg)
    : JobController(usercfg, "ARC0") {}

  JobControllerARC0::~JobControllerARC0() {}

  Plugin* JobControllerARC0::Instance(PluginArgument *arg) {
    JobControllerPluginArgument *jcarg =
      dynamic_cast<JobControllerPluginArgument*>(arg);
    if (!jcarg)
      return NULL;
    Glib::Module* module = jcarg->get_module();
    PluginsFactory* factory = jcarg->get_factory();
    if(!(factory && module)) {
      logger.msg(ERROR, "Missing reference to factory and/or module. It is unsafe to use Globus in non-persistent mode - Submitter for ARC0 is disabled. Report to developers.");
      return NULL;
    }
    factory->makePersistent(module);
    return new JobControllerARC0(*jcarg);
  }

  void JobControllerARC0::GetJobInformation() {

    std::map<std::string, std::list<Job*> > jobsbyhost;
    for (std::list<Job>::iterator it = jobstore.begin();
         it != jobstore.end(); it++)
      jobsbyhost[it->InfoEndpoint.ConnectionURL() +
                 it->InfoEndpoint.Path()].push_back(&*it);

    for (std::map<std::string, std::list<Job*> >::iterator hostit =
           jobsbyhost.begin(); hostit != jobsbyhost.end(); hostit++) {

      URL infourl = (*hostit->second.begin())->InfoEndpoint;

      // merge filters
      std::string filter = "(|";
      for (std::list<Job*>::iterator it = hostit->second.begin();
           it != hostit->second.end(); it++)
        filter += (*it)->InfoEndpoint.LDAPFilter();
      filter += ")";

      infourl.ChangeLDAPFilter(filter);

      DataHandle handler(infourl, usercfg);
      DataBuffer buffer;

      if (!handler) {
        logger.msg(INFO, "Can't create information handle - is the ARC LDAP DMC plugin available?");
        return;
      }

      if (!handler->StartReading(buffer))
        return;

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
        return;

      XMLNode xmlresult(result);
      XMLNodeList jobinfolist =
        xmlresult.Path("o/Mds-Vo-name/nordugrid-cluster-name/nordugrid-queue-name/nordugrid-info-group-name/nordugrid-job-globalid");

      for (std::list<Job*>::iterator jit = hostit->second.begin();
           jit != hostit->second.end(); ++jit) {
        XMLNodeList::iterator xit = jobinfolist.begin();
        for (; xit != jobinfolist.end(); ++xit) {
          if ((std::string)(*xit)["nordugrid-job-globalid"] == (*jit)->JobID.str()) {
            break;
          }
        }

        if (xit == jobinfolist.end()) {
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
          (*jit)->UsedSlots = stringtoi((*xit)["nordugrid-job-cpucount"]);
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
            (*jit)->UsedApplicationEnvironment.push_back((std::string)n);

        jobinfolist.erase(xit);
      }
    }
  }

  bool JobControllerARC0::GetJob(const Job& job,
                                 const std::string& downloaddir,
                                 const bool usejobname,
                                 const bool force) {

    logger.msg(VERBOSE, "Downloading job: %s", job.JobID.str());

    std::string jobidnum;
    if (usejobname && !job.Name.empty())
      jobidnum = job.Name;
    else {
      std::string path = job.JobID.Path();
      std::string::size_type pos = path.rfind('/');
      jobidnum = path.substr(pos + 1);
    }

    std::list<std::string> files = GetDownloadFiles(job.JobID);

    URL src(job.JobID);
    URL dst(downloaddir.empty() ? jobidnum : downloaddir + G_DIR_SEPARATOR_S + jobidnum);

    std::string srcpath = src.Path();
    std::string dstpath = dst.Path();

    if (!force && Glib::file_test(dstpath, Glib::FILE_TEST_EXISTS)) {
      logger.msg(WARNING, "%s directory exist! Skipping job.", dstpath);
      return false;
    }

    if (srcpath[srcpath.size() - 1] != '/')
      srcpath += '/';
    if (dstpath[dstpath.size() - 1] != G_DIR_SEPARATOR)
      dstpath += G_DIR_SEPARATOR_S;

    bool ok = true;

    for (std::list<std::string>::iterator it = files.begin();
         it != files.end(); it++) {
      src.ChangePath(srcpath + *it);
      dst.ChangePath(dstpath + *it);
      if (!ARCCopyFile(src, dst)) {
        logger.msg(INFO, "Failed downloading %s to %s", src.str(), dst.str());
        ok = false;
      }
    }

    return ok;
  }

  bool JobControllerARC0::CleanJob(const Job& job) {

    logger.msg(VERBOSE, "Cleaning job: %s", job.JobID.str());

    FTPControl ctrl;
    if (!ctrl.Connect(job.JobID, usercfg.ProxyPath(), usercfg.CertificatePath(),
                      usercfg.KeyPath(), usercfg.Timeout())) {
      logger.msg(INFO, "Failed to connect for job cleaning");
      return false;
    }

    std::string path = job.JobID.Path();
    std::string::size_type pos = path.rfind('/');
    std::string jobpath = path.substr(0, pos);
    std::string jobidnum = path.substr(pos + 1);

    if (!ctrl.SendCommand("CWD " + jobpath, usercfg.Timeout())) {
      logger.msg(INFO, "Failed sending CWD command for job cleaning");
      return false;
    }

    if (!ctrl.SendCommand("RMD " + jobidnum, usercfg.Timeout())) {
      logger.msg(INFO, "Failed sending RMD command for job cleaning");
      return false;
    }

    if (!ctrl.Disconnect(usercfg.Timeout())) {
      logger.msg(INFO, "Failed to disconnect after job cleaning");
      return false;
    }

    logger.msg(VERBOSE, "Job cleaning successful");

    return true;
  }

  bool JobControllerARC0::CancelJob(const Job& job) {

    logger.msg(VERBOSE, "Cleaning job: %s", job.JobID.str());

    FTPControl ctrl;
    if (!ctrl.Connect(job.JobID, usercfg.ProxyPath(), usercfg.CertificatePath(),
                      usercfg.KeyPath(), usercfg.Timeout())) {
      logger.msg(INFO, "Failed to connect for job cleaning");
      return false;
    }

    std::string path = job.JobID.Path();
    std::string::size_type pos = path.rfind('/');
    std::string jobpath = path.substr(0, pos);
    std::string jobidnum = path.substr(pos + 1);

    if (!ctrl.SendCommand("CWD " + jobpath, usercfg.Timeout())) {
      logger.msg(INFO, "Failed sending CWD command for job cancelling");
      return false;
    }

    if (!ctrl.SendCommand("DELE " + jobidnum, usercfg.Timeout())) {
      logger.msg(INFO, "Failed sending DELE command for job cancelling");
      return false;
    }

    if (!ctrl.Disconnect(usercfg.Timeout())) {
      logger.msg(INFO, "Failed to disconnect after job cancelling");
      return false;
    }

    logger.msg(VERBOSE, "Job cancelling successful");

    return true;
  }

  bool JobControllerARC0::RenewJob(const Job& job) {

    logger.msg(VERBOSE, "Renewing credentials for job: %s", job.JobID.str());

    FTPControl ctrl;
    if (!ctrl.Connect(job.JobID, usercfg.ProxyPath(), usercfg.CertificatePath(),
                      usercfg.KeyPath(), usercfg.Timeout())) {
      logger.msg(INFO, "Failed to connect for credential renewal");
      return false;
    }

    std::string path = job.JobID.Path();
    std::string::size_type pos = path.rfind('/');
    std::string jobpath = path.substr(0, pos);
    std::string jobidnum = path.substr(pos + 1);

    if (!ctrl.SendCommand("CWD " + jobpath, usercfg.Timeout())) {
      logger.msg(INFO, "Failed sending CWD command for credentials renewal");
      return false;
    }

    if (!ctrl.SendCommand("CWD " + jobidnum, usercfg.Timeout())) {
      logger.msg(INFO, "Failed sending CWD command for credentials renewal");
      return false;
    }

    if (!ctrl.Disconnect(usercfg.Timeout())) {
      logger.msg(INFO, "Failed to disconnect after credentials renewal");
      return false;
    }

    logger.msg(VERBOSE, "Renewal of credentials was successful");

    return true;
  }

  bool JobControllerARC0::ResumeJob(const Job& job) {
    if (!job.RestartState) {
      logger.msg(INFO, "Job %s does not report a resumable state", job.JobID.str());
      return false;
    }
    std::cout << "Resuming job " << job.JobID.str() << " at state " << job.RestartState.GetGeneralState() << " (" << job.RestartState() << ")" << std::endl;

    RenewJob(job);

    // dump rsl into temporary file
    std::string urlstr = job.JobID.str();
    std::string::size_type pos = urlstr.rfind('/');
    if (pos == std::string::npos || pos == 0) {
      logger.msg(INFO, "Illegal jobID specified");
      return false;
    }
    std::string jobnr = urlstr.substr(pos + 1);
    urlstr = urlstr.substr(0, pos) + "/new/action";
    logger.msg(VERBOSE, "HER: %s", urlstr);

    std::string rsl("&(action=restart)(jobid=" + jobnr + ")");

    std::string filename = Glib::build_filename(Glib::get_tmp_dir(), "arcresume.XXXXXX");
    int tmp_h = Glib::mkstemp(filename);
    if (tmp_h == -1) {
      logger.msg(INFO, "Could not create temporary file: %s", filename);
      return false;
    }
    std::ofstream outfile(filename.c_str(), std::ofstream::binary);
    outfile.write(rsl.c_str(), rsl.size());
    if (outfile.fail()) {
      logger.msg(INFO, "Could not write temporary file: %s", filename);
      return false;
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
      if (!res.GetDesc().empty())
        logger.msg(INFO, "Current transfer FAILED: %s - %s", std::string(res), res.GetDesc());
      else
        logger.msg(INFO, "Current transfer FAILED: %s", std::string(res));
      mover.Delete(*destination);
      return false;
    }
    else
      logger.msg(INFO, "Current transfer complete");

    //Cleaning up
    source->Remove();

    logger.msg(VERBOSE, "Job resumed successful");

    return true;
  }

  URL JobControllerARC0::GetFileUrlForJob(const Job& job,
                                          const std::string& whichfile) {

    URL url(job.JobID);

    if (whichfile == "stdout")
      url.ChangePath(url.Path() + '/' + job.StdOut);
    else if (whichfile == "stderr")
      url.ChangePath(url.Path() + '/' + job.StdErr);
    else if (whichfile == "joblog") {
      std::string path = url.Path();
      path.insert(path.rfind('/'), "/info");
      url.ChangePath(path + "/errors");
    }

    return url;
  }

  bool JobControllerARC0::GetJobDescription(const Job& job,
                                            std::string& desc_str) {
    std::string jobid = job.JobID.str();
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
    DataMover mover;
    mover.secure(false);
    mover.passive(true);
    mover.verbose(false);
    mover.force_to_meta(false);
    mover.retry(true);
    FileCache cache;
    URL source_url(cluster + "/info/" + shortid + "/description");
    std::string tmpfile = shortid + G_DIR_SEPARATOR_S + "description";
    std::string localfile = Glib::build_filename(Glib::get_tmp_dir(), tmpfile);
    URL dest_url(localfile);
    DataHandle source(source_url, usercfg);
    DataHandle destination(dest_url, usercfg);
    source->SetTries(1);
    destination->SetTries(1);
    DataStatus res = mover.Transfer(*source, *destination, cache, URLMap(),
                                    0, 0, 0, usercfg.Timeout());
    if (!res.Passed()) {
      if (!res.GetDesc().empty())
        logger.msg(INFO, "Current transfer FAILED: %s - %s",
                   std::string(res), res.GetDesc());
      else
        logger.msg(INFO, "Current transfer FAILED: %s", std::string(res));
      mover.Delete(*destination);
      return false;
    }
    else
      logger.msg(INFO, "Current transfer complete");

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
    destination->Remove();

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
      logger.msg(INFO, "Invalid JobDescription:");
      std::cout << desc_str << std::endl;
      return false;
    }
    logger.msg(VERBOSE, "Valid JobDescription found");
    return true;
  }

  URL JobControllerARC0::CreateURL(std::string service, ServiceType st) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "ldap://" + service;
      pos1 = 4;
    }
    std::string::size_type pos2 = service.find(":", pos1 + 3);
    std::string::size_type pos3 = service.find("/", pos1 + 3);
    if (pos3 == std::string::npos) {
      if (pos2 == std::string::npos)
        service += ":2135";
      if (st == COMPUTING)
        service += "/Mds-Vo-name=local, o=Grid";
      else
        service += "/Mds-Vo-name=NorduGrid, o=Grid";
    }
    else if (pos2 == std::string::npos || pos2 > pos3)
      service.insert(pos3, ":2135");
    return service;
  }

} // namespace Arc
