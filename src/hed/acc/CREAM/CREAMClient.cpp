// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/communication/ClientInterface.h>
#include <arc/compute/JobDescription.h>
#include <arc/credential/Credential.h>
#include <arc/message/MCC.h>

#include "JobStateCREAM.h"
#include "CREAMClient.h"

#ifdef ISVALID
#undef ISVALID
#endif
#define ISVALID(NODE) NODE && (std::string)NODE != "N/A" && (std::string)NODE != "[reserved]"

namespace Arc {
  Logger CREAMClient::logger(Logger::rootLogger, "CREAMClient");

  bool stringtoTime(const std::string& timestring, Time& time) {
    if (timestring == "" || timestring.length() < 15) return false;

    //The conversion for example:
    //before: 11/5/08 11:52 PM
    //after:  2008-11-05T23:52:00.000Z

    tm timestr;
    std::string::size_type pos = 0;
    if      (sscanf(timestring.substr(pos, 6).c_str(),
                    "%d/%d/%2d",
                    &timestr.tm_mon,
                    &timestr.tm_mday,
                    &timestr.tm_year) == 3) pos += 6;
    else if (sscanf(timestring.substr(pos, 7).c_str(),
                    "%2d/%d/%2d",
                    &timestr.tm_mon,
                    &timestr.tm_mday,
                    &timestr.tm_year) == 3) pos += 7;
    else if (sscanf(timestring.substr(pos, 7).c_str(),
                    "%d/%2d/%2d",
                    &timestr.tm_mon,
                    &timestr.tm_mday,
                    &timestr.tm_year) == 3) pos += 7;
    else if (sscanf(timestring.substr(pos, 8).c_str(),
                    "%2d/%2d/%2d",
                    &timestr.tm_mon,
                    &timestr.tm_mday,
                    &timestr.tm_year) == 3) pos += 8;
    else return false;

    timestr.tm_year += 100;
    timestr.tm_mon--;

    if (timestring[pos] == 'T' || timestring[pos] == ' ') pos++;

    if (sscanf(timestring.substr(pos, 5).c_str(), "%2d:%2d", &timestr.tm_hour, &timestr.tm_min) == 2) pos += 5;
    else return false;

    // skip the space characters
    while (timestring[pos] == ' ') pos++;

    if (timestring.substr(pos, 2) == "PM") timestr.tm_hour += 12;

    time.SetTime(mktime(&timestr));
    return true;
  }

  static void set_cream_namespaces(NS& ns) {
    ns["deleg"] = "http://www.gridsite.org/namespaces/delegation-2";
    ns["types"] = "http://glite.org/2007/11/ce/cream/types";
  }

  XMLNode creamJobInfo::ToXML() const {
    return XMLNode("<jobId>"
                    "<id>"+id+"</id>"
                    "<creamURL>"+creamURL+"</creamURL>"+
                    (!ISB.empty() ? "<property><name>CREAMInputSandboxURI</name><value>" + ISB +"</value></property>" : std::string()) +
                    (!OSB.empty() ? "<property><name>CREAMOutputSandboxURI</name><value>" + OSB +"</value></property>" : std::string()) +
                    "<delegationID>"+delegationID+"</delegationID>"
                   "</jobId>");
  }
  
  creamJobInfo& creamJobInfo::operator=(XMLNode n) {
    id = (std::string)n["id"];
    if (n["creamURL"]) {
      creamURL = URL((std::string)n["creamURL"]);
    }
    for (XMLNode property = n["property"]; property; ++property) {
      if ((std::string)property["name"] == "CREAMInputSandboxURI") {
        ISB = (std::string)property["value"];
      }
      else if ((std::string)property["name"] == "CREAMOutputSandboxURI") {
        OSB = (std::string)property["value"];
      }
    }
    if (n["delegationID"]) {
      delegationID = (std::string)n["delegationID"];
    }
    
    return *this;
  }

