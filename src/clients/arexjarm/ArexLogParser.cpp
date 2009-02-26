#include "ArexLogParser.h"

namespace Arc
{

  int ArexLogParser::parse(const char *filename)
  {
    int count=0;  //number of parsed values

    std::ifstream logfile(filename,std::ios::in);
    std::string line;
    while (logfile.good())
      {
	std::getline(logfile,line);
	int e=line.find('=');
	if (e!=std::string::npos)
	  {
	    count++;
	    std::string key=line.substr(0, e), 
	      value=line.substr(e+1, std::string::npos);
	    joblog[key]=value;
	  }
      }
    logfile.close();
    return count;
  }

  XMLNode ArexLogParser::createUsageRecord(const char *recordid_prefix)
  {
    Arc::NS ns_ur;
    
    //Namespaces defined by OGF
    ns_ur[""]="http://schema.ogf.org/urf/2003/09/urf";
    ns_ur["urf"]="http://schema.ogf.org/urf/2003/09/urf";
    ns_ur["xsd"]="http://www.w3.org/2001/XMLSchema";
    ns_ur["xsi"]="http://www.w3.org/2001/XMLSchema-instance";
    ns_ur["ds"]="http://www.w3.org/2000/09/xmldsig#";
    
    //Fill this Usage Record
    Arc::XMLNode ur(ns_ur,"JobUsageRecord");
    
    //RecordIdentity, GlobalJobId, LocalJobId
    if (joblog.find("ngjobid")!=joblog.end())
      {
	// Timestamp for record, required
	std::string nowstamp=Arc::Time().str(Arc::UTCTime);
	
	Arc::XMLNode rid=ur.NewChild("RecordIdentity");
	rid.NewAttribute("createTime")=nowstamp;
	//NOTE! Current LUTS also sets a "creationTime"[sic!] for each record
	
	// ID for record
	if (joblog.find("nodename")!=joblog.end())
	  rid.NewAttribute("recordId")=
	    std::string(recordid_prefix) + joblog["nodename"] + 
	    '-' + joblog["ngjobid"];
	else
	  rid.NewAttribute("recordId")=
	    std::string(recordid_prefix) + joblog["ngjobid"];
	
	ur.NewChild("JobIdentity").NewChild("GlobalJobId")=
	  joblog["ngjobid"];
	
	if (joblog.find("localjobid")!=joblog.end())
	  ur["JobIdentity"].NewChild("LocalJobId")=
	    joblog["localjobid"];
      }
    else
    {
      //TODO what if not valid?
      throw std::runtime_error("Missing required element \"RecordIdentity\"");
    }
    
    //ProcessId?
    
    //GlobalUser[Nn]ame, LocalUserId
    //TODO clarify case
    //NOTE! original JARM used "GlobalUserId"
    if (joblog.find("usersn")!=joblog.end())
      {
	ur.NewChild("UserIdentity").NewChild("GlobalUserName")=
	  joblog["usersn"];
	
	if (joblog.find("localuser")!=joblog.end())
	  ur["UserIdentity"].NewChild("LocalUserId")=
	    joblog["localuser"];
      }
    
    //JobName
    if (joblog.find("jobname")!=joblog.end())
      {
	ur.NewChild("JobName")=joblog["jobname"];
      }
    
    //Charge?
    
    //Status
    if (joblog.find("status")!=joblog.end())
      {
	ur.NewChild("Status")=joblog["status"];  //TODO convert?
      }
    else
      {
	//TODO what if not valid?
	throw std::runtime_error("Missing required element \"Status\"");
      }
    
    //WallDuration
    if (joblog.find("usedwalltime")!=joblog.end())
      {
	Arc::Period walldur(joblog["usedwalltime"],Arc::PeriodSeconds);
	ur.NewChild("WallDuration")=(std::string)walldur;
      }
    
    //CpuDuration
    if (joblog.find("usedcputime")!=joblog.end())
      {
	Arc::Period cpudur(joblog["usedcputime"],Arc::PeriodSeconds);	
	ur.NewChild("CpuDuration")=(std::string)cpudur;
      }
    
    //StartTime
    if (joblog.find("submissiontime")!=joblog.end())
      {
	Arc::Time starttime(joblog["submissiontime"]);
	ur.NewChild("StartTime")=starttime.str(Arc::UTCTime);
      }
    
    //EndTime
    if (joblog.find("endtime")!=joblog.end())
      {
	Arc::Time endtime(joblog["endtime"]);
	ur.NewChild("EndTime")=endtime.str(Arc::UTCTime);
      }
    
    //MachineName
    if (joblog.find("nodename")!=joblog.end())
      {
	ur.NewChild("MachineName")=joblog["nodename"];
      }
    
    //Host?
    
    //SubmitHost
    if (joblog.find("clienthost")!=joblog.end())
      {
	// Chop port no.
	std::string hostport=joblog["clienthost"], host;
	int clnp=hostport.find(":");
	if (clnp==std::string::npos)
	  host=hostport;
	else
	  host=hostport.substr(0,clnp);

	ur.NewChild("SubmitHost")=host;
      }
    
    //Queue
    if (joblog.find("lrms")!=joblog.end())
      {
	ur.NewChild("Queue")=joblog["lrms"];  //OK for Queue?
      }
    
    //ProjectName? Extract from JSDL?
    
    //TODO differentiated properties
    
    return ur;
  }
  
} // namespace
