#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "JobLogFile.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <list>
#include <vector>
#include <map>
#include <algorithm>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/Utils.h>
#include <arc/credential/Credential.h>
#include <arc/credential/VOMSUtil.h>

// Needed to redefine mkdir on mingw
#ifdef WIN32
#include <arc/win32.h>
#endif

namespace ArcJura
{

  std::string replaceChar(std::string str, char ch1, char ch2) {
    for (int i = 0; i < (int)str.length(); ++i) {
      if (str[i] == ch1)
        str[i] = ch2;
    }
    return str;
  }

  std::vector<Arc::VOMSACInfo> ParseVOAttr(std::string cert_str) {
	cert_str=replaceChar(cert_str,'\\', '\n');

    Arc::Credential holder(cert_str, "", "", "", "", false);
    std::string ca_dir = Arc::GetEnv("X509_CERT_DIR");
    std::string voms_dir = Arc::GetEnv("X509_VOMS_DIR");

    Arc::VOMSTrustList voms_trust_dn;
    voms_trust_dn.AddRegex(".*");
    std::vector<Arc::VOMSACInfo> voms_attributes;
    parseVOMSAC(holder, ca_dir, "", voms_dir, voms_trust_dn, voms_attributes, true, true);
    //parseVOMSAC(cert_str, ca_dir, "", voms_dir, voms_trust_dn, voms_attributes, true, true);

    return voms_attributes;
  }

  static std::string vo_filters_previous="";
  static std::multimap<std::string,std::string> log_vo_map;

  bool VOFilterCheck(std::string vo_filters, std::string voname, std::string loggerurl) {
      bool retval=true;
      std::multimap<std::string,std::string> local_vo_map;
      
      if (vo_filters == "") {
        // Accept all messages
        return true;
      }
      if (vo_filters == vo_filters_previous) {
          // tokenized it before
          local_vo_map=log_vo_map;
      } else {
         //tokenize vo_filters string to map
          char * pairs;
          char delimiter = ',';
          pairs = strtok (strdup(vo_filters.c_str()), &delimiter);
          while (pairs != NULL)
          {
            std::string pairStr = (std::string)pairs;
            std::size_t  found = pairStr.find_first_of(" ");
            // left trim
            std::size_t firstChar = pairStr.find_first_not_of(' ');
            if ( firstChar > found ) {
              pairStr.erase (0,firstChar);
            }
            // right trim
            pairStr.erase(pairStr.find_last_not_of(" \n\r\t")+1);
            
            found = pairStr.find_first_of(" ");
            std::string vo = pairStr.substr(0,found);
            std::string url = pairStr.substr(found+1);
            Arc::Logger::rootLogger.msg(Arc::DEBUG,
                   "Insert filter element: <%s,%s>",url, vo);
            local_vo_map.insert ( std::pair<std::string,std::string>(url,vo) );
            
            pairs = strtok (NULL, &delimiter);
          }
         vo_filters_previous = vo_filters;
         log_vo_map=local_vo_map;
      }
      
      if ( local_vo_map.count(loggerurl) == 0 ) {
          // No filter set for this logger URL
          // Accept the record
          Arc::Logger::rootLogger.msg(Arc::VERBOSE,
                   "Not set filter for this URL (%s).", loggerurl);
      } else {
          // Contains VO filter option for this logger URL
          bool acceptVO=false;
          std::map<std::string,std::string>::iterator it;
          Arc::Logger::rootLogger.msg(Arc::DEBUG,
                   "Current job's VO name: %s", voname);
          for (it=local_vo_map.equal_range(loggerurl).first; it!=local_vo_map.equal_range(loggerurl).second; ++it) {
            Arc::Logger::rootLogger.msg(Arc::DEBUG,
                   "VO filter for host: %s", it->second);
            if ( it->second == voname ) {
              acceptVO=true;
              break;
            }
          }
          retval = acceptVO;
      }
      return retval;
  }

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
            if ( key == "inputfile") {
                inputfiles.push_back(value);
            }
            if ( key == "outputfile") {
                outputfiles.push_back(value);
            }
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