  CREAMClient::CREAMClient(const URL& url, const MCCConfig& cfg, int timeout)
    : client(NULL),
      cafile(cfg.cafile),
      cadir(cfg.cadir) {
    logger.msg(INFO, "Creating a CREAM client");
    client = new ClientSOAP(cfg, url, timeout);
    if (!client)
      logger.msg(VERBOSE, "Unable to create SOAP client used by CREAMClient.");
    set_cream_namespaces(cream_ns);
  }

  CREAMClient::~CREAMClient() {
    if (client)
      delete client;
  }

  bool CREAMClient::process(PayloadSOAP& req, XMLNode& response, const std::string& actionNS) {
    if (!client) {
      logger.msg(VERBOSE, "CREAMClient not created properly");
      return false;
    }

    PayloadSOAP *resp = NULL;
    if (!client->process(actionNS + action, &req, &resp)) {
      logger.msg(VERBOSE, "%s request failed", action);
      return false;
    }

    if (resp == NULL) {
      logger.msg(VERBOSE, "There was no SOAP response");
      return false;
    }

    (*resp)[action + "Response"].New(response);

    delete resp;

    if (!response) {
      logger.msg(VERBOSE, "Empty response");
      return false;
    }

    XMLNode fault;
    if (response["JobUnknownFault"])
      fault = response["JobUnknownFault"];
    if (response["JobStatusInvalidFault"])
      fault = response["JobStatusInvalidFault"];
    if (response["DelegationIdMismatchFault"])
      fault = response["DelegationIdMismatchFault"];
    if (response["DateMismatchFault"])
      fault = response["DateMismatchFault"];
    if (response["LeaseIdMismatchFault"])
      fault = response["LeaseIdMismatchFault"];
    if (response["GenericFault"])
      fault = response["GenericFault"];

    if (fault) {
      logger.msg(VERBOSE, "Request failed: %s", (std::string)(fault["Description"]));
      return false;
    }

    return true;
  }

