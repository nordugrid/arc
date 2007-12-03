#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SSTREAM
#include <sstream>
#else
#include <strstream>
#endif
#include <fstream>

#include "../../files/info_types.h"
#include "../../files/info_files.h"
#include "../../misc/canonical_dir.h"

#include "../jobdesc_util.h"
#include "../../jobs/users.h"
#include "../../files/info_files.h"
#include "../../files/info_types.h"
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#define inttostring Arc::tostring

static Arc::Logger& logger = Arc::Logger::getRootLogger();


#include "jsdl_job.h"

static void add_non_cache(const char *fname,std::list<FileData> &inputdata) {
  for(std::list<FileData>::iterator i=inputdata.begin();i!=inputdata.end();++i){    if(i->has_lfn()) if((*i) == fname) {
      Arc::URL u(i->lfn);
      if(u) {
        u.AddOption("cache","no");
        u.AddOption("exec","yes");
        i->lfn=u.fullstr();
      };
    };
  };
}

static void strip_spaces(std::string& s) {
  int n;
  for(n=0;s[n];n++) if(!isspace(s[n])) break;
  if(n) s.erase(0,n);
  for(n=s.length()-1;n>=0;n--) if(!isspace(s[n])) break;
  s.resize(n+1);
}

static void set_namespaces(Arc::NS ns) {
  ns["jsdl"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl";
  ns["jsdl-posix"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
  ns["jsdl-arc"] = "http://www.nordugrid.org/ws/schemas/jsdl-arc";  
  ns["jsdl-hpcpa"] = "http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
}

//-- Constructors --//

JSDLJob::JSDLJob(void) {
  set_namespaces(jsdl_namespaces);
}

JSDLJob::JSDLJob(std::istream& f) {

  set_namespaces(jsdl_namespaces);

  std::string xml_document;
  std::string xml_line;

  //Read the input stream into a string by lines
  while (getline(f,xml_line)) xml_document += xml_line;

  (Arc::XMLNode ( xml_document )).New(jsdl_document);

}

JSDLJob::JSDLJob(const char* str) {

  set_namespaces(jsdl_namespaces);

  (Arc::XMLNode ( str )).New(jsdl_document);
}

//-- End of Constructors--//

//-- Descructor --//

JSDLJob::~JSDLJob(void) {
        // Destructor isn't needed as I hope!
	// Delete global variables if they were initialised before
	//if (jsdl_document) delete jsdl_document;	
}

//-- End of Descructor --//

void JSDLJob::set(std::istream& f) {

  std::string xml_document;
  std::string xml_line;

  //Read the input stream into a string by lines
  while (getline(f,xml_line)) xml_document += xml_line;

  jsdl_document = Arc::XMLNode (xml_document);

}

void JSDLJob::set_posix(void) {

  std::list<Arc::XMLNode> results = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl:Application//jsdl-posix:POSIXApplication", jsdl_namespaces);

  if ( results.size() > 0 ) jsdl_posix = results.front();

  results = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl:Application//jsdl-hpcpa:HPCProfileApplication", jsdl_namespaces);

  if ( results.size() > 0 ) jsdl_posix = results.front();

}


//-- Protected get functions --//

bool JSDLJob::get_jobname(std::string& jobname) {
  jobname.resize(0);

  std::list<Arc::XMLNode> results = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl:JobIdentification//jsdl:JobName", jsdl_namespaces);

  if ( results.size() > 0 ) {
    //The First result of the XPath lookup (if exists) is converted explicitly to string, which operation calls the XMLNode's "getContent" functions
    jobname = (std::string) results.front();
    return true;
  } else return false;
}

bool JSDLJob::get_arguments(std::list<std::string>& arguments) {
  arguments.clear();

  if(jsdl_posix.XPathLookup("//jsdl-hpcpa:Executable", jsdl_namespaces).size() != 0) {
    std::string str_executable = (std::string) jsdl_posix.XPathLookup("//jsdl-hpcpa:Executable", jsdl_namespaces).front();
    strip_spaces(str_executable);
    arguments.push_back(str_executable);


    logger.msg(Arc::INFO, "job description executable[jsdl-hpcpa:Executable]");

    std::list<Arc::XMLNode> results = jsdl_posix.XPathLookup("//jsdl-hpcpa:Argument", jsdl_namespaces);
    for(std::list<Arc::XMLNode>::iterator i = results.begin();i!=results.end();++i) {
      if(!(*i)) continue;
      std::string value = (*i);
      strip_spaces(value);
      arguments.push_back(value);
    };
    return true;

  } else {


  if(jsdl_posix.XPathLookup("//jsdl-posix:Executable", jsdl_namespaces).size() == 0) {
    logger.msg(Arc::ERROR, "job description is missing executable[jsdl-posix:Executable]");
    return false;
  };

    logger.msg(Arc::INFO, "job description executable[jsdl-posix:Executable]");

  std::string str_executable = jsdl_posix.XPathLookup("//jsdl-posix:Executable", jsdl_namespaces).front();
  strip_spaces(str_executable);
  arguments.push_back(str_executable);

  std::list<Arc::XMLNode> results = jsdl_posix.XPathLookup("//jsdl-posix:Argument", jsdl_namespaces);

  for(std::list<Arc::XMLNode>::iterator i = results.begin();i!=results.end();++i) {
    if(!(*i)) continue;
    std::string value = (*i);
    strip_spaces(value);
    arguments.push_back(value);
  };
  return true;

  }
}

bool JSDLJob::get_execs(std::list<std::string>& execs) {

  std::list<Arc::XMLNode> results = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl:DataStaging", jsdl_namespaces);
  std::list<Arc::XMLNode>::iterator i;
  execs.clear();
  for(i=results.begin();i!=results.end();++i) {
    if(!(*i)) continue;
    std::list<Arc::XMLNode> results = (*i).XPathLookup("//jsdl:Source", jsdl_namespaces);
    if(results.size() != 0) { /* input file */
      std::list<Arc::XMLNode> isExecutableNodes = (*i).XPathLookup("//jsdl:Source//jsdl-arc:IsExecutable", jsdl_namespaces);
      if( isExecutableNodes.size() != 0 && ( (std::string) isExecutableNodes.front() ) == "true" ) {
	std::string str_filename = (std::string) (*i).XPathLookup("//jsdl:FileName", jsdl_namespaces).front();
        execs.push_back(str_filename);
      };
    };
  };
  return true;
}

bool JSDLJob::get_RTEs(std::list<std::string>& rtes) {
  rtes.clear();
  std::list<Arc::XMLNode> resources = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl:Resource", jsdl_namespaces);
  if(resources.size() == 0) return true;
  std::list<Arc::XMLNode> rte = resources.front().XPathLookup("//jsdl-arc:RunTimeEnvironment", jsdl_namespaces);
  for(std::list<Arc::XMLNode>::iterator i_rte = rte.begin();i_rte!=rte.end();++i_rte) {
    std::string s = (std::string) (*i_rte).XPathLookup("//jsdl-arc:Name",jsdl_namespaces).front();

    Arc::XMLNode versionNode = (*i_rte).XPathLookup("//jsdl-arc:Version",jsdl_namespaces).front();
    std::list<Arc::XMLNode> upperExclusiveNodes = versionNode.XPathLookup("//jsdl-arc:UpperExclusive",jsdl_namespaces);
    std::list<Arc::XMLNode> lowerExclusiveNodes = versionNode.XPathLookup("//jsdl-arc:LowerExclusive",jsdl_namespaces);
    std::list<Arc::XMLNode> exactNodes = versionNode.XPathLookup("//jsdl-arc:Exact",jsdl_namespaces);
    std::list<Arc::XMLNode> exclusiveNodes = versionNode.XPathLookup("//jsdl-arc:Exclusive",jsdl_namespaces);

    if( versionNode ) {
      if( upperExclusiveNodes.size() != 0 ) continue; // not supported
      if( lowerExclusiveNodes.size() != 0 ) continue; // not supported
      if( ( exclusiveNodes.size() != 0 ) && !( ( (std::string) exclusiveNodes.front() ) == "true"  ) ) continue; // not supported
      if( exactNodes.size() > 1) continue; // not supported
      if( exactNodes.size() != 0 ) {
        s+="="; s+= (std::string) versionNode;
      };
    };
    rtes.push_back(s);
  };
  return true;
}

bool JSDLJob::get_middlewares(std::list<std::string>& mws) {
  mws.clear();
  std::list<Arc::XMLNode> resources = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl:Resource", jsdl_namespaces);
  if(resources.size() == 0) return true;
  std::list<Arc::XMLNode> mw = resources.front().XPathLookup("//jsdl-arc:Middleware", jsdl_namespaces);
  for(std::list<Arc::XMLNode>::iterator i_mw = mw.begin();i_mw!=mw.end();++i_mw) {
    std::string s = (std::string) (*i_mw).XPathLookup("//jsdl-arc:Name",jsdl_namespaces).front();

    Arc::XMLNode versionNode = (*i_mw).XPathLookup("//jsdl-arc:Version",jsdl_namespaces).front();
    std::list<Arc::XMLNode> upperExclusiveNodes = versionNode.XPathLookup("//jsdl-arc:UpperExclusive",jsdl_namespaces);
    std::list<Arc::XMLNode> lowerExclusiveNodes = versionNode.XPathLookup("//jsdl-arc:LowerExclusive",jsdl_namespaces);
    std::list<Arc::XMLNode> exactNodes = versionNode.XPathLookup("//jsdl-arc:Exact",jsdl_namespaces);
    std::list<Arc::XMLNode> exclusiveNodes = versionNode.XPathLookup("//jsdl-arc:Exclusive",jsdl_namespaces);

    if( versionNode ) {
      if( upperExclusiveNodes.size() != 0 ) continue; // not supported
      if( lowerExclusiveNodes.size() != 0 ) continue; // not supported
      if( ( exclusiveNodes.size() != 0 ) && !( ( (std::string) exclusiveNodes.front() ) == "true"  ) ) continue; // not supported
      if( exactNodes.size() > 1) continue; // not supported
      if( exactNodes.size() != 0 ) {
        s+="="; s+=(std::string) (*i_mw).XPathLookup("//jsdl-arc:Version",jsdl_namespaces).front();
      };
    };
    mws.push_back(s);
  };
  return true;
}

bool JSDLJob::get_acl(std::string& acl) {
  acl.resize(0);
  std::list<Arc::XMLNode> aclNodes = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl-arc:AccessControl", jsdl_namespaces);
  if( aclNodes.size() == 0) return true;
  Arc::XMLNode a = aclNodes.front();
  std::list<Arc::XMLNode> typeNodes = a.XPathLookup("//jsdl-arc:Type", jsdl_namespaces);
  std::string str_content = a.XPathLookup("//jsdl-arc:Content", jsdl_namespaces).front();
  if( typeNodes.size() == 0 ||
     ( (std::string) typeNodes.front() ) == "GACL" ) {
    if( str_content != "" ) acl=str_content;
  } else {
    return false;
  };
  return true;
}

bool JSDLJob::get_notification(std::string& s) {
  s.resize(0);
  std::list<Arc::XMLNode> nts = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl-arc:Notify", jsdl_namespaces);
  for(std::list<Arc::XMLNode>::iterator i_nts = nts.begin();i_nts!=nts.end();++i_nts) {
    std::list<Arc::XMLNode> typeNodes = (*i_nts).XPathLookup("//jsdl-arc:Type", jsdl_namespaces);
    std::list<Arc::XMLNode> endpointNodes = (*i_nts).XPathLookup("//jsdl-arc:Endpoint", jsdl_namespaces);
    std::list<Arc::XMLNode> stateNodes = (*i_nts).XPathLookup("//jsdl-arc:State", jsdl_namespaces);
    if( (typeNodes.size() == 0 || ( (std::string) typeNodes.front() ) == "Email") && endpointNodes.size() != 0 && stateNodes.size() != 0) {
          std::string s_;
          for(std::list<Arc::XMLNode>::iterator i = stateNodes.begin(); i!=stateNodes.end(); ++i) {
            std::string value = (*i);
            if ( value == "PREPARING" ) {
              s_+="b";
            } else if ( value == "INLRMS" ) {
              s_+="q";
            } else if ( value == "FINISHING" ) {
              s_+="f";
            } else if ( value == "FINISHED" ) {
              s_+="e";
            } else if ( value == "DELETED" ) {
              s_+="d";
            } else if ( value == "CANCELING" ) {
              s_+="c";
            };
          };
          if(s_.length()) {
            s+=s_; s+= (std::string) endpointNodes.front(); s+=" ";
          };
    };
  };
  return true;
}

bool JSDLJob::get_lifetime(int& l) {
  std::list<Arc::XMLNode> resources = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl:Resource", jsdl_namespaces);
  if(resources.size() == 0) return true;
  std::list<Arc::XMLNode> lifeTimeNodes = resources.front().XPathLookup("//jsdl-arc:SessionLifeTime", jsdl_namespaces);
  if( lifeTimeNodes.size() != 0 ) {
    l= atoi( ((std::string) lifeTimeNodes.front()).c_str() );
  };
  return true;
}

bool JSDLJob::get_fullaccess(bool& b) {
  std::list<Arc::XMLNode> resources = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl:Resource", jsdl_namespaces);
  if(resources.size() == 0) return true;
  std::list<Arc::XMLNode> sessionTypeNodes = resources.front().XPathLookup("//jsdl-arc:SessionLifeTime", jsdl_namespaces);
  if( sessionTypeNodes.size() != 0 ) {
    if ( ( (std::string) sessionTypeNodes.front() ) == "FULL" ) {
      b=true;
    } else b=false;
  };
  return true;
}

bool JSDLJob::get_gmlog(std::string& s) {
  s.resize(0);
  std::list<Arc::XMLNode> logNodes = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl-arc:LocalLogging", jsdl_namespaces);
  if( logNodes.size() == 0 ) return true;
  s = (std::string) logNodes.front().XPathLookup("//jsdl-arc:Directory", jsdl_namespaces).front();
  return true;
}

bool JSDLJob::get_loggers(std::list<std::string>& urls) {
  urls.clear();
  std::list<Arc::XMLNode> logNodes = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl-arc:RemoteLogging", jsdl_namespaces);
  for(std::list<Arc::XMLNode>::iterator i_logs=logNodes.begin(); i_logs!=logNodes.end(); ++i_logs) {
    urls.push_back( (std::string) (*i_logs).XPathLookup("//URL", jsdl_namespaces).front() );
  };
  return true;
}

bool JSDLJob::get_credentialserver(std::string& url) {
  url.resize(0);
  std::list<Arc::XMLNode> credNodes = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl-arc:CredentialServer", jsdl_namespaces);
  if( credNodes.size() != 0 ) {
    url= (std::string) credNodes.front().XPathLookup("//URL", jsdl_namespaces).front();
  };
  return true;
}

bool JSDLJob::get_queue(std::string& s) {
  s.resize(0);
  std::list<Arc::XMLNode> resources = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl:Resource", jsdl_namespaces);
  if(resources.size() == 0) return true;
  std::list<Arc::XMLNode> candidateTargets = resources.front().XPathLookup("//jsdl-arc:CandidateTarget", jsdl_namespaces);
  if(candidateTargets.size() == 0) return true;
  std::list<Arc::XMLNode> queuenames = resources.front().XPathLookup("//jsdl-arc:CandidateTarget//jsdl-arc:QueueName", jsdl_namespaces);
  if(queuenames.size() == 0) return true;
  s = (std::string) queuenames.front();
  return true;
}

bool JSDLJob::get_stdin(std::string& s) {
  std::list<Arc::XMLNode> inputNodes = jsdl_posix.XPathLookup("//jsdl-hpcpa:Input", jsdl_namespaces);
  if( inputNodes.size() != 0 )  {
    std::string value = (std::string) inputNodes.front();
    strip_spaces(value);
    s = value;
    return true;
  }

  inputNodes = jsdl_posix.XPathLookup("//jsdl-posix:Input", jsdl_namespaces);
  if( inputNodes.size() != 0 )  {
    std::string value = (std::string) inputNodes.front();
    strip_spaces(value);
    s = value;
  } else { s.resize(0); };
  return true;
}

bool JSDLJob::get_stdout(std::string& s) {
  std::list<Arc::XMLNode> outputNodes = jsdl_posix.XPathLookup("//jsdl-hpcpa:Output", jsdl_namespaces);
  if( outputNodes.size() != 0 )  {
    std::string value = (std::string) outputNodes.front();
    strip_spaces(value);
    s = value;
    return true;
  } 

  outputNodes = jsdl_posix.XPathLookup("//jsdl-posix:Output", jsdl_namespaces);
  if( outputNodes.size() != 0 )  {
    std::string value = (std::string) outputNodes.front();
    strip_spaces(value);
    s = value;
  } else {
    s.resize(0);
  };
  return true;
}

bool JSDLJob::get_stderr(std::string& s) {
  std::list<Arc::XMLNode> errorNodes = jsdl_posix.XPathLookup("//jsdl-hpcpa:Error", jsdl_namespaces);
  if( errorNodes.size() != 0 )  {
    std::string value = (std::string) errorNodes.front();
    strip_spaces(value);
    s = value;
    return true;
  }
  errorNodes = jsdl_posix.XPathLookup("//jsdl-posix:Error", jsdl_namespaces);
  if( errorNodes.size() != 0 )  {
    std::string value = (std::string) errorNodes.front();
    strip_spaces(value);
    s = value;
  } else {
    s.resize(0);
  };
  return true;
}

bool JSDLJob::get_data(std::list<FileData>& inputdata,int& downloads,
                       std::list<FileData>& outputdata,int& uploads) {
  std::list<Arc::XMLNode> ds = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl:DataStaging", jsdl_namespaces);
  std::list<Arc::XMLNode>::iterator i_ds;
  inputdata.clear(); downloads=0;
  outputdata.clear(); uploads=0;
  for(i_ds=ds.begin();i_ds!=ds.end();++i_ds) {
    if(!(*i_ds)) continue;
    std::list<Arc::XMLNode> fileSystemNameNodes = (*i_ds).XPathLookup("//jsdl:FilesystemName", jsdl_namespaces);
    if(fileSystemNameNodes.size() != 0) {
      logger.msg(Arc::ERROR, "FilesystemName defined in job description - all files must be relative to session directory[jsdl:FilesystemName]");
      return false;
    };
    std::list<Arc::XMLNode> sourceNodes = (*i_ds).XPathLookup("//jsdl:Source", jsdl_namespaces);
    std::list<Arc::XMLNode> targetNodes = (*i_ds).XPathLookup("//jsdl:Target", jsdl_namespaces);
    std::list<Arc::XMLNode> filenameNodes = (*i_ds).XPathLookup("//jsdl:FileName", jsdl_namespaces);
    // CreationFlag - ignore
    // DeleteOnTermination - ignore
    if(( sourceNodes.size() == 0 ) && ( targetNodes.size() == 0 )) {
      // Neither in nor out - must be file generated by job
      FileData fdata( ( (std::string) filenameNodes.front() ).c_str(),"" );
      outputdata.push_back(fdata);
    } else {
      if( sourceNodes.size() != 0 ) {
        std::list<Arc::XMLNode> sourceURINodes = sourceNodes.front().XPathLookup("//jsdl:URI", jsdl_namespaces);
        if( sourceURINodes.size() == 0 ) {
          // Source without URL - uploaded by client
          FileData fdata( ( (std::string) filenameNodes.front() ).c_str(),"");
          inputdata.push_back(fdata);
        } else {
          FileData fdata( ((std::string) filenameNodes.front() ).c_str(),
                          ((std::string) sourceURINodes.front() ).c_str());
          inputdata.push_back(fdata);
          if(fdata.has_lfn()) downloads++;
        };
      };
      if( targetNodes.size() != 0 ) {
        std::list<Arc::XMLNode> targetURINodes = targetNodes.front().XPathLookup("//jsdl:URI", jsdl_namespaces);
        if( targetURINodes.size() == 0 ) {
          // Destination without URL - keep after execution
          // TODO: check for DeleteOnTermination
          FileData fdata( ( (std::string) filenameNodes.front() ).c_str(),"");
          outputdata.push_back(fdata);
        } else {
          FileData fdata( ((std::string) filenameNodes.front() ).c_str(),
                          ((std::string) targetURINodes.front() ).c_str());
          outputdata.push_back(fdata);
          if(fdata.has_lfn()) uploads++;
        };
      };
    };
  };
  return true;
}

/*
//Had to change because of the parameter type - the function isn't mentioned in the header file so it was possible
static double get_limit(jsdl__RangeValue_USCOREType* range) {
  if(range == NULL) return 0.0;
  if(range->UpperBoundedRange) return range->UpperBoundedRange->__item;
  if(range->LowerBoundedRange) return range->LowerBoundedRange->__item;
  return 0.0;
}*/

double JSDLJob::get_limit(Arc::XMLNode range) {
  if(!range) return 0.0;
  std::list<Arc::XMLNode> upper = range.XPathLookup("//jsdl:UpperBound", jsdl_namespaces);
  std::list<Arc::XMLNode> lower = range.XPathLookup("//jsdl:LowerBound", jsdl_namespaces);
  if( upper.size() != 0 ) return atof( ( (std::string) upper.front() ).c_str() );
  if( lower.size() != 0 ) return atof( ( (std::string) lower.front() ).c_str() );
  return 0.0;
}

bool JSDLJob::get_memory(int& memory) {
  memory=0;
  std::list<Arc::XMLNode> memoryLimitNodes = jsdl_posix.XPathLookup("//jsdl-posix:MemoryLimit", jsdl_namespaces);
  if( memoryLimitNodes.size() != 0 ) {
    memory=atoi( ((std::string) memoryLimitNodes.front()).c_str() );
    return true;
  };
  std::list<Arc::XMLNode> resources = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl:Resources", jsdl_namespaces);
  if( resources.size() == 0 ) return true;
  std::list<Arc::XMLNode> totalPhysicalMemoryNodes = resources.front().XPathLookup("//jsdl:TotalPhysicalMemory", jsdl_namespaces);
  if( totalPhysicalMemoryNodes.size() != 0 ) {
    memory=(int)(get_limit( totalPhysicalMemoryNodes.front() )+0.5);
    return true;
  };
  std::list<Arc::XMLNode> individualPhysicalMemoryNodes = resources.front().XPathLookup("//jsdl:IndividualPhysicalMemory", jsdl_namespaces);
  if( individualPhysicalMemoryNodes.size() != 0 ) {
    memory=(int)(get_limit( individualPhysicalMemoryNodes.front() )+0.5);
    return true;
  };
  return true;
}

bool JSDLJob::get_cputime(int& t) {
  t=0;
  std::list<Arc::XMLNode> CPUTimeLimitNodes = jsdl_posix.XPathLookup("//jsdl-posix:CPUTimeLimit", jsdl_namespaces);
  if( CPUTimeLimitNodes.size() != 0 ) {
    t=atoi( ((std::string) CPUTimeLimitNodes.front()).c_str() );
    return true;
  };
  std::list<Arc::XMLNode> resources = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl:Resources", jsdl_namespaces);
  if( resources.size() == 0 ) return true;
  std::list<Arc::XMLNode> totalCPUTimeNodes = resources.front().XPathLookup("//jsdl:TotalCPUTime", jsdl_namespaces);
  if( totalCPUTimeNodes.size() != 0 ) {
    t=(int)(get_limit( totalCPUTimeNodes.front() )+0.5);
    return true;
  };
  std::list<Arc::XMLNode> individualCPUTimeNodes = resources.front().XPathLookup("//jsdl:IndividualCPUTime", jsdl_namespaces);
  if( individualCPUTimeNodes.size() != 0 ) {
    t=(int)(get_limit( individualCPUTimeNodes.front() )+0.5);
    return true;
  };
  return true;
}

bool JSDLJob::get_walltime(int& t) {
  t=0;
  std::list<Arc::XMLNode> wallTimeLimitNodes = jsdl_posix.XPathLookup("//jsdl-posix:WallTimeLimit", jsdl_namespaces);
  if( wallTimeLimitNodes.size() != 0 ) {
    t=atoi( ((std::string) wallTimeLimitNodes.front()).c_str() );
    return true;
  };
  return get_cputime(t);
}

bool JSDLJob::get_count(int& n) {
  std::list<Arc::XMLNode> resources = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl:Resources", jsdl_namespaces);
  n=1;
  if( resources.size() == 0 ) return true;
  std::list<Arc::XMLNode> totalCPUCountNodes = resources.front().XPathLookup("//jsdl:TotalCPUCount", jsdl_namespaces);
  std::list<Arc::XMLNode> individualCPUCountNodes = resources.front().XPathLookup("//jsdl:IndividualCPUCount", jsdl_namespaces);

  if( totalCPUCountNodes.size() != 0 ) {
    n=(int)(get_limit( totalCPUCountNodes.front() )+0.5);
    return true;
  };
  if( individualCPUCountNodes.size() != 0 ) {
    n=(int)(get_limit( individualCPUCountNodes.front() )+0.5);
    return true;
  };
  return true;
}

bool JSDLJob::get_reruns(int& n) {
  std::list<Arc::XMLNode> rerunNodes = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl-arc:Reruns", jsdl_namespaces);
  if(rerunNodes.size() != 0) {
    n= atoi( ((std::string) rerunNodes.front()).c_str() );
  };
  return true;
}

bool JSDLJob::check(void) {
  if(!jsdl_document) {
    logger.msg(Arc::ERROR, "job description is missing[jsdl_document]");
    return false;
  };

  if( !jsdl_document.XPathLookup("//jsdl:JobDescription", jsdl_namespaces).front() ) {
    logger.msg(Arc::ERROR, "job description is missing[JobDescription]");
    return false;
  };
  if(!jsdl_posix) {
    logger.msg(Arc::ERROR, "job description is missing POSIX application[jsdl_posix]");
    return false;
  };
  return true;
}

bool JSDLJob::parse(JobLocalDescription &job_desc,std::string* acl) {
  std::list<std::string> l;
  std::string s;
  int n;
  char c;
  set_posix();
  if(!check()) return false;
  if(!get_jobname(job_desc.jobname)) return false;
  if(!get_data(job_desc.inputdata,job_desc.downloads,job_desc.outputdata,job_desc.uploads)) return false;
  if(!get_arguments(job_desc.arguments)) return false;
  if(!get_stdin(job_desc.stdin_)) return false;
  if(!get_stdout(job_desc.stdout_)) return false;
  if(!get_stderr(job_desc.stderr_)) return false;
  n=-1; if(!get_lifetime(n)) return false; if(n != -1) job_desc.lifetime=inttostring(n);
  if(!get_fullaccess(job_desc.fullaccess)) return false;
  if(acl) if(!get_acl(*acl)) return false;
  if(!get_arguments(l)) return false;
  if(l.size() == 0) return false;
  c=l.begin()->c_str()[0];
  if((c != '/') && (c != '$')) {
    add_non_cache(l.begin()->c_str(),job_desc.inputdata);
  };
  if(!get_execs(l)) return false;
  for(std::list<std::string>::iterator i = l.begin();i!=l.end();++i) {
    add_non_cache(i->c_str(),job_desc.inputdata);
  };
  if(!get_gmlog(job_desc.stdlog)) return false;
  if(!get_loggers(l)) return false; if(l.size())job_desc.jobreport=*(l.begin());
  if(!get_notification(job_desc.notify)) return false;
  if(!get_queue(job_desc.queue)) return false;
  if(!get_credentialserver(job_desc.credentialserver)) return false;

  return true;
}

bool JSDLJob::set_execs(const std::string &session_dir) {
  if(!check()) return false;
  std::list<std::string> arguments;
  if(!get_arguments(arguments)) return false;
  if(arguments.size() == 0) return false;
  /* executable can be external, so first check for initial slash */
  char c = arguments.begin()->c_str()[0];
  if((c != '/') && (c != '$')){
    if(canonical_dir(*(arguments.begin())) != 0) {
      logger.msg(Arc::ERROR, "Bad name for executable: %s", (*(arguments.begin())).c_str());
      return false;
    };
    fix_file_permissions(session_dir+"/"+(*(arguments.begin())),true);
  };
  std::list<std::string> execs;
  if(!get_execs(execs)) return false;
  for(std::list<std::string>::iterator i = execs.begin();i!=execs.end();++i) {
    if(canonical_dir(*i) != 0) {
      logger.msg(Arc::ERROR, "Bad name for executable: %s", (*i).c_str()); 
      return false;
    };
    fix_file_permissions(session_dir+"/"+(*i));
  };
  return true;
}

#ifdef JobDescription
#undef JobDescription
bool JSDLJob::write_grami(const JobDescription &desc,const JobUser &user,const char* opt_add) {
#define JobDescription jsdl__JobDescription
#else
bool JSDLJob::write_grami(const JobDescription &desc,const JobUser &user,const char* opt_add) {
#endif
  set_posix();
  if(!check()) return false;
  if(desc.get_local() == NULL) return false;
  std::string session_dir = desc.SessionDir();
  JobLocalDescription& job_desc = *(desc.get_local());
  std::string fgrami = user.ControlDir() + "/job." + desc.get_id() + ".grami";
  std::ofstream f(fgrami.c_str(),std::ios::out | std::ios::trunc);
  if(!f.is_open()) return false;
  if(!fix_file_owner(fgrami,desc,user)) return false;
  f<<"joboption_directory='"<<session_dir<<"'"<<std::endl;
  int n;
  std::list<std::string> arguments;
  if(!get_arguments(arguments)) return false;
  n=0;
  for(std::list<std::string>::iterator i = arguments.begin();
                                         i!=arguments.end();++i) {
    f<<"joboption_arg_"<<n<<"="<<value_for_shell(i->c_str(),true)<<std::endl;
    n++;
  };
  std::string s;
  if(!get_stdin(s)) return false;
  if(s.length() == 0) s=NG_RSL_DEFAULT_STDIN;
  f<<"joboption_stdin="<<value_for_shell(s.c_str(),true)<<std::endl;
  if(!get_stdout(s)) return false;
  if(s.length() == 0) { s=NG_RSL_DEFAULT_STDOUT; }
  else {
    if(canonical_dir(s) != 0) {
      logger.msg(Arc::WARNING, "Bad name for stdout: %s", s.c_str());
      return false;
    };
    s=session_dir+s;
  };
  f<<"joboption_stdout="<<value_for_shell(s.c_str(),true)<<std::endl;
  if(!get_stderr(s)) return false;
  if(s.length() == 0) { s=NG_RSL_DEFAULT_STDERR; }
  else {
    if(canonical_dir(s) != 0) {
      logger.msg(Arc::WARNING, "Bad name for stderr: %s", s.c_str());
      return false;
    };
    s=session_dir+s;
  };
  f<<"joboption_stderr="<<value_for_shell(s.c_str(),true)<<std::endl;
  if(!get_walltime(n)) return false;
  if(n<=0) { f<<"joboption_walltime="<<std::endl; }
  else { f<<"joboption_walltime="<<numvalue_for_shell(n)<<std::endl; };
  if(!get_cputime(n)) return false;
  if(n<=0) { f<<"joboption_cputime="<<std::endl; }
  else { f<<"joboption_cputime="<<numvalue_for_shell(n)<<std::endl; };
  if(!get_memory(n)) return false;
  if(n<=0) { f<<"joboption_memory="<<std::endl; }
  else { f<<"joboption_memory="<<numvalue_for_shell(n)<<std::endl; };
  if(!get_count(n)) return false;
  if(n<=0) { f<<"joboption_count=1"<<std::endl; }
  else { f<<"joboption_count="<<numvalue_for_shell(n)<<std::endl; };

  // Pre-parsed
  f<<"joboption_jobname="<<value_for_shell(job_desc.jobname,true)<<std::endl;
  f<<"joboption_gridid="<<value_for_shell(desc.get_id(),true)<<std::endl;
  f<<"joboption_queue="<<value_for_shell(job_desc.queue,true)<<std::endl;
  if(job_desc.exectime.defined()) {
    f<<"joboption_starttime="<<job_desc.exectime<<std::endl;
  } else {
    f<<"joboption_starttime="<<std::endl;
  };
/*
  GLOBUS_RSL_PARAM_GET(rsl_tree,GLOBUS_RSL_PARAM_SEQUENCE,
                           NG_RSL_ENVIRONMENT_PARAM,tmp_param);
  {
    int i;
    for(i=0;tmp_param[i]!=NULL;i++) { 
      f<<"joboption_env_"<<(i/2)<<"='"<<value_for_shell(tmp_param[i],false)<<"=";
      i++; if(tmp_param[i]==NULL) { f<<"'"<<std::endl; break; };
      f<<value_for_shell(tmp_param[i],false)<<"'"<<std::endl;
    };
  };


*/


  std::list<std::string> rtes;
  if(!get_RTEs(rtes)) return false;
  n=0;
  for(std::list<std::string>::iterator i = rtes.begin();i!=rtes.end();++i) {
    std::string tmp_s = *i;
    for(int ii=0;ii<tmp_s.length();++ii) tmp_s[ii]=toupper(tmp_s[ii]);
    if(canonical_dir(tmp_s) != 0) {
      logger.msg(Arc::WARNING, "Bad name for runtime environemnt: %s", (*i).c_str());
      return false;
    };
    f<<"joboption_runtime_"<<n<<"="<<value_for_shell(i->c_str(),true)<<std::endl;
    n++;
  };
  //f<<"joboption_rsl="<<frsl<<std::endl;
  print_to_grami(f);
  if(opt_add) f<<opt_add<<std::endl;
  return true;
}

void JSDLJob::print_to_grami(std::ostream &o) {
}

