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


  bool SubmitterPluginARC0::Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted) {
    FTPControl ctrl;
    URL url(et.ComputingEndpoint->URLString);
    
    bool ok = true;
    for (std::list<JobDescription>::const_iterator it = jobdescs.begin(); it != jobdescs.end(); ++it) {
      if (!ctrl.Connect(url,
                        usercfg.ProxyPath(), usercfg.CertificatePath(),
                        usercfg.KeyPath(), usercfg.Timeout())) {
        logger.msg(INFO, "Submit: Failed to connect");
        notSubmitted.push_back(&*it);
        ok = false;
        continue;
      }
  
      if (!ctrl.SendCommand("CWD " + url.Path(), usercfg.Timeout())) {
        logger.msg(INFO, "Submit: Failed sending CWD command");
        ctrl.Disconnect(usercfg.Timeout());
        notSubmitted.push_back(&*it);
        ok = false;
        continue;
      }
  
      std::string response;
  
      if (!ctrl.SendCommand("CWD new", response, usercfg.Timeout())) {
        logger.msg(INFO, "Submit: Failed sending CWD new command");
        ctrl.Disconnect(usercfg.Timeout());
        notSubmitted.push_back(&*it);
        ok = false;
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
      preparedjobdesc.OtherAttributes["nordugrid:xrsl;clientsoftware"] = "libarcclient-" VERSION;
  #ifdef HAVE_GETHOSTNAME
      char hostname[1024];
      gethostname(hostname, 1024);
      preparedjobdesc.OtherAttributes["nordugrid:xrsl;hostname"] = hostname;
  #endif
  
      if (!preparedjobdesc.Prepare(et)) {
        logger.msg(INFO, "Failed to prepare job description to target resources.");
        ctrl.Disconnect(usercfg.Timeout());
        notSubmitted.push_back(&*it);
        ok = false;
        continue;
      }
  
      std::string jobdescstring;
      if (!preparedjobdesc.UnParse(jobdescstring, "nordugrid:xrsl", "GRIDMANAGER")) {
        logger.msg(INFO, "Unable to submit job. Job description is not valid in the %s format", "nordugrid:xrsl");
        notSubmitted.push_back(&*it);
        ok = false;
        continue;
      }
  
      if (!ctrl.SendData(jobdescstring, "job", usercfg.Timeout())) {
        logger.msg(INFO, "Submit: Failed sending job description");
        ctrl.Disconnect(usercfg.Timeout());
        notSubmitted.push_back(&*it);
        ok = false;
        continue;
      }
  
      if (!ctrl.Disconnect(usercfg.Timeout())) {
        logger.msg(INFO, "Submit: Failed to disconnect after submission");
        notSubmitted.push_back(&*it);
        ok = false;
        continue;
      }
  
      URL jobid(url);
      jobid.ChangePath(jobid.Path() + '/' + jobnumber);
  
      if (!PutFiles(preparedjobdesc, jobid)) {
        logger.msg(INFO, "Submit: Failed uploading local input files");
        notSubmitted.push_back(&*it);
        ok = false;
        continue;
      }

      // Prepare contact url for information about this job
      URL infoendpoint(et.ComputingService->Cluster);
      infoendpoint.ChangeLDAPFilter("(nordugrid-job-globalid=" + escape_chars(jobid.str(),filter_esc,'\\',false,escape_hex) + ")");
      infoendpoint.ChangeLDAPScope(URL::subtree);
  
      Job j;
      j.IDFromEndpoint = infoendpoint.fullstr();
      AddJobDetails(preparedjobdesc, jobid, et.ComputingService->Cluster, j);
      jc.addEntity(j);
    }

    return ok;
  }
} // namespace Arc