  bool CREAMClient::stat(const std::string& jobid, Job& job) {
    logger.msg(VERBOSE, "Creating and sending a status request");

    action = "JobInfo";

    PayloadSOAP req(cream_ns);
    XMLNode xjobId = req.NewChild("types:" + action + "Request").NewChild("types:jobId");
    xjobId.NewChild("types:id") = jobid;
    xjobId.NewChild("types:creamURL") = client->GetURL().str();

    XMLNode response;
    if (!process(req, response)) return false;

    XMLNode jobInfoNode;
    jobInfoNode = response["result"]["jobInfo"];

    XMLNode lastStatusNode = jobInfoNode.Path("status").back();

    if (lastStatusNode["name"]) {
      job.State = JobStateCREAM((std::string)lastStatusNode["name"]);
    }
    if (lastStatusNode["failureReason"]) {
      job.Error.push_back((std::string)lastStatusNode["failureReason"]);
    }
    
    if (!job.State) {
      logger.msg(VERBOSE, "Unable to retrieve job status.");
      return false;
    }

    if (ISVALID(jobInfoNode["jobId"]["id"])) {
      job.IDFromEndpoint = (std::string)jobInfoNode["jobId"]["id"];
    }
    if (ISVALID(jobInfoNode["type"]))
      job.Type = (std::string)jobInfoNode["type"];
    if (ISVALID(jobInfoNode["JDL"])) {
      job.JobDescription = (std::string)jobInfoNode["JDL"];

      std::list<JobDescription> jds;
      if (JobDescription::Parse(job.JobDescription, jds) && !jds.empty()) {
        if (!jds.front().Application.Input.empty())
          job.StdIn = jds.front().Application.Input;

        if (!jds.front().Application.Output.empty())
          job.StdOut = jds.front().Application.Output;

        if (!jds.front().Application.Error.empty())
          job.StdErr = jds.front().Application.Error;

        if (!jds.front().Resources.QueueName.empty()) {
          job.Queue = jds.front().Resources.QueueName;
        }
      }
    }
    if (ISVALID(lastStatusNode["exitCode"]))
      job.ExitCode = stringtoi((std::string)lastStatusNode["exitCode"]);
    if (ISVALID(jobInfoNode["delegationProxyInfo"])) {
      /* Format of delegationProxyInfo node.
      [ isRFC="<true|false>";
        valid from="<mon>/<date>/<yy> <hour>:<min> <AM|PM> (<timezone>)";
        valid to="<mon>/<date>/<yy> <hour>:<min> <AM|PM> (<timezone>)";
        holder DN="<DN>";
        holder AC issuer="<DN>";
        VO="<VO-name>";
        AC issuer="<DN>";
        VOMS attributes=<VOMS-Attributes>
      ]
      */
      
      std::string delegationProxy = (std::string)jobInfoNode["delegationProxyInfo"];
      std::size_t lBracketPos = delegationProxy.find('['), rBracketPos = delegationProxy.rfind(']');
      if (lBracketPos != std::string::npos && rBracketPos != std::string::npos) {
        delegationProxy = trim(delegationProxy.substr(lBracketPos, rBracketPos - lBracketPos));
      }
      std::list<std::string> tDelegInfo;
      tokenize(delegationProxy, tDelegInfo, ";");
      for (std::list<std::string>::iterator it = tDelegInfo.begin();
           it != tDelegInfo.end(); ++it) {
        std::list<std::string> keyValuePair;
        tokenize(*it, keyValuePair, "=", "\"", "\"");
        if (keyValuePair.size() != 2) continue;
        if (lower(trim(keyValuePair.front())) == "holder dn") job.Owner = trim(keyValuePair.back(), " \"");
        if (lower(trim(keyValuePair.front())) == "valid to")  stringtoTime(trim(keyValuePair.back(), " \""), job.ProxyExpirationTime);
      }
    }
    if (ISVALID(jobInfoNode["localUser"]))
      job.LocalOwner = (std::string)jobInfoNode["localUser"];
    if (ISVALID(jobInfoNode["lastCommand"])) {
      int job_register_id_first = -1;
      int job_register_id_last = -1;
      int job_start_id_first = -1;
      int job_start_id_last = -1;
      int local_id = 0;
      while (true) {
        if (!jobInfoNode["lastCommand"][local_id])
          break;
        if ((std::string)jobInfoNode["lastCommand"][local_id]["name"] == "JOB_REGISTER") {
          if (job_register_id_first == -1 && job_register_id_last == -1) {
            job_register_id_first = local_id;
            job_register_id_last = local_id;
          }
          else if (job_register_id_last > -1)
            job_register_id_last = local_id;
        }  //end of the JOB_REGISTER

        if ((std::string)jobInfoNode["lastCommand"][local_id]["name"] == "JOB_START") {
          if (job_start_id_first == -1 && job_start_id_last == -1) {
            job_start_id_first = local_id;
            job_start_id_last = local_id;
          }
          else if (job_start_id_last > -1)
            job_start_id_last = local_id;
        }  //end of the JOB_START
        local_id++;
      }

      //dependent on JOB_REGISTER
      if (job_register_id_first > -1)
        if (ISVALID(jobInfoNode["lastCommand"][job_register_id_first]["creationTime"])) {
          Time time((std::string)jobInfoNode["lastCommand"][job_register_id_first]["creationTime"]);
          if (time.GetTime() != -1)
            job.SubmissionTime = time;
        }

      if (job_register_id_last > -1)
        if (ISVALID(jobInfoNode["lastCommand"][job_register_id_last]["creationTime"])) {
          Time time((std::string)jobInfoNode["lastCommand"][job_register_id_last]["creationTime"]);
          if (time.GetTime() != -1)
            job.CreationTime = time;
        }
      //end of the JOB_REGISTER

      //dependent on JOB_START
      if (job_start_id_first > -1) {
        if (ISVALID(jobInfoNode["lastCommand"][job_start_id_first]["startSchedulingTime"])) {
          Time time((std::string)jobInfoNode["lastCommand"][job_start_id_first]["startSchedulingTime"]);
          if (time.GetTime() != -1)
            job.ComputingManagerSubmissionTime = time;
        }

        if (ISVALID(jobInfoNode["lastCommand"][job_start_id_first]["startProcessingTime"])) {
          Time time((std::string)jobInfoNode["lastCommand"][job_start_id_first]["startProcessingTime"]);
          if (time.GetTime() != -1)
            job.StartTime = time;
        }
      }

      if (job_start_id_last > -1)
        if (ISVALID(jobInfoNode["lastCommand"][job_start_id_last]["executionCompletedTime"])) {
          Time time((std::string)jobInfoNode["lastCommand"][job_start_id_last]["executionCompletedTime"]);
          if (time.GetTime() != -1)
            job.ComputingManagerEndTime = time;
        }
      //end of the JOB_START
    } //end of the LastCommand
    if (ISVALID(lastStatusNode["timestamp"]) && (job.State() == "DONE-OK" || job.State() == "DONE-FAILED")) {
      Time time((std::string)lastStatusNode["timestamp"]);
      if (time.GetTime() != -1)
        job.EndTime = time;
    }

    return true;
  }

