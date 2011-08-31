// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <sys/stat.h>

#include <glibmm.h>

#include <arc/CheckSum.h>
#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/StringConv.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/client/JobDescription.h>

#include "SubmitterARC0.h"
#include "FTPControl.h"

namespace Arc {

  // Characters to be escaped in LDAP filter according to RFC4515
  static const std::string filter_esc("&|=!><~*/()");

  Logger SubmitterARC0::logger(Logger::getRootLogger(), "Submitter.ARC0");

  SubmitterARC0::SubmitterARC0(const UserConfig& usercfg)
    : Submitter(usercfg, "ARC0") {}

  SubmitterARC0::~SubmitterARC0() {}

  Plugin* SubmitterARC0::Instance(PluginArgument *arg) {
    SubmitterPluginArgument *subarg =
      dynamic_cast<SubmitterPluginArgument*>(arg);
    if (!subarg)
      return NULL;
    Glib::Module* module = subarg->get_module();
    PluginsFactory* factory = subarg->get_factory();
    if(!(factory && module)) {
      logger.msg(ERROR, "Missing reference to factory and/or module. It is unsafe to use Globus in non-persistent mode - Submitter for ARC0 is disabled. Report to developers.");
      return NULL;
    }
    factory->makePersistent(module);
    return new SubmitterARC0(*subarg);
  }


  bool SubmitterARC0::Submit(const JobDescription& jobdesc, const ExecutionTarget& et, Job& job) {

    FTPControl ctrl;

    if (!ctrl.Connect(et.url,
                      usercfg.ProxyPath(), usercfg.CertificatePath(),
                      usercfg.KeyPath(), usercfg.Timeout())) {
      logger.msg(INFO, "Submit: Failed to connect");
      return false;
    }

    if (!ctrl.SendCommand("CWD " + et.url.Path(), usercfg.Timeout())) {
      logger.msg(INFO, "Submit: Failed sending CWD command");
      ctrl.Disconnect(usercfg.Timeout());
      return false;
    }

    std::string response;

    if (!ctrl.SendCommand("CWD new", response, usercfg.Timeout())) {
      logger.msg(INFO, "Submit: Failed sending CWD new command");
      ctrl.Disconnect(usercfg.Timeout());
      return false;
    }

    std::string::size_type pos2 = response.rfind('"');
    std::string::size_type pos1 = response.rfind('/', pos2 - 1);
    std::string jobnumber = response.substr(pos1 + 1, pos2 - pos1 - 1);

    JobDescription modjobdesc(jobdesc);

    if (!ModifyJobDescription(modjobdesc, et)) {
      logger.msg(INFO, "Submit: Failed to modify job description "
                        "to be sent to target.");
      ctrl.Disconnect(usercfg.Timeout());
      return false;
    }

    std::string jobdescstring;
    if (!modjobdesc.UnParse(jobdescstring, "nordugrid:xrsl", "GRIDMANAGER")) {
      logger.msg(INFO, "Unable to submit job. Job description is not valid in the %s format", "nordugrid:xrsl");
      return false;
    }

    if (!ctrl.SendData(jobdescstring, "job", usercfg.Timeout())) {
      logger.msg(INFO, "Submit: Failed sending job description");
      ctrl.Disconnect(usercfg.Timeout());
      return false;
    }

    if (!ctrl.Disconnect(usercfg.Timeout())) {
      logger.msg(INFO, "Submit: Failed to disconnect after submission");
      return false;
    }

    URL jobid(et.url);
    jobid.ChangePath(jobid.Path() + '/' + jobnumber);

    if (!PutFiles(modjobdesc, jobid)) {
      logger.msg(INFO, "Submit: Failed uploading local input files");
      return false;
    }

    // Prepare contact url for information about this job
    URL infoendpoint(et.Cluster);
    infoendpoint.ChangeLDAPFilter("(nordugrid-job-globalid=" + escape_chars(jobid.str(),filter_esc,'\\',false,escape_hex) + ")");
    infoendpoint.ChangeLDAPScope(URL::subtree);

    AddJobDetails(modjobdesc, jobid, et.Cluster, infoendpoint, job);

    return true;
  }

  bool SubmitterARC0::Migrate(const URL& /* jobid */, const JobDescription& /* jobdesc */,
                             const ExecutionTarget& et, bool /* forcemigration */,
                             Job& /* job */) {
    logger.msg(INFO, "Trying to migrate to %s: Migration to a legacy ARC resource is not supported.", et.url.str());
    return false;
  }

