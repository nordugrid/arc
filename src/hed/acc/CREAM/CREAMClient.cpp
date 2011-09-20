// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/DateTime.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/JobDescription.h>
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
    if (timestring == "" || timestring.length() < 15)
      return "";

    //The conversion for example:
    //before: 11/5/08 11:52 PM
    //after:  2008-11-05T23:52:00.000Z

    tm timestr;
    std::string::size_type pos = 0;
    if (sscanf(timestring.substr(pos, 8).c_str(),
               "%2d/%2d/%2d",
               &timestr.tm_mon,
               &timestr.tm_mday,
               &timestr.tm_year) == 3)
      pos += 8;
    else if (sscanf(timestring.substr(pos, 7).c_str(),
                    "%2d/%d/%2d",
                    &timestr.tm_mon,
                    &timestr.tm_mday,
                    &timestr.tm_year) == 3)
      pos += 7;
    else if (sscanf(timestring.substr(pos, 7).c_str(),
                    "%d/%2d/%2d",
                    &timestr.tm_mon,
                    &timestr.tm_mday,
                    &timestr.tm_year) == 3)
      pos += 7;
    else if (sscanf(timestring.substr(pos, 6).c_str(),
                    "%d/%d/%2d",
                    &timestr.tm_mon,
                    &timestr.tm_mday,
                    &timestr.tm_year) == 3)
      pos += 6;
    else
      return false;

    timestr.tm_year += 100;
    timestr.tm_mon--;

    if (timestring[pos] == 'T' || timestring[pos] == ' ')
      pos++;

    if (sscanf(timestring.substr(pos, 5).c_str(),
               "%2d:%2d",
               &timestr.tm_hour,
               &timestr.tm_min) == 2)
      pos += 5;
    else
      return false;

    // skip the space characters
    while (timestring[pos] == ' ')
      pos++;

    if (timestring.substr(pos, pos + 2) == "PM")
      timestr.tm_hour += 12;

    time.SetTime(mktime(&timestr));
    return true;
  }

  static void set_cream_namespaces(NS& ns) {
    ns["deleg"] = "http://www.gridsite.org/namespaces/delegation-2";
    ns["types"] = "http://glite.org/2007/11/ce/cream/types";
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

  bool CREAMClient::process(PayloadSOAP& req, XMLNode& response) {
    if (!client) {
      logger.msg(VERBOSE, "CREAMClient not created properly");
      return false;
    }

    PayloadSOAP *resp = NULL;
    if (!client->process("http://glite.org/2007/11/ce/cream/" + action, &req, &resp)) {
      logger.msg(VERBOSE, "%s request failed", action);
      return false;
    }

    if (resp == NULL) {
      logger.msg(VERBOSE, "There was no SOAP response");
      return false;
    }

    if ((*resp)[action + "Response"]["result"])
      (*resp)[action + "Response"]["result"].New(response);
    else
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
    XMLNode jobStatusRequest = req.NewChild("types:" + action). // Should not be concatenated with "Request", as in the other requests.
                                   NewChild("types:jobId").
                                   NewChild("types:id") = jobid;
    if (!delegationId.empty())
      jobStatusRequest.NewChild("types:delegationProxyId") = delegationId;

    XMLNode response;
    if (!process(req, response))
      return false;

    XMLNode jobInfoNode;
    jobInfoNode = response["jobInfo"];

    XMLNode lastStatusNode = jobInfoNode.Path("status").back();

    if (lastStatusNode["name"])
      job.State = JobStateCREAM((std::string)lastStatusNode["name"]);

    if (!job.State) {
      logger.msg(VERBOSE, "Unable to retrieved job status.");
      return false;
    }

    if (ISVALID(jobInfoNode["jobId"]["id"]) && ISVALID(jobInfoNode["jobId"]["creamURL"])) {
      job.IDFromEndpoint = URL((std::string)jobInfoNode["jobId"]["creamURL"] + "/" + (std::string)jobInfoNode["jobId"]["id"]);
    }
    if (ISVALID(jobInfoNode["jobId"]["id"]))
      job.LocalIDFromManager = (std::string)jobInfoNode["jobId"]["id"];
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
    if (job.State() == "DONE-FAILED")
      job.Error.push_back((std::string)lastStatusNode["failureReason"]);
    if (ISVALID(jobInfoNode["delegationProxyInfo"])) {
      std::string delegationProxy = (std::string)jobInfoNode["delegationProxyInfo"];
      std::list<std::string> splited_proxy;
      tokenize(delegationProxy, splited_proxy, "\n");
      for (std::list<std::string>::iterator it = splited_proxy.begin();
           it != splited_proxy.end(); it++) {
        if (it->find("Holder Subject") < it->find(":"))
          job.Owner = it->substr(it->find_first_of(":") + 1);
        if (it->find("Valid To") < it->find(":")) {
          Time time;
          if (stringtoTime(it->substr(it->find_first_of(":") + 2), time) && time.GetTime() != -1)
            job.ProxyExpirationTime = time;
        }
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
    req.NewChild("types:J" + action + "Request").NewChild("types:jobId").NewChild("types:id") = jobid;

    XMLNode response;
    if (!process(req, response))
      return false;

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
    req.NewChild("types:" + action + "Request").NewChild("types:jobId").NewChild("types:id") = jobid;

    XMLNode response;
    if (!process(req, response))
      return false;

    if (!response) {
      logger.msg(VERBOSE, "Empty response");
      return false;
    }

    return true;
  }

  bool CREAMClient::registerJob(const std::string& jdl_text,
                                creamJobInfo& info) {
    logger.msg(VERBOSE, "Creating and sending job register request");

    action = "JobRegister";

    PayloadSOAP req(cream_ns);
    XMLNode act_job = req.NewChild("types:" + action + "Request").NewChild("types:JobDescriptionList");
    act_job.NewChild("types:JDL") = jdl_text;
    act_job.NewChild("types:autoStart") = "false";
    if (!delegationId.empty())
      act_job.NewChild("types:delegationId") = delegationId;

    XMLNode response;
    if (!process(req, response))
      return false;

    if (!response) {
      logger.msg(VERBOSE, "Empty response");
      return false;
    }

    if (!response["jobId"]["id"]) {
      logger.msg(VERBOSE, "No job ID in response");
      return false;
    }

    info.jobId = (std::string)response["jobId"]["id"];
    if (response["jobId"]["creamURL"])
      info.creamURL = URL((std::string)response["jobId"]["creamURL"]);
    for (XMLNode property = response["jobId"]["property"]; property; ++property) {
      if ((std::string)property["name"] == "CREAMInputSandboxURI")
        info.ISB_URI = (std::string)property["value"];
      else if ((std::string)property["name"] == "CREAMOutputSandboxURI")
        info.OSB_URI = (std::string)property["value"];
    }

    return true;
  }

  bool CREAMClient::startJob(const std::string& jobid) {
    logger.msg(VERBOSE, "Creating and sending job start request");

    action = "JobStart";

    PayloadSOAP req(cream_ns);
    XMLNode jobStartRequest = req.NewChild("types:" + action + "Request");
    jobStartRequest.NewChild("types:jobId").NewChild("types:id") = jobid;
    if (!delegationId.empty())
      jobStartRequest.NewChild("types:delegationId") = delegationId;

    XMLNode response;
    if (!process(req, response))
      return false;

    if (!response) {
      logger.msg(VERBOSE, "Empty response");
      return false;
    }

    if (!response["jobId"]["id"]) {
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
    if (!process(req, response))
      return false;

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
    if (!process(req, response))
      return false;

    if (!response) {
      logger.msg(VERBOSE, "Failed putting signed delegation certificate to service");
      return false;
    }

    return true;
  }

  bool CREAMClient::destroyDelegation(const std::string& delegation_id) {
    logger.msg(VERBOSE, "Creating delegation");

    action = "destroy";

    PayloadSOAP req(cream_ns);
    req.NewChild("deleg:" + action).NewChild("delegationID") = delegation_id;

    XMLNode response;
    if (!process(req, response))
      return false;

    if (!response) {
      logger.msg(VERBOSE, "Empty response");
      return false;
    }

    return true;
  }
} // namespace Arc
