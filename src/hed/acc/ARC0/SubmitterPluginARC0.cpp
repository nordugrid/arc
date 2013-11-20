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
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/Job.h>
#include <arc/compute/JobDescription.h>
#include <arc/compute/SubmissionStatus.h>

#include "SubmitterPluginARC0.h"
#include "FTPControl.h"

namespace Arc {

  // Characters to be escaped in LDAP filter according to RFC4515
  static const std::string filter_esc("&|=!><~*/()");

  Logger SubmitterPluginARC0::logger(Logger::getRootLogger(), "SubmitterPlugin.ARC0");

  bool SubmitterPluginARC0::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "gsiftp";
  }

  static URL CreateURL(std::string str) {
    std::string::size_type pos1 = str.find("://");
    if (pos1 == std::string::npos) {
      str = "gsiftp://" + str;
      pos1 = 6;
    } else {
      if(lower(str.substr(0,pos1)) != "gsiftp") return URL();
    }
    std::string::size_type pos2 = str.find(":", pos1 + 3);
    std::string::size_type pos3 = str.find("/", pos1 + 3);
    if (pos3 == std::string::npos) {
      if (pos2 == std::string::npos)
        str += ":2811";
      str += "/jobs";
    }
    else if (pos2 == std::string::npos || pos2 > pos3)
      str.insert(pos3, ":2811");

    return str;
  }
  
  static URL CreateInfoURL(std::string str) {
    std::string::size_type pos1 = str.find("://");
    if (pos1 == std::string::npos) {
      str = "ldap://" + str;
      pos1 = 4;
    } else {
      if(lower(str.substr(0,pos1)) != "ldap") return URL();
    }
    std::string::size_type pos2 = str.find(":", pos1 + 3);
    std::string::size_type pos3 = str.find("/", pos1 + 3);
    if (pos3 == std::string::npos) {
      if (pos2 == std::string::npos)
        str += ":2135";
      str += "/Mds-Vo-name=local,o=Grid";
    }
    else if (pos2 == std::string::npos || pos2 > pos3)
      str.insert(pos3, ":2135");

    return str;
  }
  
  Plugin* SubmitterPluginARC0::Instance(PluginArgument *arg) {
    SubmitterPluginArgument *subarg =
      dynamic_cast<SubmitterPluginArgument*>(arg);
    if (!subarg)
      return NULL;
    Glib::Module* module = subarg->get_module();
    PluginsFactory* factory = subarg->get_factory();
    if(!(factory && module)) {
      logger.msg(ERROR, "Missing reference to factory and/or module. It is unsafe to use Globus in non-persistent mode - SubmitterPlugin for ARC0 is disabled. Report to developers.");
      return NULL;
    }
    factory->makePersistent(module);
    return new SubmitterPluginARC0(*subarg, arg);
  }


  SubmissionStatus SubmitterPluginARC0::Submit(const std::list<JobDescription>& jobdescs, const std::string& endpoint, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted) {
    FTPControl ctrl;
    URL url = CreateURL(endpoint);
    URL infoURL = CreateInfoURL(url.Host());
    
    SubmissionStatus retval;
    for (std::list<JobDescription>::const_iterator it = jobdescs.begin(); it != jobdescs.end(); ++it) {
      if (!ctrl.Connect(url, usercfg)) {
        logger.msg(INFO, "Submit: Failed to connect");
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
  
      if (!ctrl.SendCommand("CWD " + url.Path(), usercfg.Timeout())) {
        logger.msg(INFO, "Submit: Failed sending CWD command");
        ctrl.Disconnect(usercfg.Timeout());
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
  
      std::string response;
  
      if (!ctrl.SendCommand("CWD new", response, usercfg.Timeout())) {
        logger.msg(INFO, "Submit: Failed sending CWD new command");
        ctrl.Disconnect(usercfg.Timeout());
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
  
      std::string::size_type pos2 = response.rfind('"');
      std::string::size_type pos1 = response.rfind('/', pos2 - 1);
      std::string jobnumber = response.substr(pos1 + 1, pos2 - pos1 - 1);
  
      JobDescription preparedjobdesc(*it);
  
      if (preparedjobdesc.OtherAttributes["nordugrid:xrsl;clientxrsl"].empty())
        preparedjobdesc.UnParse(preparedjobdesc.OtherAttributes["nordugrid:xrsl;clientxrsl"], "nordugrid:xrsl");
  
      preparedjobdesc.OtherAttributes["nordugrid:xrsl;action"] = "request";
      preparedjobdesc.OtherAttributes["nordugrid:xrsl;savestate"] = "yes";
      preparedjobdesc.OtherAttributes["nordugrid:xrsl;clientsoftware"] = "libarccompute-" VERSION;
  #ifdef HAVE_GETHOSTNAME
      char hostname[1024];
      gethostname(hostname, 1024);
      preparedjobdesc.OtherAttributes["nordugrid:xrsl;hostname"] = hostname;
  #endif

      if (!preparedjobdesc.Prepare()) {
        logger.msg(INFO, "Failed to prepare job description.");
        ctrl.Disconnect(usercfg.Timeout());
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      }

      std::string jobdescstring;
      JobDescriptionResult ures = preparedjobdesc.UnParse(jobdescstring, "nordugrid:xrsl", "GRIDMANAGER");
      if (!ures) {
        logger.msg(INFO, "Unable to submit job. Job description is not valid in the %s format: %s", "nordugrid:xrsl", ures.str());
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      }
  
      if (!ctrl.SendData(jobdescstring, "job", usercfg.Timeout())) {
        logger.msg(INFO, "Submit: Failed sending job description");
        ctrl.Disconnect(usercfg.Timeout());
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
  
      if (!ctrl.Disconnect(usercfg.Timeout())) {
        logger.msg(INFO, "Submit: Failed to disconnect after submission");
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
  
      URL jobid(url);
      URL ContactString(url);
      jobid.ChangePath(jobid.Path() + '/' + jobnumber);
  
      if (!PutFiles(preparedjobdesc, jobid)) {
        logger.msg(INFO, "Submit: Failed uploading local input files");
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }

      Job j;

      // Prepare contact url for information about this job
      URL infoendpoint(infoURL);
      infoendpoint.ChangeLDAPFilter("(nordugrid-job-globalid=" + escape_chars(jobid.str(),filter_esc,'\\',false,escape_hex) + ")");
      infoendpoint.ChangeLDAPScope(URL::subtree);

      // Proposed mandatory attributes for ARC 3.0
      j.JobID = jobid.fullstr();
      j.ServiceInformationURL = infoendpoint;
      j.ServiceInformationURL.ChangeLDAPFilter("");
      j.ServiceInformationInterfaceName = "org.nordugrid.ldapng";
      j.JobStatusURL = infoendpoint;
      j.JobStatusURL.ChangeLDAPFilter("(nordugrid-job-globalid=" + escape_chars(jobid.str(),filter_esc,'\\',false,escape_hex) + ")");
      j.JobStatusURL.ChangeLDAPScope(URL::subtree);
      j.JobStatusInterfaceName = "org.nordugrid.ldapng";
      j.JobManagementURL = ContactString;
      j.JobManagementInterfaceName = "org.nordugrid.gridftpjob";
      j.IDFromEndpoint = jobnumber;
      j.StageInDir = jobid;
      j.StageOutDir = jobid;
      j.SessionDir = jobid;

      AddJobDetails(preparedjobdesc, j);
      jc.addEntity(j);
    }

    return retval;
  }

  SubmissionStatus SubmitterPluginARC0::Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted) {
    SubmissionStatus retval;

    // gridftp and ldapng intterfaces are bound. So for submiting to
    // to gridftp presence of ldapng is needed.
    // This will not help against misbehaving information system 
    // because actual state of interface is not propagated to 
    // OtherEndpoints. But it should prevent submission to sites 
    // where ldapng is explicitely disabled.
    bool ldapng_interface_present = false;
    for (std::list< CountedPointer<ComputingEndpointAttributes> >::const_iterator it = et.OtherEndpoints.begin(); it != et.OtherEndpoints.end(); it++) {
      if (((*it)->InterfaceName == "org.nordugrid.ldapng") &&
          (((*it)->HealthState == "ok") || ((*it)->HealthState.empty()))) {
        ldapng_interface_present = true;
        break;
      }
    }
    if(!ldapng_interface_present) {
      logger.msg(INFO, "Submit: service has no suitable information interface - need org.nordugrid.ldapng");
      retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
      retval |= SubmissionStatus::NO_SERVICES;
      return retval;
    }

    FTPControl ctrl;
    URL url(et.ComputingEndpoint->URLString);
    
    for (std::list<JobDescription>::const_iterator it = jobdescs.begin(); it != jobdescs.end(); ++it) {
      if (!ctrl.Connect(url, usercfg)) {
        logger.msg(INFO, "Submit: Failed to connect");
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
  
      if (!ctrl.SendCommand("CWD " + url.Path(), usercfg.Timeout())) {
        logger.msg(INFO, "Submit: Failed sending CWD command");
        ctrl.Disconnect(usercfg.Timeout());
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
  
      std::string response;
  
      if (!ctrl.SendCommand("CWD new", response, usercfg.Timeout())) {
        logger.msg(INFO, "Submit: Failed sending CWD new command");
        ctrl.Disconnect(usercfg.Timeout());
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
  
      std::string::size_type pos2 = response.rfind('"');
      std::string::size_type pos1 = response.rfind('/', pos2 - 1);
      std::string jobnumber = response.substr(pos1 + 1, pos2 - pos1 - 1);
  
      JobDescription preparedjobdesc(*it);
  
      if (preparedjobdesc.OtherAttributes["nordugrid:xrsl;clientxrsl"].empty())
        preparedjobdesc.UnParse(preparedjobdesc.OtherAttributes["nordugrid:xrsl;clientxrsl"], "nordugrid:xrsl");
  
      preparedjobdesc.OtherAttributes["nordugrid:xrsl;action"] = "request";
      preparedjobdesc.OtherAttributes["nordugrid:xrsl;savestate"] = "yes";
      preparedjobdesc.OtherAttributes["nordugrid:xrsl;clientsoftware"] = "libarccompute-" VERSION;
  #ifdef HAVE_GETHOSTNAME
      char hostname[1024];
      gethostname(hostname, 1024);
      preparedjobdesc.OtherAttributes["nordugrid:xrsl;hostname"] = hostname;
  #endif
  
      if (!preparedjobdesc.Prepare(et)) {
        logger.msg(INFO, "Failed to prepare job description to target resources.");
        ctrl.Disconnect(usercfg.Timeout());
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      }
  
      std::string jobdescstring;
      JobDescriptionResult ures = preparedjobdesc.UnParse(jobdescstring, "nordugrid:xrsl", "GRIDMANAGER");
      if (!ures) {
        logger.msg(INFO, "Unable to submit job. Job description is not valid in the %s format: %s", "nordugrid:xrsl", ures.str());
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        continue;
      }
  
      if (!ctrl.SendData(jobdescstring, "job", usercfg.Timeout())) {
        logger.msg(INFO, "Submit: Failed sending job description");
        ctrl.Disconnect(usercfg.Timeout());
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
  
      if (!ctrl.Disconnect(usercfg.Timeout())) {
        logger.msg(INFO, "Submit: Failed to disconnect after submission");
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }
  
      URL jobid(url);
      URL ContactString(url);
      jobid.ChangePath(jobid.Path() + '/' + jobnumber);
  
      if (!PutFiles(preparedjobdesc, jobid)) {
        logger.msg(INFO, "Submit: Failed uploading local input files");
        notSubmitted.push_back(&*it);
        retval |= SubmissionStatus::DESCRIPTION_NOT_SUBMITTED;
        retval |= SubmissionStatus::ERROR_FROM_ENDPOINT;
        continue;
      }

      // Prepare contact url for information about this job
      URL infoendpoint;
      for (std::list< CountedPointer<ComputingEndpointAttributes> >::const_iterator it = et.OtherEndpoints.begin(); it != et.OtherEndpoints.end(); it++) {
        if (((*it)->InterfaceName == "org.nordugrid.ldapng") &&
            (((*it)->HealthState == "ok") || ((*it)->HealthState.empty()))) {
          infoendpoint = URL((*it)->URLString);
          infoendpoint.ChangeLDAPScope(URL::subtree);
          break;
        }
      }

      if (!infoendpoint) {
        // Should not happen
        infoendpoint = CreateInfoURL(ContactString.Host());
      }

      Job j;
      
      // Proposed mandatory attributes for ARC 3.0
      j.JobID = jobid.fullstr();
      j.ServiceInformationURL = infoendpoint;
      j.ServiceInformationURL.ChangeLDAPFilter("");
      j.ServiceInformationInterfaceName = "org.nordugrid.ldapng";
      j.JobStatusURL = infoendpoint;
      j.JobStatusURL.ChangeLDAPFilter("(nordugrid-job-globalid=" + escape_chars(jobid.str(),filter_esc,'\\',false,escape_hex) + ")");
      j.JobStatusURL.ChangeLDAPScope(URL::subtree);
      j.JobStatusInterfaceName = "org.nordugrid.ldapng";
      j.JobManagementURL = ContactString;
      j.JobManagementInterfaceName = "org.nordugrid.gridftpjob";
      j.IDFromEndpoint = jobnumber;
      j.StageInDir = jobid;
      j.StageOutDir = jobid;
      j.SessionDir = jobid;
      
      AddJobDetails(preparedjobdesc, j);
      jc.addEntity(j);
    }

    return retval;
  }
} // namespace Arc