  int JobLogFile::write(const std::string& filename_)
  {
    int count=0;  //number of saved values

    // save inherited filename for archiving!
    if ( !filename.empty() && !(*this)["jobreport_option_archiving"].empty() )
      {
        std::string base_fn;
        size_type seppos=filename.rfind('/');
        if (seppos==std::string::npos)
          base_fn=filename;
        else
          base_fn=filename.substr(seppos+1,std::string::npos);
        (*this)["archiving_basefilename"] = base_fn;
      }
    //filename=_filename;

    std::ofstream logfile(filename_.c_str());

    std::map<std::string,std::string>::iterator it;
    for(it=begin(); it!=end(); it++)
      {
        if ( it->first != "inputfile" && it->first != "outputfile" && it->second != "" )
          {
            logfile << it->first << "=" << it->second << std::endl;
            count++;
          }
      }
    for (int i=0; i<(int)inputfiles.size(); i++) {
      logfile << "inputfile=" << inputfiles[i] << std::endl;
      count++;
    }
    for (int i=0; i<(int)outputfiles.size(); i++) {
      logfile << "outputfile=" << outputfiles[i] << std::endl;
      count++;
    }

    logfile.close();
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
            Arc::Logger::rootLogger.msg(Arc::VERBOSE,
                "Read archive file %s",
                archive_fn.c_str());
            return;
          }
        Arc::Logger::rootLogger.msg(Arc::VERBOSE,
           "Could not read archive file %s for job log file %s (%s), generating new Usage Record",
           archive_fn.c_str(),
           filename.c_str(),
           Arc::StrError(errno));
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
    ns_ur["vo"]="http://www.sgas.se/namespaces/2009/05/ur/vo";
    ns_ur["tr"]="http://www.sgas.se/namespaces/2010/10/filetransfer";

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
        rid.NewAttribute("urf:createTime")=nowstamp;
        //NOTE! Current LUTS also sets a "creationTime"[sic!] for each record
        
        // ID for record
        if (find("headnode")!=end()) {
          Arc::URL headnode((*this)["headnode"]);
          rid.NewAttribute("urf:recordId")=
            std::string(recordid_prefix) + headnode.Host() + 
            '-' + (*this)["ngjobid"]; 
        }
        else  if (!mainnode.empty())
          rid.NewAttribute("urf:recordId")=
            std::string(recordid_prefix) + mainnode + 
            '-' + (*this)["ngjobid"];
        else
          rid.NewAttribute("urf:recordId")=
            std::string(recordid_prefix) + (*this)["ngjobid"];
        
        ur.NewChild("JobIdentity").NewChild("GlobalJobId")=
          (*this)["globalid"];
        