  bool SubmitterARC0::ModifyJobDescription(JobDescription& jobdesc, const ExecutionTarget& et) const {
    if (jobdesc.OtherAttributes["nordugrid:xrsl;clientxrsl"].empty())
      jobdesc.UnParse(jobdesc.OtherAttributes["nordugrid:xrsl;clientxrsl"], "nordugrid:xrsl");

    // Check for identical file names.
    // Check if executable and input is contained in the file list.
    // Calculate checksum and filesize for local input files.
    bool executableIsAdded(false), inputIsAdded(false), outputIsAdded(false), errorIsAdded(false), logDirIsAdded(false);
    bool targets_exist(false);
    struct stat fileStat;
    for (std::list<FileType>::iterator it1 = jobdesc.Files.begin();
         it1 != jobdesc.Files.end(); it1++) {
      for (std::list<FileType>::const_iterator it2 = it1;
           it2 != jobdesc.Files.end(); it2++) {
        if (it1 == it2) {
          continue;
        }

        targets_exist = (!it1->Target.empty() && !it2->Target.empty());
        if (it1->Name == it2->Name && ((!it1->Source.empty() && !it2->Source.empty()) ||
                                        targets_exist)) {
          if (targets_exist){
             // When the output file would be sent more than one targets
             if ( !(it1->Target.front() == it2->Target.front()) ) continue;
          }
          logger.msg(ERROR, "Two files have identical file name '%s'.", it1->Name);
          return false;
        }
      }

      if (!it1->Source.empty() && it1->Source.front().Protocol() == "file") {
        if (it1->FileSize < 0) {
          if (stat(it1->Source.front().Path().c_str(), &fileStat) == 0) {
            it1->FileSize = fileStat.st_size;
          }
          else {
            logger.msg(ERROR, "Cannot stat local input file %s", it1->Source.front().Path());
            return false;
          }
        }
        if (it1->Checksum.empty()) {
          it1->Checksum = CheckSumAny::FileChecksum(it1->Source.front().Path(), CheckSumAny::md5);
          if (it1->Checksum.empty()) {
            logger.msg(ERROR, "Unable to calculate checksum of local input file %s", it1->Source.front().Path());
            return false;
          }
        }
      }

      executableIsAdded  |= (it1->Name == jobdesc.Application.Executable.Name);
      inputIsAdded       |= (it1->Name == jobdesc.Application.Input);
      outputIsAdded      |= (it1->Name == jobdesc.Application.Output);
      errorIsAdded       |= (it1->Name == jobdesc.Application.Error);
      logDirIsAdded      |= (it1->Name == jobdesc.Application.LogDir);
    }

    if (!executableIsAdded &&
        !Glib::path_is_absolute(jobdesc.Application.Executable.Name)) {
      FileType file;
      file.Name = jobdesc.Application.Executable.Name;
      file.Source.push_back(URL(file.Name));
      file.KeepData = false;
      file.IsExecutable = true;
      jobdesc.Files.push_back(file);
    }

    if (!jobdesc.Application.Input.empty() && !inputIsAdded) {
      FileType file;
      file.Name = jobdesc.Application.Input;
      file.Source.push_back(URL(file.Name));
      file.KeepData = false;
      file.IsExecutable = false;
      jobdesc.Files.push_back(file);
    }

    if (!jobdesc.Application.Output.empty() && !outputIsAdded) {
      FileType file;
      file.Name = jobdesc.Application.Output;
      file.KeepData = true;
      file.IsExecutable = false;
      jobdesc.Files.push_back(file);
    }

    if (!jobdesc.Application.Error.empty() && !errorIsAdded) {
      FileType file;
      file.Name = jobdesc.Application.Error;
      file.KeepData = true;
      file.IsExecutable = false;
      jobdesc.Files.push_back(file);
    }

    if (!jobdesc.Application.LogDir.empty() && !logDirIsAdded) {
      FileType file;
      file.Name = jobdesc.Application.LogDir;
      file.KeepData = true;
      file.IsExecutable = false;
      jobdesc.Files.push_back(file);
    }

    if (!jobdesc.Resources.RunTimeEnvironment.empty() &&
        !jobdesc.Resources.RunTimeEnvironment.selectSoftware(et.ApplicationEnvironments)) {
      // This error should never happen since RTE is checked in the Broker.
      logger.msg(VERBOSE, "Unable to select runtime environment");
      return false;
    }

    if (!jobdesc.Resources.CEType.empty() &&
        !jobdesc.Resources.CEType.selectSoftware(et.Implementation)) {
      // This error should never happen since Middleware is checked in the Broker.
      logger.msg(VERBOSE, "Unable to select middleware");
      return false;
    }

    if (!jobdesc.Resources.OperatingSystem.empty() &&
        !jobdesc.Resources.OperatingSystem.selectSoftware(et.Implementation)) {
      // This error should never happen since OS is checked in the Broker.
      logger.msg(VERBOSE, "Unable to select operating system.");
      return false;
    }

    // Set queue name to the selected ExecutionTarget
    jobdesc.Resources.QueueName = et.ComputingShareName;

    jobdesc.OtherAttributes["nordugrid:xrsl;action"] = "request";
    jobdesc.OtherAttributes["nordugrid:xrsl;savestate"] = "yes";
    jobdesc.OtherAttributes["nordugrid:xrsl;clientsoftware"] = "libarcclient-" VERSION;
#ifdef HAVE_GETHOSTNAME
    char hostname[1024];
    gethostname(hostname, 1024);
    jobdesc.OtherAttributes["nordugrid:xrsl;hostname"] = hostname;
#endif

    return true;
  }

} // namespace Arc
