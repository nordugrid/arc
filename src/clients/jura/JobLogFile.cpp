#include "JobLogFile.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <list>

#include "arc/Logger.h"

// Needed to redefine mkdir on mingw
#ifdef WIN32
#include <arc/win32.h>
#endif

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

    //Parse jobreport_options string!

    std::string jobreport_opts=(*this)["accounting_options"];
    std::string option;
    size_type pcomma=jobreport_opts.find(',');
    size_type pcolon;
    while (pcomma!=std::string::npos)
      {
	option=jobreport_opts.substr(0,pcomma);
	// separate opt_name:value pair
	pcolon=option.find(':');
	if (pcolon!=std::string::npos)
	  {
	    std::string key=option.substr(0, pcolon), 
	      value=option.substr(pcolon+1, std::string::npos);

	    (*this)[std::string("jobreport_option_")+key]=value;
	  }
	
	//next:
	jobreport_opts=jobreport_opts.substr(pcomma+1, std::string::npos);
	pcomma=jobreport_opts.find(',');
      }
    option=jobreport_opts;
    pcolon=option.find(':');
    if (pcolon!=std::string::npos)
      {
	std::string key=option.substr(0, pcolon), 
	  value=option.substr(pcolon+1, std::string::npos);
	
	(*this)[std::string("jobreport_option_")+key]=value;
      }



    return count;
  }

  void JobLogFile::createUsageRecord(Arc::XMLNode &usagerecord,
				     const char *recordid_prefix)
  {
    //***
    //If archiving is enabled: first try to load archived UR
    std::string archive_fn=getArchivingPath();
    if (!archive_fn.empty())
      {
	errno=0;
	if (usagerecord.ReadFromFile(archive_fn))
	  {
	    Arc::Logger::rootLogger.msg(Arc::DEBUG,
		"Read archive file %s",
		archive_fn.c_str());
	    return;
	  }
	Arc::Logger::rootLogger.msg(Arc::DEBUG,
	   "Could not read archive file %s for job log file %s (%s), generating new UR",
	   archive_fn.c_str(),
	   filename.c_str(),
	   strerror(errno));
      }
    //Otherwise go on and create new UR
    //***

    Arc::NS ns_ur;
    
    //Namespaces defined by OGF
    ns_ur[""]="http://schema.ogf.org/urf/2003/09/urf";
    ns_ur["urf"]="http://schema.ogf.org/urf/2003/09/urf";
    ns_ur["xsd"]="http://www.w3.org/2001/XMLSchema";
    ns_ur["xsi"]="http://www.w3.org/2001/XMLSchema-instance";
    ns_ur["ds"]="http://www.w3.org/2000/09/xmldsig#";
    ns_ur["arc"]="http://www.nordugrid.org/ws/schemas/ur-arc";

    //Get node names
    std::list<std::string> nodenames;
    std::string mainnode;
    std::string nodestr=(*this)["nodename"];
    size_type pcolon=nodestr.find(':');
    while (pcolon!=std::string::npos)
      {
	nodenames.push_back(nodestr.substr(0,pcolon));
	nodestr=nodestr.substr(pcolon+1,std::string::npos);
	pcolon=nodestr.find(':');
      }
    if (!nodestr.empty()) nodenames.push_back(nodestr);
    if (!nodenames.empty()) mainnode=*(nodenames.begin());

    //Get runtime environments
    std::list<std::string> rtes;
    std::string rtestr=(*this)["runtimeenvironment"];
    size_type pspace=rtestr.find(" ");
    while (pspace!=std::string::npos)
      {
	std::string rte=rtestr.substr(0,pspace);
	if (!rte.empty()) rtes.push_back(rte);
	rtestr=rtestr.substr(pspace+1,std::string::npos);
	pspace=rtestr.find(" ");
      }
    if (!rtestr.empty()) rtes.push_back(rtestr);
    
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
	if (!mainnode.empty())
	  rid.NewAttribute("recordId")=
	    std::string(recordid_prefix) + mainnode + 
	    '-' + (*this)["ngjobid"];
	else
	  rid.NewAttribute("recordId")=
	    std::string(recordid_prefix) + (*this)["ngjobid"];
	
	ur.NewChild("JobIdentity").NewChild("GlobalJobId")=
	  (*this)["globalid"];
	
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
    
    //---
    //Network?

    //Disk?

    //Memory
    if (find("usedmemory")!=end())
      {
	Arc::XMLNode memn=ur.NewChild("Memory")=(*this)["usedmemory"];
	memn.NewAttribute("storageUnit")="kB";
	memn.NewAttribute("metric")="average";
	memn.NewAttribute("type")="virtual";
      }

    if (find("usedmaxresident")!=end())
      {
	Arc::XMLNode memn=ur.NewChild("Memory")=(*this)["usedmaxresident"];
	memn.NewAttribute("storageUnit")="kB";
	memn.NewAttribute("metric")="max";
	memn.NewAttribute("type")="physical";
      }

    if (find("usedaverageresident")!=end())
      {
	Arc::XMLNode memn=ur.NewChild("Memory")=(*this)["usedaverageresident"];
	memn.NewAttribute("storageUnit")="kB";
	memn.NewAttribute("metric")="average";
	memn.NewAttribute("type")="physical";
      }
    
    //Swap?

    //TimeDuration, TimeInstant, ServiceLevel?

    //---
    //WallDuration
    if (find("usedwalltime")!=end())
      {
	Arc::Period walldur((*this)["usedwalltime"],Arc::PeriodSeconds);
	ur.NewChild("WallDuration")=(std::string)walldur;
      }
    
    //CpuDuration

    if (find("usedusercputime")!=end() && find("usedkernelcputime")!=end())
      {
	Arc::Period udur((*this)["usedusercputime"],Arc::PeriodSeconds);
	Arc::Period kdur((*this)["usedkernelcputime"],Arc::PeriodSeconds);

	Arc::XMLNode udurn=ur.NewChild("CpuDuration")=(std::string)udur;
	udurn.NewAttribute("usageType")="user";

	Arc::XMLNode kdurn=ur.NewChild("CpuDuration")=(std::string)kdur;
	kdurn.NewAttribute("usageType")="kernel";
      }
    else
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
	ur.NewChild("MachineName")=mainnode;
      }
    
    //Host
    if (!mainnode.empty())
      {
	Arc::XMLNode primary_node=ur.NewChild("Host");
	primary_node=mainnode;
	primary_node.NewAttribute("primary")="true";
	std::list<std::string>::iterator it=nodenames.begin();
	++it;
	while (it!=nodenames.end())
	  {
	    ur.NewChild("Host")=*it;
	    ++it;
	  }	
      }
    
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

    
    //NodeCount
    if (find("nodecount")!=end())
      {
	ur.NewChild("NodeCount")=(*this)["nodecount"];
      }

    //Processors?

    //Extra:
    //RunTimeEnvironment

    for(std::list<std::string>::iterator jt=rtes.begin();
	jt!=rtes.end();
	++jt)
      {
	ur.NewChild("arc:RunTimeEnvironment")=*jt;
      }
    
    //TODO user id info

    //***
    //Archiving if enabled:
    if (!archive_fn.empty())
      {
	struct stat st;
	std::string dir_name=(*this)["jobreport_option_archiving"];
	if (stat(dir_name.c_str(),&st)!=0)
	  {
	    Arc::Logger::rootLogger.msg(Arc::DEBUG,
				        "Creating directory %s",
				        dir_name.c_str());
	    errno=0;
	    if (mkdir(dir_name.c_str(),S_IRWXU)!=0)
	      {
		Arc::Logger::rootLogger.msg(Arc::ERROR,
		    "Failed to create archive directory %s: %s",
		    dir_name.c_str(),
		    strerror(errno));
	      }
	  }
	
	Arc::Logger::rootLogger.msg(Arc::DEBUG,
				    "Archiving UR to file %s",
				    archive_fn.c_str());
	errno=0;
	if (!ur.SaveToFile(archive_fn.c_str()))
	  {
	    Arc::Logger::rootLogger.msg(Arc::ERROR,
					"Failed to write file %s: %s",
					archive_fn.c_str(),
					strerror(errno));
	  }
      }
    //***

    usagerecord.Replace(ur);
  }

  bool JobLogFile::exists()
  {
    //TODO cross-platform?
    struct stat s;
    return (0==stat(filename.c_str(),&s));
  }

  bool JobLogFile::olderThan(time_t age)
  {
    struct stat s;
    return ( ( 0==stat(filename.c_str(),&s) ) &&
	     ( (time(NULL)-s.st_mtime) > age ) 
	     );
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

  std::string JobLogFile::getArchivingPath()
  {
    //no archiving dir set
    if ((*this)["jobreport_option_archiving"].empty()) return std::string();

    //if set, archive file name corresponds to original job log file
    std::string base_fn;
    size_type seppos=filename.rfind('/');
    if (seppos==std::string::npos)
      base_fn=filename;
    else
      base_fn=filename.substr(seppos+1,std::string::npos);

    return (*this)["jobreport_option_archiving"]+"/usagerecord."+base_fn;
  }
  
} // namespace