  bool CREAMClient::cancel(const std::string& jobid) {
    logger.msg(VERBOSE, "Creating and sending request to terminate a job");

    action = "JobCancel";

    PayloadSOAP req(cream_ns);
    XMLNode xjobId = req.NewChild("types:" + action + "Request").NewChild("types:jobId");
    xjobId.NewChild("types:id") = jobid;
    xjobId.NewChild("types:creamURL") = client->GetURL().str();

    XMLNode response;
    if (!process(req, response)) return false;

    if (!response) {
      logger.msg(VERBOSE, "Empty response");
      return false;
    }

    return true;
  }

  bool CREAMClient::purge(const std::string& jobid) {
    logger.msg(VERBOSE, "Creating and sending request to clean a job");

    action = "JobPurge";

    PayloadSOAP req(cream_ns);
    XMLNode xjobId = req.NewChild("types:" + action + "Request").NewChild("types:jobId");
    xjobId.NewChild("types:id") = jobid;
    xjobId.NewChild("types:creamURL") = client->GetURL().str();

    XMLNode response;
    if (!process(req, response)) return false;

    if (!response) {
      logger.msg(VERBOSE, "Empty response");
      return false;
    }

    return true;
  }

  bool CREAMClient::resume(const std::string& jobid) {
    logger.msg(VERBOSE, "Creating and sending request to resume a job");

    action = "JobResume";

    PayloadSOAP req(cream_ns);
    XMLNode xjobId = req.NewChild("types:" + action + "Request").NewChild("types:jobId");
    xjobId.NewChild("types:id") = jobid;
    xjobId.NewChild("types:creamURL") = client->GetURL().str();

    XMLNode response;
    if (!process(req, response)) return false;

    if (!response) {
      logger.msg(VERBOSE, "Empty response");
      return false;
    }

    return true;
  }

  bool CREAMClient::listJobs(std::list<creamJobInfo>& info) {
    logger.msg(VERBOSE, "Creating and sending request to list jobs");

    action = "JobList";

    PayloadSOAP req(cream_ns);
    req.NewChild("types:" + action + "Request");

    XMLNode response;
    if (!process(req, response)) return false;

    if (!response) {
      logger.msg(VERBOSE, "Empty response");
      return false;
    }
    
    for (XMLNode n = response["result"]; n; ++n) {
      creamJobInfo i;
      i = n;
      info.push_back(i);
    }

    return true;
  }