        if (find("localid")!=end()) {
          std::string prefix = "";
          if (find("jobreport_option_localid_prefix")!=end())
            prefix = (*this)["jobreport_option_localid_prefix"] +"-";
          ur["JobIdentity"].NewChild("LocalJobId")=prefix + (*this)["localid"];
        }
      }
    else
    {
      //TODO what if not valid?
      Arc::Logger::rootLogger.msg(Arc::VERBOSE,
                      "Missing required Usage Record element \"RecordIdentity\", in job log file %s",
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
        ur.NewChild("UserIdentity");
        if (find("localuser")!=end())
          ur["UserIdentity"].NewChild("LocalUserId")=
            (*this)["localuser"];

        ur["UserIdentity"].NewChild("GlobalUserName")=
          (*this)["usersn"];
      }
    
    //VO Attributes  
    if (find("usercert")!=end())
      {
        std::vector<Arc::VOMSACInfo> voms_attributes = ParseVOAttr((*this)["usercert"]);
        std::string loggerurl=(*this)["loggerurl"];
        bool needToSend = false;
        for(int n = 0; n<(int)voms_attributes.size(); ++n) {

            if(voms_attributes[n].attributes.size() > 0) {
                if ( !ur["UserIdentity"] ) {
                    ur.NewChild("UserIdentity");
                }
                Arc::XMLNode vo=ur["UserIdentity"].NewChild("vo:VO");
                vo.NewAttribute("vo:type")="voms";
                
                std::string voname=voms_attributes[n].voname;
                vo.NewChild("vo:Name")=voname;
                vo.NewChild("vo:Issuer")=voms_attributes[n].issuer;
                

                if ( find("vo_filters")!=end() ) {
                    if ( !needToSend ) {
                        needToSend = VOFilterCheck((*this)["vo_filters"], voname, loggerurl);
                        if ( !needToSend ) {
                            Arc::Logger::rootLogger.msg(Arc::DEBUG,
                                "VO (%s) not set for this (%s) SGAS server by VO filter.",
                                 voname, loggerurl);
                        }
                    }
                } else {
                    needToSend = true;
                }

                for(int i = 0; i < (int)voms_attributes[n].attributes.size(); i++) {
                  std::string attr = voms_attributes[n].attributes[i];
                  std::string::size_type pos;
                  if((pos = attr.find("hostname=")) != std::string::npos) {
                    continue;
                  }
                  Arc::XMLNode vo_attr=vo.NewChild("vo:Attribute");

                  if(attr.find("Role=") == std::string::npos ||
                     attr.find("Capability=") == std::string::npos) {  
                    vo_attr.NewChild("vo:Group")=attr;
                  }
                  if((pos = attr.find("Role=")) != std::string::npos) {
                    std::string str = attr.substr(pos+5);
                    vo_attr.NewChild("vo:Role")=str;
                  }
                  if((pos = attr.find("Capability=")) != std::string::npos) {
                    std::string str = attr.substr(pos+11);
                    vo_attr.NewChild("vo:Capability")=str;
                  }
                }
            }
        }
        if ( !needToSend && (int)voms_attributes.size() > 0) {
            Arc::Logger::rootLogger.msg(Arc::INFO,
                 "[VO filter] Job log will be not send. %s.", filename.c_str());
            usagerecord.Destroy();
            return;
        }
      }

    if (find("jobreport_option_vomsless_vo")!=end() && !ur["UserIdentity"]["vo:VO"])
      {
        if ( !ur["UserIdentity"] ) {
          ur.NewChild("UserIdentity");
        }
        Arc::XMLNode vo=ur["UserIdentity"].NewChild("vo:VO");
        vo.NewAttribute("vo:type")="voms";

        std::string option=(*this)["jobreport_option_vomsless_vo"];
        std::string name=option;
        std::string issuer=option;
        // separate opt_vo_name#opt_vo_issuer pair
        pcolon=option.find('#');
        if (pcolon!=std::string::npos)
          {
            name=option.substr(0, pcolon), 
            issuer=option.substr(pcolon+1, std::string::npos);
          }
        vo.NewChild("vo:Name")=name;
        vo.NewChild("vo:Issuer")=issuer;
        vo.NewChild("vo:Attribute").NewChild("vo:Group")="/"+name;
      }

    if (find("jobreport_option_vo_group")!=end())
      {
        bool find = false;
        if ( !ur["UserIdentity"] ) {
          ur.NewChild("UserIdentity");
        }
        Arc::XMLNode useridentity = ur["UserIdentity"];

        Arc::XMLNode node = useridentity["vo:VO"]["vo:Attribute"]["vo:Group"];
        while ( bool(node) ){
          if (std::string(node) == (*this)["jobreport_option_vo_group"]) {
            find = true;
            break;
          }
          ++node;
        }
        if ( !find ) {
          if ( !useridentity["vo:VO"] ) {
            Arc::XMLNode vo = useridentity.NewChild("vo:VO");
            vo.NewAttribute("vo:type")="voms";
          }
          if ( !useridentity["vo:VO"]["vo:Attribute"] ) {
            useridentity["vo:VO"].NewChild("vo:Attribute");
          }
          useridentity["vo:VO"]["vo:Attribute"].NewChild("vo:Group")=(*this)["jobreport_option_vo_group"].substr(1);
        }
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
        Arc::Logger::rootLogger.msg(Arc::VERBOSE,
                        "Missing required element \"Status\" in job log file %s",
                        filename.c_str());
        usagerecord.Destroy();
        return;
      }
    
    //---
    //Network?

    //Disk?

    //Memory
    if (find("usedmemory")!=end() && (*this)["usedmemory"]!="0")
      {
        Arc::XMLNode memn=ur.NewChild("Memory")=(*this)["usedmemory"];
        memn.NewAttribute("urf:storageUnit")="KB";
        memn.NewAttribute("urf:metric")="average";
        memn.NewAttribute("urf:type")="virtual";
      }

    if (find("usedmaxresident")!=end() && (*this)["usedmaxresident"]!="0")
      {
        Arc::XMLNode memn=ur.NewChild("Memory")=(*this)["usedmaxresident"];
        memn.NewAttribute("urf:storageUnit")="KB";
        memn.NewAttribute("urf:metric")="max";
        memn.NewAttribute("urf:type")="physical";
      }

    if (find("usedaverageresident")!=end() && (*this)["usedaverageresident"]!="0")
      {
        Arc::XMLNode memn=ur.NewChild("Memory")=(*this)["usedaverageresident"];
        memn.NewAttribute("urf:storageUnit")="KB";
        memn.NewAttribute("urf:metric")="average";
        memn.NewAttribute("urf:type")="physical";
      }
    
    //Swap?

    //TimeDuration, TimeInstant, ServiceLevel?

    //---
    //WallDuration
    if (find("usedwalltime")!=end())
      {
        Arc::Period walldur((*this)["usedwalltime"],Arc::PeriodSeconds);
        std::string walld = (std::string)walldur;
        if (walld == "P"){
          walld = "PT0S";
        }
        ur.NewChild("WallDuration")=walld;
      }
    
    //CpuDuration

    if (find("usedusercputime")!=end() && find("usedkernelcputime")!=end())
      {
        Arc::Period udur((*this)["usedusercputime"],Arc::PeriodSeconds);
        Arc::Period kdur((*this)["usedkernelcputime"],Arc::PeriodSeconds);

        std::string udurs = (std::string)udur;
        if (udurs == "P"){
            udurs = "PT0S";
        }
        Arc::XMLNode udurn=ur.NewChild("CpuDuration")=udurs;
        udurn.NewAttribute("urf:usageType")="user";

        std::string kdurs = (std::string)kdur;
        if (kdurs == "P"){
            kdurs = "PT0S";
        }
        Arc::XMLNode kdurn=ur.NewChild("CpuDuration")=kdurs;
        kdurn.NewAttribute("urf:usageType")="system";
      }
    else
    if (find("usedcputime")!=end())
      {
        Arc::Period cpudur((*this)["usedcputime"],Arc::PeriodSeconds);
        std::string cpudurs = (std::string)cpudur;
        if (cpudurs == "P"){
            cpudurs = "PT0S";
        }
        ur.NewChild("CpuDuration")=cpudurs;
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
    if (find("headnode")!=end())
      {
        std::string machinename=(*this)["headnode"];
        size_t first = machinename.find(":");
        size_t last = machinename.find(":", first+1);
        if (last == std::string::npos) {
            last=machinename.length();
        }
        ur.NewChild("MachineName")=machinename.substr(first+3,last-first-3);
      }
    
    //Host
    if (!mainnode.empty())
      {
        Arc::XMLNode primary_node=ur.NewChild("Host");
        primary_node=mainnode;
        primary_node.NewAttribute("urf:primary")="true";
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
    
    //Headnode
    /* Unhandled UR element by SGAS
     if (find("headnode")!=end())
      {
        ur.NewChild("Headnode")=(*this)["headnode"];
      }*/

    //Queue
    if (find("queue")!=end())
      {
        ur.NewChild("Queue")=(*this)["queue"];
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

    //Processors
    if (find("processors")!=end())
      {
        ur.NewChild("Processors")=(*this)["processors"];
      }

    //Transfer statistics
    if (find("inputfile")!=end())
      {
        parseInputOutputFiles(ur, inputfiles);
      }
    if (find("outputfile")!=end())
      {
        parseInputOutputFiles(ur, outputfiles, "output");
      }

    //Extra:
    //RuntimeEnvironment

    for(std::list<std::string>::iterator jt=rtes.begin();
        jt!=rtes.end();
        ++jt)
      {
        ur.NewChild("arc:RuntimeEnvironment")=*jt;
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
            Arc::Logger::rootLogger.msg(Arc::VERBOSE,
                                        "Creating directory %s",
                                        dir_name.c_str());
            errno=0;
            if (mkdir(dir_name.c_str(),S_IRWXU)!=0)
              {
                Arc::Logger::rootLogger.msg(Arc::ERROR,
                    "Failed to create archive directory %s: %s",
                    dir_name.c_str(),
                    Arc::StrError(errno));
              }
          }
        
        Arc::Logger::rootLogger.msg(Arc::VERBOSE,
                                    "Archiving Usage Record to file %s",
                                    archive_fn.c_str());
        errno=0;
        if (!ur.SaveToFile(archive_fn.c_str()))
          {
            Arc::Logger::rootLogger.msg(Arc::ERROR,
                                        "Failed to write file %s: %s",
                                        archive_fn.c_str(),
                                        Arc::StrError(errno));
          }
      }
    //***

    usagerecord.Replace(ur);
  }

  void JobLogFile::createCARUsageRecord(Arc::XMLNode &usagerecord,
                                     const char *recordid_prefix)
  {
    //***
    //If archiving is enabled: first try to load archived UR
    std::string archive_fn=getArchivingPath(true);
    if (!archive_fn.empty())
      {
        errno=0;
        if (usagerecord.ReadFromFile(archive_fn))
          {
            Arc::Logger::rootLogger.msg(Arc::VERBOSE,
                "Read archive file %s",
                archive_fn.c_str());
            return;
          }
        Arc::Logger::rootLogger.msg(Arc::VERBOSE,
           "Could not read archive file %s for job log file %s (%s), generating new Usage Record",
           archive_fn.c_str(),
           filename.c_str(),
           Arc::StrError(errno));
      }
    //Otherwise go on and create new UR
    //***

    Arc::NS ns_ur;
    
    //Namespaces defined by OGF 2.0 (CAR 1.2)
    ns_ur[""]="http://eu-emi.eu/namespaces/2012/11/computerecord";
    ns_ur["urf"]="http://eu-emi.eu/namespaces/2012/11/computerecord";
    ns_ur["xsd"]="http://www.w3.org/2001/XMLSchema";
    ns_ur["xsi"]="http://www.w3.org/2001/XMLSchema-instance";

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

    /**BASE Record properties
     * 
     * UsageRecord
     * RecordIdentity
     * JobIdentity
     * UserIdentity
     * JobName
     * Charge
     * Status
     * ExitStatus
     * WallDuration
     * CpuDuration
     * EndTime
     * StartTime
     * MachineName
     * Host
     * SubmitHost
     * Site
     * Infrastructure
     */

    /** Differentiated Record Properties
     * 
     * Queue
     * Middleware
     * Memory
     * Swap
     * NodeCount
     * Processors
     * TimeInstant
     * ServiceLevel
     */
     
    //Fill this Usage Record
    //UsageRecord
    Arc::XMLNode ur(ns_ur,"UsageRecord");

    //RecordIdentity, JobIdentity GlobalJobId, LocalJobId
    if (find("ngjobid")!=end())
      {
        // Timestamp for record, required
        std::string nowstamp=Arc::Time().str(Arc::UTCTime);

        Arc::XMLNode rid=ur.NewChild("RecordIdentity");
        rid.NewAttribute("urf:createTime")=nowstamp;

        // ID for record
        if (find("headnode")!=end()){
          Arc::URL headnode((*this)["headnode"]);
          rid.NewAttribute("urf:recordId")=
            std::string(recordid_prefix) + headnode.Host() + 
            '-' + (*this)["ngjobid"]; 
        }      
        else if (!mainnode.empty())
          rid.NewAttribute("urf:recordId")=
            std::string(recordid_prefix) + mainnode + 
            '-' + (*this)["ngjobid"];
        else
          rid.NewAttribute("urf:recordId")=
            std::string(recordid_prefix) + (*this)["ngjobid"];

        //GlobalJobId
        if (find("globalid")!=end())
          ur.NewChild("JobIdentity").NewChild("GlobalJobId")=
            (*this)["globalid"];

        //LocalJobId
        if (!ur["JobIdentity"])
          ur.NewChild("JobIdentity");

        if (find("localid")!=end()) {
          ur["JobIdentity"].NewChild("LocalJobId")=
            (*this)["localid"];
        } else {
          ur["JobIdentity"].NewChild("LocalJobId")=
            (*this)["ngjobid"];
        }
      }
    else
    {
      Arc::Logger::rootLogger.msg(Arc::VERBOSE,
                      "Missing required Usage Record element \"RecordIdentity\", in job log file %s",
                      filename.c_str());
      usagerecord.Destroy();
      return;
    }

    //ProcessId
    if (find("???")!=end()) //TODO: not present in the AREX accounting log
      {
         if (!ur["JobIdentity"])
           ur.NewChild("JobIdentity");

         Arc::XMLNode pid = 
           ur["JobIdentity"].NewChild("ProcessId")=
            (*this)["???"];
         pid.NewAttribute("urf:Host")=(*this)["nodename"];
       }

    //UserIdentity
    Arc::XMLNode useridentity = ur.NewChild("UserIdentity");
    //GlobalUserName
    if (find("usersn")!=end())
      {
        Arc::XMLNode globalusername =
          useridentity.NewChild("GlobalUserName")=
            (*this)["usersn"];
        globalusername.NewAttribute("urf:type")="opensslCompat";
      }

    //Group
    //GroupAttribute  
    if (find("usercert")!=end())
      {
        std::vector<Arc::VOMSACInfo> voms_attributes = ParseVOAttr((*this)["usercert"]);
        for(int n = 0; n<(int)voms_attributes.size(); ++n) {

            if(voms_attributes[n].attributes.size() > 0) {
                if ( !useridentity["Group"] ) {
                    useridentity.NewChild("Group")=voms_attributes[n].voname;
                }
                for(int i = 0; i < (int)voms_attributes[n].attributes.size(); i++) {
                  std::string attr = voms_attributes[n].attributes[i];
                  std::string::size_type pos;
                  if((pos = attr.find("hostname=")) != std::string::npos) {
                    continue;
                  }
                  else {
                    pos = attr.find("Role=");

                    std::string group = attr.substr(0,pos-1);
                    Arc::XMLNode vog=useridentity.NewChild("GroupAttribute")=group;
                    vog.NewAttribute("urf:type")="vo-group";

                    if( pos != std::string::npos) {
                      std::string role = attr.substr(pos);
                      Arc::XMLNode vo=useridentity.NewChild("GroupAttribute")=role;
                      vo.NewAttribute("urf:type")="vo-role";
                    }
                  }
                }
            }
        }
      }

    if (find("jobreport_option_vomsless_vo")!=end() && !useridentity["Group"])
      {
        if ( !useridentity["Group"] ) {
          useridentity.NewChild("Group");
        }
        Arc::XMLNode vo=useridentity["Group"];

        std::string option=(*this)["jobreport_option_vomsless_vo"];
        std::string name=option;
        std::string issuer=option;
        // separate opt_vo_name#opt_vo_issuer pair
        pcolon=option.find('#');
        if (pcolon!=std::string::npos)
          {
            name=option.substr(0, pcolon), 
            issuer=option.substr(pcolon+1, std::string::npos);
          }
        vo=name;
        Arc::XMLNode gattr=useridentity.NewChild("GroupAttribute")="/"+name;
        gattr.NewAttribute("urf:type")="vo-role";
      }

    if (find("jobreport_option_vo_group")!=end())
      {
        bool find = false;
        Arc::XMLNode node = useridentity["GroupAttribute"];
        while ( bool(node) ){
          if (node.Attribute("urf:type") == "vo-group" &&
              std::string(node) == (*this)["jobreport_option_vo_group"]) {
            find = true;
            break;
          }
          ++node;
        }
        if ( !find ) {
          if ( !useridentity["Group"] ) {
            useridentity.NewChild("Group")=(*this)["jobreport_option_vo_group"].substr(1);
          }
          Arc::XMLNode vog=useridentity.NewChild("GroupAttribute")=(*this)["jobreport_option_vo_group"];
          vog.NewAttribute("urf:type")="vo-group";
        }
      }

    if (find("projectname")!=end())
      {
        Arc::XMLNode project=useridentity.NewChild("GroupAttribute")=(*this)["projectname"];
        project.NewAttribute("urf:type")="ProjectName";
      }

    //LocalUserId
    if (find("localuser")!=end())
      {
        useridentity.NewChild("LocalUserId")=
          (*this)["localuser"];
      }

    //LocalGroup

    //JobName
    if (find("jobname")!=end())
      {
        ur.NewChild("JobName")=(*this)["jobname"];
      }

    //Charge +unit,formula
 
    //Status
    if (find("status")!=end())
      {
        /*
         * The available status values are:
         *  aborted, completed, failed, held, queued, started, suspended
         */
        ur.NewChild("Status")=(*this)["status"];  //TODO convert?
      }
    else
      {
        //TODO what if not valid?
        Arc::Logger::rootLogger.msg(Arc::VERBOSE,
                        "Missing required element \"Status\" in job log file %s",
                        filename.c_str());
        usagerecord.Destroy();
        return;
      }

    // ExitStatus
    if (find("exitcode")!=end())
      {
        ur.NewChild("ExitStatus")=(*this)["exitcode"];
      }

    //Infrastructure
    Arc::XMLNode infran = ur.NewChild("Infrastructure");
    std::string type = "local";
    if (find("headnode")!=end() && (*this)["lrms"] != "fork" ){
        type = "grid";
        std::string lrms = (std::string)(*this)["lrms"];
        std::transform(lrms.begin(), lrms.end(), lrms.begin(), ::toupper);
        infran.NewAttribute("urf:description")="JURA-ARC-"+lrms;  //Prefix required to the APEL
    }
    infran.NewAttribute("urf:type")=type;

    //Middleware
    Arc::XMLNode middleware =
          ur.NewChild("Middleware")="";
    middleware.NewAttribute("urf:name")="arc";
    middleware.NewAttribute("urf:version")=VERSION;
    middleware.NewAttribute("urf:description")=PACKAGE_STRING;

    // WallDuration (EndTime-StartTime)
    ur.NewChild("WallDuration")="PT0S";
    if (find("usedwalltime")!=end())
      {
        Arc::Period walldur((*this)["usedwalltime"],Arc::PeriodSeconds);
        std::string walld = (std::string)walldur;
        if (walld == "P"){
          walld = "PT0S";
        }
        ur["WallDuration"]=walld;
      }

    // CpuDuration
    /* TODO: This element contains the CPU time consumed. If the job ran
     *  on many cores/processors/nodes/sites, all separate consumptions
     *  shall be aggregated in this value. This as an impact on MPI jobs,
     *  where the consumption of all the 'nodes' of the MPI job get
     *  aggregated into this CPU consumption. This is the way LRMS
     *  accounting work on the batch systems underlying the avaialble
     *  CE implementations.
     */
    if (find("usedusercputime")!=end() && find("usedkernelcputime")!=end())
      {
        Arc::Period udur((*this)["usedusercputime"],Arc::PeriodSeconds);
        Arc::Period kdur((*this)["usedkernelcputime"],Arc::PeriodSeconds);

        std::string udurs = (std::string)udur;
        if (udurs == "P"){
            udurs = "PT0S";
        }
        Arc::XMLNode udurn=ur.NewChild("CpuDuration")=udurs;
        udurn.NewAttribute("urf:usageType")="user";

        std::string kdurs = (std::string)kdur;
        if (kdurs == "P"){
            kdurs = "PT0S";
        }
        Arc::XMLNode kdurn=ur.NewChild("CpuDuration")=kdurs;
        kdurn.NewAttribute("urf:usageType")="system";

        Arc::Period all(udur.GetPeriod() + kdur.GetPeriod());
        std::string alls = (std::string)all;
        if (alls == "P"){
            alls = "PT0S";
        }
        Arc::XMLNode alln=ur.NewChild("CpuDuration")=alls;
        alln.NewAttribute("urf:usageType")="all";
      }
    else
    if (find("usedcputime")!=end())
      {
        Arc::Period cpudur((*this)["usedcputime"],Arc::PeriodSeconds);
        std::string cpudurs = (std::string)cpudur;
        if (cpudurs == "P"){
            cpudurs = "PT0S";
        }
        Arc::XMLNode alln = ur.NewChild("CpuDuration")=cpudurs;
        alln.NewAttribute("urf:usageType")="all";
      }
    else
      {
        Arc::Logger::rootLogger.msg(Arc::VERBOSE,
                        "Missing required element \"CpuDuration\" in job log file %s",
                        filename.c_str());
        usagerecord.Destroy();
        return;
      }

    //ServiceLevel
    std::string benchmark_type = "Si2K";
    std::string benchmark_value = "1.0";
    std::string benchmark_description = "";
    if (find("jobreport_option_benchmark_type")!=end())
      {
        std::string type = (*this)["jobreport_option_benchmark_type"];
        std::vector<std::string> accepted_types;
        accepted_types.push_back("Si2k");
        accepted_types.push_back("Sf2k");
        accepted_types.push_back("HEPSPEC");
        if (std::find(accepted_types.begin(), accepted_types.end(), type) == accepted_types.end())
          {
            Arc::Logger::rootLogger.msg(Arc::WARNING,
                                        "Set non standard bechmark type: %s",
                                        type);

          }
        benchmark_type = type;
      }
    if (find("jobreport_option_benchmark_value")!=end())
      {
        std::string value = (*this)["jobreport_option_benchmark_value"];
        float float_value = Arc::stringtof(value);
        if (float_value==0)
          {
            Arc::Logger::rootLogger.msg(Arc::WARNING,
                                        "Ignored incoming benchmark value: %s, Use float value!",
                                        value);
          }
        else
          {
            std::ostringstream ss;
            ss << float_value;
            benchmark_value = ss.str();
            if (benchmark_value.find(".")==std::string::npos)
              {
                benchmark_value +=".0";
              }
          }
      }
    if (find("jobreport_option_benchmark_description")!=end())
      {
        benchmark_description = (*this)["jobreport_option_benchmark_description"];
      }

    Arc::XMLNode sleveln = ur.NewChild("ServiceLevel")=benchmark_value;
    sleveln.NewAttribute("urf:type")=benchmark_type;
    sleveln.NewAttribute("urf:description")=benchmark_description;


    //Memory
    if (find("usedmemory")!=end() && (*this)["usedmemory"] != "0")
      {
        Arc::XMLNode memn=ur.NewChild("Memory")=(*this)["usedmemory"];
        memn.NewAttribute("urf:type")="Shared";
        memn.NewAttribute("urf:metric")="average";
        memn.NewAttribute("urf:storageUnit")="KB";
      }

    if (find("usedmaxresident")!=end() && (*this)["usedmaxresident"] != "0")
      {
        Arc::XMLNode memn=ur.NewChild("Memory")=(*this)["usedmaxresident"];
        memn.NewAttribute("urf:type")="Physical";
        memn.NewAttribute("urf:metric")="max";
        memn.NewAttribute("urf:storageUnit")="KB";
      }

    if (find("usedaverageresident")!=end() && (*this)["usedaverageresident"] != "0")
      {
        Arc::XMLNode memn=ur.NewChild("Memory")=(*this)["usedaverageresident"];
        memn.NewAttribute("urf:type")="Physical";
        memn.NewAttribute("urf:metric")="average";
        memn.NewAttribute("urf:storageUnit")="KB";
      }

    //Swap
    //TimeInstant

    //NodeCount
    if (find("nodecount")!=end())
      {
        ur.NewChild("NodeCount")=(*this)["nodecount"];
      }

    //Processors
    if (find("processors")!=end())
      {
        ur.NewChild("Processors")=(*this)["processors"];
      }

    //EndTime
    if (find("endtime")!=end())
      {
        Arc::Time endtime((*this)["endtime"]);
        ur.NewChild("EndTime")=endtime.str(Arc::UTCTime);
      }

     //StartTime
    if (find("submissiontime")!=end())
      {
        Arc::Time starttime((*this)["submissiontime"]);
        ur.NewChild("StartTime")=starttime.str(Arc::UTCTime);
      }

    //MachineName
    if (find("headnode")!=end())
      {
        Arc::URL machineName((*this)["headnode"]);
        ur.NewChild("MachineName")=machineName.Host();
      }

    //SubmitHost
    if (find("headnode")!=end())
      {
        Arc::XMLNode submitn = ur.NewChild("SubmitHost")=(*this)["headnode"];
      }

    //Queue
    if (find("queue")!=end())
      {
        Arc::XMLNode queue =
          ur.NewChild("Queue")=(*this)["queue"];
        queue.NewAttribute("urf:description")="execution";
      }

    //Site
    if (find("headnode")!=end())
      {
        Arc::URL machineName((*this)["headnode"]);
        std::string site = machineName.Host();
        // repcale "." to "-"
        int position = site.find( "." ); // find first space
        while ( position != (int)std::string::npos ){
            site.replace( position, 1, "-" );
            position = site.find( ".", position + 1 );
        }
        // to upper case
        std::locale loc;
        for (size_t i=0; i<site.length(); ++i){
            site[i] = toupper(site[i],loc);
        }
        Arc::XMLNode siten = ur.NewChild("Site")=site;
        siten.NewAttribute("urf:type")="arc";
      }
    if (find("jobreport_option_gocdb_name")!=end())
      {
        //GOCDB name is required by the APEL as a Site value.
        std::string site = (*this)["jobreport_option_gocdb_name"];
        if ( !ur["Site"] ) {
            Arc::XMLNode siten = ur.NewChild("Site")=site;
            siten.NewAttribute("urf:type")="arc";
        } else {
            ur["Site"] = site;
        }
      }
    //Host
    if (!mainnode.empty())
      {
        Arc::XMLNode primary_node=ur.NewChild("Host");
        primary_node=mainnode;
        primary_node.NewAttribute("urf:primary")="true";
        std::list<std::string>::iterator it=nodenames.begin();
        ++it;
        while (it!=nodenames.end())
          {
            ur.NewChild("Host")=*it;
            ++it;
          }
      }

    /*Aggregated USAGE RECORD
     * 
     * SummaryRecord
     * SummaryRecords
     * Site
     * Month
     * Year
     * UserIdentity
     * MachineName
     * SubmitHost
     * Host
     * Queue
     * Infrastructure
     * Middleware
     * EarliestEndTime
     * LatestEndTime
     * WallDuration
     * CpuDuration
     * ServiceLevel
     * NumberOfJobs
     * Memory
     * Swap
     * NodeCoutn
     * Processors
     */

    //***
    //Archiving if enabled:
    if (!archive_fn.empty())
      {
        struct stat st;
        std::string dir_name=(*this)["jobreport_option_archiving"];
        if (stat(dir_name.c_str(),&st)!=0)
          {
            Arc::Logger::rootLogger.msg(Arc::VERBOSE,
                                        "Creating directory %s",
                                        dir_name.c_str());
            errno=0;
            if (mkdir(dir_name.c_str(),S_IRWXU)!=0)
              {
                Arc::Logger::rootLogger.msg(Arc::ERROR,
                    "Failed to create archive directory %s: %s",
                    dir_name.c_str(),
                    Arc::StrError(errno));
              }
          }

        Arc::Logger::rootLogger.msg(Arc::VERBOSE,
                                    "Archiving Usage Record to file %s",
                                    archive_fn.c_str());
        errno=0;
        if (!ur.SaveToFile(archive_fn.c_str()))
          {
            Arc::Logger::rootLogger.msg(Arc::ERROR,
                                        "Failed to write file %s: %s",
                                        archive_fn.c_str(),
                                        Arc::StrError(errno));
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
    if (!allow_remove) return;
    errno=0;
    int e = ::remove(filename.c_str());
    if (e)
      Arc::Logger::rootLogger.msg(Arc::ERROR,"Failed to delete file %s:%s",
                      filename.c_str(),
                      Arc::StrError(errno));
  }

  std::string JobLogFile::getArchivingPath(bool car)
  {
    //no archiving dir set
    if ((*this)["jobreport_option_archiving"].empty()) return std::string();

    //if set, archive file name corresponds to original job log file
    std::string base_fn = (*this)["archiving_basefilename"];
    if (base_fn.empty())
      {
        size_type seppos=filename.rfind('/');
        if (seppos==std::string::npos)
          base_fn=filename;
        else
          base_fn=filename.substr(seppos+1,std::string::npos);
      }

    if (car) {
      return (*this)["jobreport_option_archiving"]+"/usagerecordCAR."+base_fn;
    }
    return (*this)["jobreport_option_archiving"]+"/usagerecord."+base_fn;
  }
  
  void JobLogFile::parseInputOutputFiles(Arc::XMLNode &node, std::vector<std::string> &filelist, std::string type) {
    if ( !node["tr:FileTransfers"] ) {
        node.NewChild("tr:FileTransfers");
    }
    for (int i=0; i<(int)filelist.size(); i++) {
        std::string nodeName = "FileDownload";
        if ( type == "output") {
            nodeName = "FileUpload";
        }
        Arc::XMLNode dl = node["tr:FileTransfers"].NewChild("tr:" + nodeName);
        std::string option=filelist[i];
        size_type pcolon=option.find(',');
        while (pcolon!=std::string::npos)
          {
            std::string pair=option.substr(0, pcolon);
            size_type peqv=pair.find('=');
            std::string key=pair.substr(0, peqv);
            std::string value=pair.substr(peqv+1, std::string::npos);
            if ( key == "url" ) key = "URL";
            if ( key == "size" ) key = "Size";
            if ( key == "starttime" ) key = "StartTime";
            if ( key == "endtime" ) key = "EndTime";
            if ( type == "input" ) {
                if ( key == "bypasscache" ) key = "BypassCache";
                if ( key == "fromcache" ) key = "RetrievedFromCache";
                if ( value == "yes" )  value = "true";
                if ( value == "no" )  value = "false";
            }

            dl.NewChild("tr:"+key)=value;

            //next:
            if ( pcolon != option.length() ) {
              option=option.substr(pcolon+1, std::string::npos);
            } else {
                option="";
            }
            pcolon=option.find(',');
            if ( option.length() > 0 && pcolon==std::string::npos) {
              pcolon=option.length();
            }
          }
    }
  }
  
} // namespace
