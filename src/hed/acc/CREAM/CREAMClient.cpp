#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/miscutils.h>
#include <vector>
#include <string>


#include <arc/client/ClientInterface.h>
#include <arc/message/MCC.h>
#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/DateTime.h>
#include <arc/client/JobDescription.h>
#include <arc/StringConv.h>

#include "CREAMClient.h"
#include "OpenSSLFunctions.h"

namespace Arc {

  Logger CREAMClient::logger(Logger::rootLogger, "CREAM-Client");
  Logger local_logger(Logger::rootLogger, "CREAM-Client");

  bool Setting_Checker(const XMLNode node) {
    if ( !node || (std::string)node == "N/A" || (std::string)node == "[reserved]")
       return false;
    return true;
  }

  //This is splits the string str by any char appears in delim and put it into the results vector.  
  void StringSplit(std::string str, std::string delim, std::vector<std::string>& results, bool rem_wsp=true){
    if (rem_wsp){
       //replace the whitespace characters (\n) with " "
       size_t found;
       std::string whitespaces("\n");
    
       found=str.find_first_of( whitespaces );
       while (found!=std::string::npos){
 	   str[found]=' ';
	   found=str.find_first_of( whitespaces ,found+1);
       }
    }
		    
    //split the string   
    int cutAt;
    while( (cutAt = str.find_first_of(delim)) != (int)str.npos ){
       if(cutAt > 0)
         results.push_back(str.substr(0,cutAt));

       str = str.substr(cutAt+1);
    }

    if (str.length() > 0)
       results.push_back(str);
  }

  //This funtion is convert from the Proxy time to valid Time string.
  std::string Str_to_TimeStr( std::string timestring ){
    if ( timestring == "" || timestring.length() < 16 )
       return "";

    //The conversion for example:
    //before: 11/5/08 11:52 PM
    //after:  2008-11-05T23:52:00.000Z
    
    int year;
    int month;
    int day;
    int hour;
    int min;
    std::string AMorPM;
    std::string::size_type pos = 0;
    if(sscanf(timestring.substr(pos, 8).c_str(),
              "%2d/%2d/%2d",
	      &month,
	      &day,
	      &year) == 3)
       pos += 8;
    else if(sscanf(timestring.substr(pos, 7).c_str(),
              "%2d/%d/%2d",
	      &month,
    	      &day,
	      &year) == 3)
       pos += 7;
    else if(sscanf(timestring.substr(pos, 7).c_str(),
              "%d/%2d/%2d",
	      &month,
    	      &day,
	      &year) == 3)
       pos += 7;
    else if(sscanf(timestring.substr(pos, 6).c_str(),
              "%d/%d/%2d",
	      &month,
    	      &day,
	      &year) == 3)
       pos += 6;
    else {
       Arc::local_logger.msg(ERROR, "Can not parse date: %s", timestring);
       return "";
    }									    
  
    if(timestring[pos] == 'T' || timestring[pos] == ' ') pos++;
     
    if(sscanf(timestring.substr(pos, 5).c_str(),
 	     "%2d:%2d",
	     &hour,
	     &min) == 2)
       pos += 5;
    else {
       Arc::local_logger.msg(ERROR, "Can not parse time: %s", timestring);
       return "";
    }

    // skip the space characters
    if(timestring[pos] == ' ') {
	pos++;
	while(timestring[pos] == ' ') pos++;
    }

    AMorPM = timestring.substr(pos,pos+2);										    
  
    year += 2000;
    
    if ( AMorPM == "PM")
       hour += 12;

    std::ostringstream oss;
    oss << year << "-" << month << "-" << day << "T" << hour << ":" << min << ":00.000Z";
    std::string result(oss.str());
    
    return result;
  }

