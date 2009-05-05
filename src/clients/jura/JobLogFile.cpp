#include "JobLogFile.h"

#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "arc/Logger.h"

namespace Arc
{

  int JobLogFile::parse(const std::string& _filename)
  {
    int count=0;  //number of parsed values
    clear();
    filename=_filename;
    if (!exists()) return -1;

    std::ifstream logfile(filename.c_str(),std::ios::in);
    std::string line;
    while (logfile.good())
      {
	std::getline(logfile,line);
	size_type e=line.find('=');
	if (e!=std::string::npos)
	  {
	    count++;
	    std::string key=line.substr(0, e), 
	      value=line.substr(e+1, std::string::npos);
	    (*this)[key]=value;
	  }
      }
    logfile.close();
    //TODO parse jobreport_options string!
    return count;
  }

  void JobLogFile::createUsageRecord(Arc::XMLNode &usagerecord,
				     const char *recordid_prefix)
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
    if (find("ngjobid")!=end())
      {
	// Timestamp for record, required
	std::string nowstamp=Arc::Time().str(Arc::UTCTime);
	
	Arc::XMLNode rid=ur.NewChild("RecordIdentity");
	rid.NewAttribute("createTime")=nowstamp;
	//NOTE! Current LUTS also sets a "creationTime"[sic!] for each record
	
	// ID for record
	if (find("nodename")!=end())
	  rid.NewAttribute("recordId")=
	    std::string(recordid_prefix) + (*this)["nodename"] + 
	    '-' + (*this)["ngjobid"];
	else
	  rid.NewAttribute("recordId")=
	    std::string(recordid_prefix) + (*this)["ngjobid"];
	
	ur.NewChild("JobIdentity").NewChild("GlobalJobId")=
	  (*this)["ngjobid"];
	
	if (find("localjobid")!=end())
	  ur["JobIdentity"].NewChild("LocalJobId")=
	    (*this)["localjobid"];
      }
    else
    {
      //TODO what if not valid?
      Arc::Logger::rootLogger.msg(Arc::DEBUG,
		      "Missing required UR element \"RecordIdentity\", in job log file %s",
		      filename.c_str());
      usagerecord.Destroy();
      return;
    }
    
    //ProcessId?
    
    //GlobalUser[Nn]ame, LocalUserId
    //TODO clarify case
    //NOTE! original JARM used "GlobalUserId"
    if (find("usersn")!=end())
      {
	ur.NewChild("UserIdentity").NewChild("GlobalUserName")=
	  (*this)["usersn"];
	
	if (find("localuser")!=end())
	  ur["UserIdentity"].NewChild("LocalUserId")=
	    (*this)["localuser"];
      }
    
    //JobName
    if (find("jobname")!=end())
      {
	ur.NewChild("JobName")=(*this)["jobname"];
      }
    
    //Charge?
    
    //Status
    if (find("status")!=end())
      {
	ur.NewChild("Status")=(*this)["status"];  //TODO convert?
      }
    else
      {
	//TODO what if not valid?
	Arc::Logger::rootLogger.msg(Arc::DEBUG,
			"Missing required element \"Status\" in job log file %s",
			filename.c_str());
	usagerecord.Destroy();
	return;
      }
    
    //WallDuration
    if (find("usedwalltime")!=end())
      {
	Arc::Period walldur((*this)["usedwalltime"],Arc::PeriodSeconds);
	ur.NewChild("WallDuration")=(std::string)walldur;
      }
    
    //CpuDuration
    if (find("usedcputime")!=end())
      {
	Arc::Period cpudur((*this)["usedcputime"],Arc::PeriodSeconds);
	ur.NewChild("CpuDuration")=(std::string)cpudur;
      }
    
    //StartTime
    if (find("submissiontime")!=end())
      {
	Arc::Time starttime((*this)["submissiontime"]);
	ur.NewChild("StartTime")=starttime.str(Arc::UTCTime);
      }
    
    //EndTime
    if (find("endtime")!=end())
      {
	Arc::Time endtime((*this)["endtime"]);
	ur.NewChild("EndTime")=endtime.str(Arc::UTCTime);
      }
    
    //MachineName
    if (find("nodename")!=end())
      {
	ur.NewChild("MachineName")=(*this)["nodename"];
      }
    
    //Host? TODO!
    
    //SubmitHost
    if (find("clienthost")!=end())
      {
	// Chop port no.
	std::string hostport=(*this)["clienthost"], host;
	size_type clnp=hostport.find(":");
	if (clnp==std::string::npos)
	  host=hostport;
	else
	  host=hostport.substr(0,clnp);

	ur.NewChild("SubmitHost")=host;
      }
    
    //Queue
    if (find("lrms")!=end())
      {
	ur.NewChild("Queue")=(*this)["lrms"];  //OK for Queue?
      }
    
    //ProjectName
    if (find("projectname")!=end())
      {
	ur.NewChild("ProjectName")=(*this)["projectname"];
      }

    
    //TODO differentiated properties

    usagerecord.Replace(ur);
  }

  bool JobLogFile::exists()
  {
    //TODO cross-platform?
    struct stat s;
    return (0==stat(filename.c_str(),&s));
  }
  
  void JobLogFile::remove()
  {
    errno=0;
    int e = ::remove(filename.c_str());
    if (e)
      Arc::Logger::rootLogger.msg(Arc::ERROR,"Failed to delete file %s:%s",
		      filename.c_str(),
		      strerror(errno));
  }
  
} // namespace