  bool CREAMClient::registerJob(const std::string& jdl_text,
                                creamJobInfo& info) {
    logger.msg(VERBOSE, "Creating and sending job register request");

    action = "JobRegister";

    PayloadSOAP req(cream_ns);
    XMLNode act_job = req.NewChild("types:" + action + "Request").NewChild("types:jobDescriptionList");
    act_job.NewChild("types:JDL") = jdl_text;
    if (!delegationId.empty())
      act_job.NewChild("types:delegationId") = delegationId;
    act_job.NewChild("types:autoStart") = "false";

    XMLNode response;
    if (!process(req, response)) return false;

    if (!response) {
      logger.msg(VERBOSE, "Empty response");
      return false;
    }

    if (!response["jobId"]["id"]) {
      logger.msg(VERBOSE, "No job ID in response");
      return false;
    }

    info = response["jobId"];

    return true;
  }

  bool CREAMClient::startJob(const std::string& jobid) {
    logger.msg(VERBOSE, "Creating and sending job start request");

    action = "JobStart";

    PayloadSOAP req(cream_ns);
    XMLNode jobStartRequest = req.NewChild("types:" + action + "Request");
    XMLNode xjobId = jobStartRequest.NewChild("types:jobId");
    xjobId.NewChild("types:id") = jobid;
    xjobId.NewChild("types:creamURL") = client->GetURL().str();

    XMLNode response;
    if (!process(req, response)) return false;

    if (!response) {
      logger.msg(VERBOSE, "Empty response");
      return false;
    }

    if (!response["result"]["jobId"]["id"]) {
      logger.msg(VERBOSE, "No job ID in response");
      return false;
    }

    return true;
  }

  bool CREAMClient::createDelegation(const std::string& delegation_id,
                                     const std::string& proxy) {
    logger.msg(VERBOSE, "Creating delegation");

    action = "getProxyReq";

    PayloadSOAP req(cream_ns);
    req.NewChild("deleg:" + action).NewChild("delegationID") = delegation_id;

    XMLNode response;
    if (!process(req, response, "http://www.gridsite.org/namespaces/delegation-2/")) return false;

    std::string proxyRequestStr = (std::string)response["getProxyReqReturn"];
    if (proxyRequestStr.empty()) {
      logger.msg(VERBOSE, "Malformed response: missing getProxyReqReturn");
      return false;
    }

    //Sign the proxy certificate
    Credential signer(proxy, "", cadir, cafile);
    std::string signedCert;
    // TODO: Hardcoded time shift - VERY BAD approach
    Time start_time = Time() - Period(300);
    Time end_time = signer.GetEndTime();
    if(end_time < start_time) {
      logger.msg(VERBOSE, "Delegatable credentials expired: %s",end_time.str());
      return false;
    }
    // CREAM is picky about end time of delegated credentials, so
    // make sure it does not exceed end time of signer
    Credential proxy_cred(start_time,end_time-start_time);
    proxy_cred.InquireRequest(proxyRequestStr);
    proxy_cred.SetProxyPolicy("gsi2", "", "", -1);

    if (!(signer.SignRequest(&proxy_cred, signedCert))) {
      logger.msg(VERBOSE, "Failed signing certificate request");
      return false;
    }

    std::string signedOutputCert, signedOutputCertChain;
    signer.OutputCertificate(signedOutputCert);
    signer.OutputCertificateChain(signedOutputCertChain);
    signedCert.append(signedOutputCert).append(signedOutputCertChain);

    action = "putProxy";
    req = PayloadSOAP(cream_ns);
    XMLNode putProxyRequest = req.NewChild("deleg:" + action);
    putProxyRequest.NewChild("delegationID") = delegation_id;
    putProxyRequest.NewChild("proxy") = signedCert;

    response = XMLNode();
    if (!process(req, response, "http://www.gridsite.org/namespaces/delegation-2/"))
      return false;

    if (!response) {
      logger.msg(VERBOSE, "Failed putting signed delegation certificate to service");
      return false;
    }

    return true;
  }
} // namespace Arc