  static void set_cream_namespaces(NS& ns) {
    ns["SOAP-ENV"] = "http://schemas.xmlsoap.org/soap/envelope/";
    ns["SOAP-ENC"] = "http://schemas.xmlsoap.org/soap/encoding/";
    ns["xsi"] = "http://www.w3.org/2001/XMLSchema-instance";
    ns["xsd"] = "http://www.w3.org/2001/XMLSchema";
    ns["ns1"] = "http://www.gridsite.org/namespaces/delegation-2";
    ns["ns2"] = "http://glite.org/2007/11/ce/cream/types";
    ns["ns3"] = "http://glite.org/2007/11/ce/cream";
  }

  CREAMClient::CREAMClient(const URL& url, const MCCConfig& cfg)
    : client(NULL) {
    logger.msg(INFO, "Creating a CREAM client");
    client = new ClientSOAP(cfg, url);
    set_cream_namespaces(cream_ns);
  }

  CREAMClient::~CREAMClient() {
    if (client)
      delete client;
  }

  bool CREAMClient::stat(const std::string& jobid, Job& job) {
    logger.msg(INFO, "Creating and sending a status request");

    PayloadSOAP req(cream_ns);
    NS ns2;
    ns2["ns2"] = "http://glite.org/2007/11/ce/cream/types";
    XMLNode jobStatusRequest = req.NewChild("ns2:JobInfo", ns2);
    XMLNode jobId = jobStatusRequest.NewChild("ns2:jobId", ns2);
    XMLNode id = jobId.NewChild("ns2:id", ns2);
    id.Set(jobid);
    if (this->delegationId != "") {
      XMLNode delegId =
	jobStatusRequest.NewChild("ns2:delegationProxyId", ns2);
      delegId.Set(this->delegationId);
    }

    // Send status request
    PayloadSOAP *resp = NULL;

    if (client) {
      MCC_Status MCC_status =
	client->process("http://glite.org/2007/11/ce/cream/JobInfo",
			&req, &resp);
      if (resp == NULL) {
	logger.msg(ERROR, "There was no SOAP response");
	return false;
      }
    }
    else {
      logger.msg(ERROR, "There is no connection chain configured");
      return false;
    }

    XMLNode failureReason, fault;
    if ((*resp)["JobInfoResponse"]["result"]["jobInfo"]["failureReason"])
      (*resp)["JobInfoResponse"]["result"]["jobInfo"]["failureReason"].New(failureReason);
    std::string faultstring = (std::string)failureReason;

    int last_status_id = 0;
    while (true) {
      if (!((*resp)["JobInfoResponse"]["result"]["jobInfo"]["status"][last_status_id])) break;
      last_status_id++;
    }

    //Set the job's attributes
    if (last_status_id > 0)
       job.State = (std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["status"][last_status_id-1]["name"];

    if (Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["jobId"]["id"]) &&
        Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["jobId"]["creamURL"]) ){
       Arc::URL url_obj( (std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["jobId"]["creamURL"] + "/" + (std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["jobId"]["id"] );
       job.JobID = url_obj;
    }

    if (Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["type"]))
       job.Type = (std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["type"];

    if (Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["jobId"]["creamURL"]) ){
       Arc::URL url_obj( (std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["jobId"]["creamURL"] );
       job.IDFromEndpoint = url_obj;
    }

    if (Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["jobId"]["id"]) )
       job.LocalIdFromManager = (std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["jobId"]["id"];

    if (Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["JDL"])){
       job.JobDescription = (std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["JDL"];
       
       Arc::JobDescription jd;
       if (jd.setSource(job.JobDescription) && jd.isValid()){
          Arc::XMLNode jd_xml;
          //jd_xml = jd.getXML();
          if ( !jd.getXML(jd_xml) ){
             std::cerr << "The JobDescription was empty." << std::endl;
             return false;
          }           
          if ( (bool)(jd_xml["JobDescription"]["Application"]["POSIXApplication"]["Input"]) &&
              (std::string)jd_xml["JobDescription"]["Application"]["POSIXApplication"]["Input"] != "" )        
             job.StdIn = (std::string)jd_xml["JobDescription"]["Application"]["POSIXApplication"]["Input"];

          if ( (bool)(jd_xml["JobDescription"]["Application"]["POSIXApplication"]["Output"]) &&
              (std::string)jd_xml["JobDescription"]["Application"]["POSIXApplication"]["Output"] != "" )        
             job.StdOut = (std::string)jd_xml["JobDescription"]["Application"]["POSIXApplication"]["Output"];

          if ( (bool)(jd_xml["JobDescription"]["Application"]["POSIXApplication"]["Error"]) &&
              (std::string)jd_xml["JobDescription"]["Application"]["POSIXApplication"]["Error"] != "" )        
             job.StdErr = (std::string)jd_xml["JobDescription"]["Application"]["POSIXApplication"]["Error"];

          if ( (bool)(jd_xml["JobDescription"]["Resources"]["CandidateTarget"]["QueueName"]) &&
              (std::string)jd_xml["JobDescription"]["Resources"]["CandidateTarget"]["QueueName"] != "" )        
             job.Queue = (std::string)jd_xml["JobDescription"]["Resources"]["CandidateTarget"]["QueueName"];

/*           job.RequestedWallTime =
             job.RequestedTotalCPUTime =
             job.RequestedMainMemory =
             job.RequestedSlots =
*/
       }
    }

    if (Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["status"][last_status_id-1]["exitCode"]))
       job.ExitCode = stringtoi((*resp)["JobInfoResponse"]["result"]["jobInfo"]["status"][last_status_id-1]["exitCode"]);

    if (Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["status"][last_status_id-1]["name"]) &&
        (std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["status"][last_status_id-1]["name"] == "DONE-FAILED")
       job.Error.push_back((std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["status"][last_status_id-1]["failureReason"]);

    if (Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["delegationProxyInfo"])){
       std::string delegationProxy = (std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["delegationProxyInfo"];
       std::vector<std::string> splited_proxy;
       Arc::StringSplit(delegationProxy, "\n", splited_proxy, false);
       std::vector<std::string>::iterator it;
       for ( it=splited_proxy.begin(); it < splited_proxy.end(); it++ ){
           if ( (*it).find("Holder Subject") < (*it).find(":") ) {
              job.Owner = (*it).substr( (*it).find_first_of(":")+1 ); 
	   }
           if ( (*it).find("Valid To") < (*it).find(":") ) {
              Arc::Time time( Arc::Str_to_TimeStr( (*it).substr( (*it).find_first_of(":")+2 ) ) );
	      if (time.GetTime() != -1)
	         job.ProxyExpirationTime = time;
	   }
       }
    }

    if (Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["localUser"]))
       job.LocalOwner = (std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["localUser"];
       
    if (Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["jobId"]["creamURL"]) ){
       job.ExecutionCE = (std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["jobId"]["creamURL"];
       job.JobManagementEndpoint = (std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["jobId"]["creamURL"];
    }

    if (Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["lastCommand"]) ){
       int job_register_id_first = -1;
       int job_register_id_last = -1;
       int job_start_id_first = -1;
       int job_start_id_last = -1;
       int local_id = 0;
       while (true) {
         if (!((*resp)["JobInfoResponse"]["result"]["jobInfo"]["lastCommand"][local_id])) break;
	 if ((std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["lastCommand"][local_id]["name"] == "JOB_REGISTER"){
	    if ( job_register_id_first == -1 && job_register_id_last == -1 ){
	       job_register_id_first = local_id;
	       job_register_id_last = local_id;
	    }
	    else if ( job_register_id_last > -1 ){
	       job_register_id_last = local_id;
	    }
	 } //end of the JOB_REGISTER      

	 if ((std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["lastCommand"][local_id]["name"] == "JOB_START"){
	    if ( job_start_id_first == -1 && job_start_id_last == -1 ){
	       job_start_id_first = local_id;
	       job_start_id_last = local_id;
	    }
	    else if ( job_start_id_last > -1 ){
	       job_start_id_last = local_id;
	    }
	 } //end of the JOB_START     
         local_id++;
       }
       	 
       //dependent from the JOB_REGISTER
       if ( job_register_id_first > -1 ){      
          if (Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["lastCommand"][job_register_id_first]["creationTime"])){
              Arc::Time time((std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["lastCommand"][job_register_id_first]["creationTime"]);
	      if (time.GetTime() != -1)
                 job.SubmissionTime = time;
          }
       }

       if ( job_register_id_last > -1 ){      
          if (Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["lastCommand"][job_register_id_last]["creationTime"])){
              Arc::Time time((std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["lastCommand"][job_register_id_last]["creationTime"]);
	      if (time.GetTime() != -1)
                 job.CreationTime = time;
          }
       }//end of the JOB_REGISTER
       
       //dependent from the JOB_START
       if ( job_start_id_first > -1 ){                   
          if (Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["lastCommand"][job_start_id_first]["startSchedulingTime"])){
              Arc::Time time((std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["lastCommand"][job_start_id_first]["startSchedulingTime"]);
	      if (time.GetTime() != -1)
                 job.ComputingManagerSubmissionTime = time;
          }

          if (Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["lastCommand"][job_start_id_first]["startProcessingTime"])){
              Arc::Time time((std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["lastCommand"][job_start_id_first]["startProcessingTime"]);
	      if (time.GetTime() != -1)
                 job.StartTime = time;
          }
       }

       if ( job_start_id_last > -1 ){            
          if (Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["lastCommand"][job_start_id_last]["executionCompletedTime"])){
              Arc::Time time((std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["lastCommand"][job_start_id_last]["executionCompletedTime"]);
	      if (time.GetTime() != -1)
                 job.ComputingManagerEndTime = time;
          }
       }//end of the JOB_START     
    } //end of the LastCommand
    
    if (Setting_Checker((*resp)["JobInfoResponse"]["result"]["jobInfo"]["status"][last_status_id-1]["timestamp"]) &&
        ((std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["status"][last_status_id-1]["name"] == "DONE-OK" ||
	 (std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["status"][last_status_id-1]["name"] == "DONE-FAILED") ){
       Arc::Time time((std::string)(*resp)["JobInfoResponse"]["result"]["jobInfo"]["status"][last_status_id-1]["timestamp"]);
       if (time.GetTime() != -1)
          job.EndTime = time;
    }

    if ((*resp)["JobInfoResponse"]["result"]["JobUnknownFault"])
      (*resp)["JobInfoResponse"]["result"]["JobUnknownFault"].New(fault);
    if ((*resp)["JobInfoResponse"]["result"]["JobStatusInvalidFault"])
      (*resp)["JobInfoResponse"]["result"]["JobStatusInvalidFault"].New(fault);
    if ((*resp)["JobInfoResponse"]["result"]["DelegationIdMismatchFault"])
      (*resp)["JobInfoResponse"]["result"]["DelegationIdMismatchFault"].New(fault);
    if ((*resp)["JobInfoResponse"]["result"]["DateMismatchFault"])
      (*resp)["JobInfoResponse"]["result"]["DateMismatchFault"].New(fault);
    if ((*resp)["JobInfoResponse"]["result"]["LeaseIdMismatchFault"])
      (*resp)["JobInfoResponse"]["result"]["LeaseIdMismatchFault"].New(fault);
    if ((*resp)["JobInfoResponse"]["result"]["GenericFault"])
      (*resp)["JobInfoResponse"]["result"]["GenericFault"].New(fault);
    delete resp;
    if ((bool)fault) {
      logger.msg(ERROR, (std::string)(fault["Description"]));
      return false;
    }
    if (job.State == "") {
      logger.msg(ERROR, "The job status could not be retrieved");
      return false;
    }
    else
      return true;
  }

  bool CREAMClient::cancel(const std::string& jobid) {
    logger.msg(INFO, "Creating and sending request to terminate a job");

    PayloadSOAP req(cream_ns);
    NS ns2;
    ns2["ns2"] = "http://glite.org/2007/11/ce/cream/types";
    XMLNode jobCancelRequest = req.NewChild("ns2:JobCancelRequest", ns2);
    XMLNode jobId = jobCancelRequest.NewChild("ns2:jobId", ns2);
    XMLNode id = jobId.NewChild("ns2:id", ns2);
    id.Set(jobid);

    // Send cancel request
    PayloadSOAP *resp = NULL;
    if (client) {
      MCC_Status status = client->process("http://glite.org/2007/11/ce/cream/JobCancel", &req, &resp);
      if (resp == NULL) {
	logger.msg(ERROR, "There was no SOAP response");
	return false;
      }
    }
    else {
      logger.msg(ERROR, "There is no connection chain configured");
      return false;
    }

    XMLNode cancelled, fault;
    (*resp)["JobCancelResponse"]["result"]["jobId"]["id"].New(cancelled);
    std::string result = (std::string)cancelled;
    if ((*resp)["JobCancelResponse"]["result"]["JobUnknownFault"])
      (*resp)["JobCancelResponse"]["result"]["JobUnknownFault"].New(fault);
    if ((*resp)["JobCancelResponse"]["result"]["JobStatusInvalidFault"])
      (*resp)["JobCancelResponse"]["result"]["JobStatusInvalidFault"].New(fault);
    if ((*resp)["JobCancelResponse"]["result"]["DelegationIdMismatchFault"])
      (*resp)["JobCancelResponse"]["result"]["DelegationIdMismatchFault"].New(fault);
    if ((*resp)["JobCancelResponse"]["result"]["DateMismatchFault"])
      (*resp)["JobCancelResponse"]["result"]["DateMismatchFault"].New(fault);
    if ((*resp)["JobCancelResponse"]["result"]["LeaseIdMismatchFault"])
      (*resp)["JobCancelResponse"]["result"]["LeaseIdMismatchFault"].New(fault);
    if ((*resp)["JobCancelResponse"]["result"]["GenericFault"])
      (*resp)["JobCancelResponse"]["result"]["GenericFault"].New(fault);
    delete resp;
    if ((bool)fault) {
      logger.msg(ERROR, (std::string)(fault["Description"]));
      return false;
    }
    if (result == "") {
      logger.msg(ERROR, "Job termination failed");
      return false;
    }
    return true;
  }

  bool CREAMClient::purge(const std::string& jobid) {
    logger.msg(INFO, "Creating and sending request to clean a job");

    PayloadSOAP req(cream_ns);
    NS ns2;
    ns2["ns2"] = "http://glite.org/2007/11/ce/cream/types";
    XMLNode jobStatusRequest = req.NewChild("ns2:JobPurgeRequest", ns2);
    XMLNode jobId = jobStatusRequest.NewChild("ns2:jobId", ns2);
    XMLNode id = jobId.NewChild("ns2:id", ns2);
    id.Set(jobid);
    XMLNode creamURL = jobId.NewChild("ns2:creamURL", ns2);

    // Send clean request
    PayloadSOAP *resp = NULL;
    if (client) {
      MCC_Status status =
	client->process("http://glite.org/2007/11/ce/cream/JobPurge",
			&req, &resp);
      if (resp == NULL) {
	logger.msg(ERROR, "There was no SOAP response");
	return false;
      }
    }
    else {
      logger.msg(ERROR, "There is no connection chain configured");
      return false;
    }

    XMLNode cancelled, fault;
    (*resp)["JobPurgeResponse"]["result"]["jobId"]["id"].New(cancelled);
    std::string result = (std::string)cancelled;
    if ((*resp)["JobPurgeResponse"]["result"]["JobUnknownFault"])
      (*resp)["JobPurgeResponse"]["result"]["JobUnknownFault"].New(fault);
    if ((*resp)["JobPurgeResponse"]["result"]["JobStatusInvalidFault"])
      (*resp)["JobPurgeResponse"]["result"]["JobStatusInvalidFault"].New(fault);
    if ((*resp)["JobPurgeResponse"]["result"]["DelegationIdMismatchFault"])
      (*resp)["JobPurgeResponse"]["result"]["DelegationIdMismatchFault"].New(fault);
    if ((*resp)["JobPurgeResponse"]["result"]["DateMismatchFault"])
      (*resp)["JobPurgeResponse"]["result"]["DateMismatchFault"].New(fault);
    if ((*resp)["JobPurgeResponse"]["result"]["LeaseIdMismatchFault"])
      (*resp)["JobPurgeResponse"]["result"]["LeaseIdMismatchFault"].New(fault);
    if ((*resp)["JobPurgeResponse"]["result"]["GenericFault"])
      (*resp)["JobPurgeResponse"]["result"]["GenericFault"].New(fault);
    delete resp;
    if ((bool)fault) {
      logger.msg(ERROR, (std::string)(fault["Description"]));
      return false;
    }
    if (result == "") {
      logger.msg(ERROR, "Job cleaning failed");
      return false;
    }
    return true;
  }

  bool CREAMClient::registerJob(const std::string& jdl_text,
				creamJobInfo& info) {
    logger.msg(INFO, "Creating and sending job register request");

    PayloadSOAP req(cream_ns);
    NS ns2;
    ns2["ns2"] = "http://glite.org/2007/11/ce/cream/types";
    XMLNode jobRegisterRequest = req.NewChild("ns2:JobRegisterRequest", ns2);
    XMLNode act_job =
      jobRegisterRequest.NewChild("ns2:JobDescriptionList", ns2);
    XMLNode jdl_node = act_job.NewChild("ns2:JDL", ns2);
    jdl_node.Set(jdl_text);
    if (this->delegationId != "") {
      XMLNode delegId = act_job.NewChild("ns2:delegationId", ns2);
      delegId.Set(this->delegationId);
    }
    XMLNode autostart_node = act_job.NewChild("ns2:autoStart", ns2);
    autostart_node.Set("false");
    PayloadSOAP *resp = NULL;
    // Send job request
    if (client) {
      MCC_Status status =
	client->process("http://glite.org/2007/11/ce/cream/JobRegister",
			&req, &resp);
      if (!status) {
	logger.msg(ERROR, "Submission request failed");
	return false;
      }
      if (resp == NULL) {
	logger.msg(ERROR, "There was no SOAP response");
	return false;
      }
    }
    else {
      logger.msg(ERROR, "There is no connection chain configured");
      return false;
    }
    XMLNode id, fault;
    (*resp)["JobRegisterResponse"]["result"]["jobId"]["id"].New(id);

    std::string result = (std::string)id;
    if ((*resp)["JobRegisterResponse"]["result"]["JobUnknownFault"])
      (*resp)["JobRegisterResponse"]["result"]["JobUnknownFault"].New(fault);
    if ((*resp)["JobRegisterResponse"]["result"]["JobStatusInvalidFault"])
      (*resp)["JobRegisterResponse"]["result"]["JobStatusInvalidFault"].New(fault);
    if ((*resp)["JobRegisterResponse"]["result"]["DelegationIdMismatchFault"])
      (*resp)["JobRegisterResponse"]["result"]["DelegationIdMismatchFault"].New(fault);
    if ((*resp)["JobRegisterResponse"]["result"]["DateMismatchFault"])
      (*resp)["JobRegisterResponse"]["result"]["DateMismatchFault"].New(fault);
    if ((*resp)["JobRegisterResponse"]["result"]["LeaseIdMismatchFault"])
      (*resp)["JobRegisterResponse"]["result"]["LeaseIdMismatchFault"].New(fault);
    if ((*resp)["JobRegisterResponse"]["result"]["GenericFault"])
      (*resp)["JobRegisterResponse"]["result"]["GenericFault"].New(fault);

    // Create the return value
    XMLNode property;
    if ((*resp)["JobRegisterResponse"]["result"]["jobId"]["creamURL"])
      info.creamURL = (std::string)((*resp)["JobRegisterResponse"]["result"]["jobId"]["creamURL"]);
    property = (*resp)["JobRegisterResponse"]["result"]["jobId"]["property"];
    while ((bool)property) {
      if ((std::string)(property["name"]) == "CREAMInputSandboxURI")
	info.ISB_URI = (std::string)(property["value"]);
      else if ((std::string)(property["name"]) == "CREAMOutputSandboxURI")
	info.OSB_URI = (std::string)(property["value"]);
      ++property;
    }

    delete resp;
    if ((bool)fault) {
      logger.msg(ERROR, (std::string)(fault["Description"]));
      return false;
    }
    if (result == "") {
      logger.msg(ERROR, "No job ID has been received");
      return false;
    }
    info.jobId = result;
    return true;
  }

  bool CREAMClient::startJob(const std::string& jobid) {
    logger.msg(INFO, "Creating and sending job start request");

    PayloadSOAP req(cream_ns);
    NS ns2;
    ns2["ns2"] = "http://glite.org/2007/11/ce/cream/types";
    XMLNode jobStartRequest = req.NewChild("ns2:JobStartRequest", ns2);
    XMLNode jobId = jobStartRequest.NewChild("ns2:jobId", ns2);
    XMLNode id_node = jobId.NewChild("ns2:id", ns2);
    id_node.Set(jobid);
    if (this->delegationId != "") {
      XMLNode delegId = jobStartRequest.NewChild("ns2:delegationId", ns2);
      delegId.Set(this->delegationId);
    }
    PayloadSOAP *resp = NULL;

    // Send job request
    if (client) {
      MCC_Status status =
	client->process("http://glite.org/2007/11/ce/cream/JobStart",
			&req, &resp);
      if (!status) {
	logger.msg(ERROR, "Submission request failed");
	return false;
      }
      if (resp == NULL) {
	logger.msg(ERROR, "There was no SOAP response");
	return false;
      }
    }
    else {
      logger.msg(ERROR, "There is no connection chain configured");
      return false;
    }
    XMLNode id, fault;
    (*resp)["JobStartResponse"]["result"]["jobId"]["id"].New(id);

    std::string result = (std::string)id;
    if ((*resp)["JobStartResponse"]["result"]["JobUnknownFault"])
      (*resp)["JobStartResponse"]["result"]["JobUnknownFault"].New(fault);
    if ((*resp)["JobStartResponse"]["result"]["JobStatusInvalidFault"])
      (*resp)["JobStartResponse"]["result"]["JobStatusInvalidFault"].New(fault);
    if ((*resp)["JobStartResponse"]["result"]["DelegationIdMismatchFault"])
      (*resp)["JobStartResponse"]["result"]["DelegationIdMismatchFault"].New(fault);
    if ((*resp)["JobStartResponse"]["result"]["DateMismatchFault"])
      (*resp)["JobStartResponse"]["result"]["DateMismatchFault"].New(fault);
    if ((*resp)["JobStartResponse"]["result"]["LeaseIdMismatchFault"])
      (*resp)["JobStartResponse"]["result"]["LeaseIdMismatchFault"].New(fault);
    if ((*resp)["JobStartResponse"]["result"]["GenericFault"])
      (*resp)["JobStartResponse"]["result"]["GenericFault"].New(fault);
    delete resp;
    if ((bool)fault) {
      logger.msg(ERROR, (std::string)(fault["Description"]));
      return false;
    }
    if (result == "") {
      logger.msg(ERROR, "Job starting failed");
      return false;
    }
    return true;
  }

  bool CREAMClient::createDelegation(const std::string& delegation_id,
				     const std::string& proxy) {
    logger.msg(INFO, "Creating delegation");

    PayloadSOAP req(cream_ns);
    NS ns1;
    ns1["ns1"] = "http://www.gridsite.org/namespaces/delegation-2";
    XMLNode getProxyReqRequest = req.NewChild("ns1:getProxyReq", ns1);
    XMLNode delegid = getProxyReqRequest.NewChild("delegationID", ns1);
    delegid.Set(delegation_id);
    PayloadSOAP *resp = NULL;

    // Send job request
    if (client) {
      MCC_Status status = client->process("", &req, &resp);
      if (!status) {
	logger.msg(ERROR, "Submission request failed");
	return false;
      }
      if (resp == NULL) {
	logger.msg(ERROR, "There was no SOAP response");
	return false;
      }
    }
    else {
      logger.msg(ERROR, "There is no connection chain configured");
      return false;
    }

    std::string getProxyReqReturnValue;

    if ((bool)(*resp) &&
	(bool)((*resp)["getProxyReqResponse"]["getProxyReqReturn"]) &&
	((std::string)(*resp)["getProxyReqResponse"]["getProxyReqReturn"]
	 != ""))
      getProxyReqReturnValue =
	(std::string)(*resp)["getProxyReqResponse"]["getProxyReqReturn"];
    else {
      logger.msg(ERROR, "Creating delegation failed");
      return false;
    }
    delete resp;

    std::string signedcert;
    char *cert = NULL;
    int timeleft = getCertTimeLeft(proxy);

    if (makeProxyCert(&cert, (char*)getProxyReqReturnValue.c_str(),
		      (char*)proxy.c_str(), (char*)proxy.c_str(), timeleft)) {
      logger.msg(ERROR, "DelegateProxy failed");
      return false;
    }
    signedcert.assign(cert);

    PayloadSOAP req2(cream_ns);
    XMLNode putProxyRequest = req2.NewChild("ns1:putProxy", ns1);
    XMLNode delegid_node = putProxyRequest.NewChild("delegationID", ns1);
    delegid_node.Set(delegation_id);
    XMLNode proxy_node = putProxyRequest.NewChild("proxy", ns1);
    proxy_node.Set(signedcert);
    resp = NULL;

    // Send job request
    if (client) {
      MCC_Status status = client->process("", &req2, &resp);
      if (!status) {
	logger.msg(ERROR, "Submission request failed");
	return false;
      }
      if (resp == NULL) {
	logger.msg(ERROR, "There was no SOAP response");
	return false;
      }
    }
    else {
      logger.msg(ERROR, "There is no connection chain configured");
      return false;
    }

    if (!(bool)(*resp) || !(bool)((*resp)["putProxyResponse"])) {
      logger.msg(ERROR, "Creating delegation failed");
      return false;
    }
    delete resp;
    return true;

  }

  bool CREAMClient::destroyDelegation(const std::string& delegation_id) {
    logger.msg(INFO, "Creating delegation");

    PayloadSOAP req(cream_ns);
    NS ns1;
    ns1["ns1"] = "http://www.gridsite.org/namespaces/delegation-2";
    XMLNode getProxyReqRequest = req.NewChild("ns1:destroy", ns1);
    XMLNode delegid = getProxyReqRequest.NewChild("delegationID", ns1);
    delegid.Set(delegation_id);
    PayloadSOAP *resp = NULL;

    // Send job request
    if (client) {
      MCC_Status status = client->process("", &req, &resp);
      if (!status) {
	logger.msg(ERROR, "Submission request failed");
	return false;
      }
      if (resp == NULL) {
	logger.msg(ERROR, "There was no SOAP response");
	return false;
      }
    }
    else {
      logger.msg(ERROR, "There is no connection chain configured");
      return false;
    }

    std::string getProxyReqReturnValue;

    if (!(bool)(*resp) || !(bool)((*resp)["destroyResponse"])) {
      logger.msg(ERROR, "Destroying delegation failed");
      return false;
    }
    delete resp;
    return true;
  }

} // namespace Arc
