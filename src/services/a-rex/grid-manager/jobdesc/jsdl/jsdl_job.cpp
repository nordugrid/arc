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

//-- Constructors --//

JSDLJob::JSDLJob(void) {
}

JSDLJob::JSDLJob(std::istream& f) {

  std::string xml_document;
  std::string xml_line;

  //Read the input stream into a string by lines
  while (getline(f,xml_line)) xml_document += xml_line;

  (Arc::XMLNode ( xml_document )).New(jsdl_document);

}

JSDLJob::JSDLJob(const char* str) {
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

  Arc::XMLNode n = jsdl_document["JobDescription"]["Application"]["POSIXApplication"];
  if ( (bool)n ) jsdl_posix = n;

  n = jsdl_document["JobDescription"]["Application"]["HPCProfileApplication"];
  if ( (bool)n ) jsdl_hpcpa = n;

std::string s;
jsdl_document.GetXML(s);
logger.msg(Arc::INFO, "JSDL: %s",s);
jsdl_posix.GetXML(s);
logger.msg(Arc::INFO, "JSDL: POSIX: %s",s);
jsdl_hpcpa.GetXML(s);
logger.msg(Arc::INFO, "JSDL: HPCPA: %s",s);

}


//-- Protected get functions --//

bool JSDLJob::get_jobname(std::string& jobname) {

  Arc::XMLNode n = jsdl_document["JobDescription"]["JobIdentification"]["JobName"];
  if ( (bool)n ) {
    jobname = (std::string) n;
    return true;
  } else return true;

}

bool JSDLJob::get_jobprojects(std::list<std::string>& jobprojects) {
  Arc::XMLNode n = jsdl_document["JobDescription"]["JobIdentification"];

  if( (bool)(n) ) {

    jobprojects.clear();

    for(int i=0; (bool)(n["JobProject"][i]) ; i++ ) {
      std::string value = (std::string) n["JobProject"][i];
      strip_spaces(value);
      jobprojects.push_back(value);
    }
  }
  return true;
}

bool JSDLJob::get_arguments(std::list<std::string>& arguments) {

  Arc::XMLNode n;
  if( (bool)(n = jsdl_hpcpa["Executable"]) ) {
    std::string str_executable = (std::string) n;
    strip_spaces(str_executable);
    if (str_executable[0] != '/' && (str_executable[0] != '.' && str_executable[1] != '/')) str_executable = "./" + str_executable;
    arguments.clear();
    arguments.push_back(str_executable);

    logger.msg(Arc::INFO, "job description executable is provided by element [jsdl-hpcpa:Executable] - %s",
                          str_executable.c_str());

    for( int i=0; (bool)(jsdl_hpcpa["Argument"][i]) ; i++ ) {
      std::string value = (std::string) jsdl_hpcpa["Argument"][i];
      strip_spaces(value);
      arguments.push_back(value);
    }

    return true;

  } else if( (bool)(n = jsdl_posix["Executable"]) ) {
    std::string str_executable = (std::string) n;
    strip_spaces(str_executable);
    if (str_executable[0] != '/' && (str_executable[0] != '.' && str_executable[1] != '/')) str_executable = "./" + str_executable;
    arguments.clear();
    arguments.push_back(str_executable);

    logger.msg(Arc::INFO, "job description executable is provided by element [jsdl-posix:Executable] - %s",
                          str_executable.c_str());

    for( int i=0; (bool)(jsdl_posix["Argument"][i]); i++ ) {
      std::string value = (std::string) jsdl_posix["Argument"][i];
      strip_spaces(value);
      arguments.push_back(value);
    }

    return true;

  }

  logger.msg(Arc::ERROR, "job description contains no recognisable executable");
  return false;

}

bool JSDLJob::get_execs(std::list<std::string>& execs) {

  Arc::XMLNode n = jsdl_document["JobDescription"];
  for( int i=0; (bool)(n["DataStaging"][i]); i++ ) {
    Arc::XMLNode childNode = n["DataStaging"][i]["Source"];
    if( (bool)childNode ) { /* input file */
      Arc::XMLNode isExecutableNode = childNode["IsExecutable"];
      if( ((bool)isExecutableNode) && (( (std::string) isExecutableNode ) == "true") ) {
	std::string str_filename = (std::string) n["DataStaging"][i]["FileName"];
        execs.push_back(str_filename);
      };
    };
  };
  return true;
}

bool JSDLJob::get_RTEs(std::list<std::string>& rtes) {
  Arc::XMLNode resources = jsdl_document["JobDescription"]["Resources"];
  if( !resources ) return true;
  for( int i=0; (bool)(resources["RunTimeEnvironment"][i]); i++ ) {

    std::string s = (std::string) resources["RunTimeEnvironment"][i]["Name"];

    Arc::XMLNode versionNode = resources["RunTimeEnvironment"][i]["Version"];

    if( (bool)versionNode ) {
      if( (bool)(versionNode["UpperExclusive"]) ) continue; // not supported
      if( (bool)(versionNode["LowerExclusive"]) ) continue; // not supported
      if( ( (bool)(versionNode["Exclusive"]) ) && (!( ( (std::string) versionNode["Exclusive"] ) == "true") ) ) continue; // not supported
      if( (bool)(versionNode["Exact"][1]) ) continue; // not supported (More than one)
      if( (bool)(versionNode["Exact"]) ) {
        s+="-"; s+= (std::string) versionNode["Exact"];
      };
    };
    if(!s.empty()) rtes.push_back(s);
  };
  return true;
}

bool JSDLJob::get_environments(std::list<std::pair<std::string,std::string> >& envs) {

  // TODO: we need to test this environment part

  Arc::XMLNode app = jsdl_document["JobDescription"]["Application"]["POSIXApplication"];
  if(app) {
    for( int i=0; (bool)(app["Environment"][i]); i++ ) {
       Arc::XMLNode env = app["Environment"][i];
       Arc::XMLNode name = env.Attribute("name");
       std::string value = (std::string)env;
       if( !name ) {
           logger.msg(Arc::ERROR, "JSDL: POSIX: POSIXApplication without name attribute");
           return false;
       }
       envs.push_back(std::make_pair(name,value));
       logger.msg(Arc::DEBUG, "ENV: "+(std::string)name+"="+value);
    }
  }
  else {
     app = jsdl_document["JobDescription"]["Application"]["HPCProfileApplication"];
     if( app ) {
       for( int i=0; (bool)(app["Environment"][i]); i++ ) {
         Arc::XMLNode env = app["Environment"][i];
         Arc::XMLNode name = env.Attribute("name");
         std::string value = (std::string)env;
         if( !name ) {
             logger.msg(Arc::ERROR, "JSDL: HPC: HPCProfileApplication without name attribute");
             return false;
         }
         envs.push_back(std::make_pair(name,value));
         logger.msg(Arc::DEBUG, "ENV: "+(std::string)name+"="+value);
      }
    }
  }
  return true;
}

bool JSDLJob::get_middlewares(std::list<std::string>& mws) {

  Arc::XMLNode resources = jsdl_document["JobDescription"]["Resources"];
  if( !resources ) return true;
  for( int i=0; (bool)(resources["Middleware"][i]); i++ ) {

    std::string s = (std::string) resources["Middleware"][i]["Name"];

    Arc::XMLNode versionNode = resources["Middleware"][i]["Version"];

    if( (bool)versionNode ) {
      if( (bool)(versionNode["UpperExclusive"]) ) continue; // not supported
      if( (bool)(versionNode["LowerExclusive"]) ) continue; // not supported
      if( ( (bool)(versionNode["Exclusive"]) ) && (!( ( (std::string) versionNode["Exclusive"] ) == "true") ) ) continue; // not supported
      if( (bool)(versionNode["Exact"][1]) ) continue; // not supported (More than one)
      if( (bool)(versionNode["Exact"]) ) {
        s+="="; s+= (std::string) versionNode;
      };
    };
    mws.push_back(s);
  };
  return true;
}

bool JSDLJob::get_acl(std::string& acl) {

  Arc::XMLNode aclNode = jsdl_document["JobDescription"]["AccessControl"];
  if( !aclNode ) return true;
  Arc::XMLNode typeNode = aclNode["Type"];
  Arc::XMLNode contentNode = aclNode["Content"];
  if( !contentNode ) return false;
  if( (!typeNode) || ( ( (std::string) typeNode ) == "GACL" ) || ( ( (std::string) typeNode ) == "ARC" ) ) {
    std::string str_content;
    if(contentNode.Size() > 0) {
      Arc::XMLNode acl_doc;
      contentNode.Child().New(acl_doc);
      acl_doc.GetDoc(str_content);
    } else {
      str_content = (std::string)contentNode;
    }
    if( str_content != "" ) acl=str_content;
  } else {
    logger.msg(Arc::ERROR, "JSDL: unsupported ACL type specified: %s", (std::string) typeNode);
    return false;
  };
  return true;
}

bool JSDLJob::get_notification(std::string& s) {

  for( int i=0; (bool)(jsdl_document["JobDescription"]["Notify"][i]); i++ ) {
    Arc::XMLNode notifyNode = jsdl_document["JobDescription"]["Notify"][i];
    Arc::XMLNode typeNode = notifyNode["Type"];
    Arc::XMLNode endpointNode  = notifyNode["Endpoint"];
    Arc::XMLNode stateNode = notifyNode["State"];
    if( ( (!typeNode) || ( ( (std::string) typeNode ) == "Email") ) && 
        ( (bool)endpointNode ) && ( (bool)stateNode ) ) {
          std::string s_;
          for( int j=0; (bool)(stateNode[j]); j++ ) {
            std::string value = (std::string) stateNode[i];
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
            s+=s_; s+= (std::string) endpointNode; s+=" ";
          };
    };
  };
  return true;
}

bool JSDLJob::get_lifetime(int& l) {
  Arc::XMLNode resources = jsdl_document["JobDescription"]["Resource"];
  if( !resources ) return true;
  Arc::XMLNode lifeTimeNodes = resources["SessionLifeTime"];
  if( (bool)lifeTimeNodes ) {
    char * pEnd;
    l= strtol( ((std::string) lifeTimeNodes).c_str(), &pEnd, 10 );
  };
  return true;
}

bool JSDLJob::get_fullaccess(bool& b) {
  Arc::XMLNode resources = jsdl_document["JobDescription"]["Resource"];
  if( !resources ) return true;
  Arc::XMLNode sessionTypeNodes = resources["SessionLifeTime"];
  if( (bool)sessionTypeNodes ) {
    if ( ( (std::string) sessionTypeNodes ) == "FULL" ) {
      b=true;
    } else b=false;
  };
  return true;
}

bool JSDLJob::get_gmlog(std::string& s) {

  Arc::XMLNode logNodes = jsdl_document["JobDescription"]["LocalLogging"];
  if( !logNodes ) return true;
  s = (std::string) logNodes["Directory"];
  return true;
}

bool JSDLJob::get_loggers(std::list<std::string>& urls) {

  Arc::XMLNode logNodes = jsdl_document["JobDescription"]["RemoteLogging"];
  for( int i=0; (bool)(jsdl_document["JobDescription"]["RemoteLogging"][i]); i++ ) {
    urls.push_back( (std::string) jsdl_document["JobDescription"]["RemoteLogging"][i]["URL"]);
  };
  return true;
}

bool JSDLJob::get_credentialserver(std::string& url) {

  Arc::XMLNode credNodes = jsdl_document["jsdl:JobDescription"]["CredentialServer"];
  if( (bool)credNodes ) {
    url= (std::string) credNodes["URL"];
  };
  return true;
}

bool JSDLJob::get_queue(std::string& s) {

  Arc::XMLNode queuename = jsdl_document["JobDescription"]["Resource"]["CandidateTarget"]["QueueName"];
  if( !queuename ) return true;
  s = (std::string) queuename;
  return true;
}

bool JSDLJob::get_stdin(std::string& s) {
  Arc::XMLNode inputNode = jsdl_hpcpa["Input"];
  if( (bool)inputNode )  {
    std::string value = (std::string) inputNode;
    strip_spaces(value);
    s = value;
    return true;
  }

  inputNode = jsdl_posix["Input"];
  if( (bool)inputNode )  {
    std::string value = (std::string) inputNode;
    strip_spaces(value);
    s = value;
  } 
  return true;
}

bool JSDLJob::get_stdout(std::string& s) {
  Arc::XMLNode outputNode = jsdl_hpcpa["Output"];
  if( (bool)outputNode )  {
    std::string value = (std::string) outputNode;
    strip_spaces(value);
    s = value;
    return true;
  }

  outputNode = jsdl_posix["Output"];
  if( (bool)outputNode )  {
    std::string value = (std::string) outputNode;
    strip_spaces(value);
    s = value;
  } 
  return true;
}

bool JSDLJob::get_stderr(std::string& s) {
  Arc::XMLNode errorNode = jsdl_hpcpa["Error"];
  if( (bool)errorNode )  {
    std::string value = (std::string) errorNode;
    strip_spaces(value);
    s = value;
    return true;
  }

  errorNode = jsdl_posix["Error"];
  if( (bool)errorNode )  {
    std::string value = (std::string) errorNode;
    strip_spaces(value);
    s = value;
  }
  return true;
}

bool JSDLJob::get_data(std::list<FileData>& inputdata,int& downloads,
                       std::list<FileData>& outputdata,int& uploads) {
  inputdata.clear(); downloads=0;
  outputdata.clear(); uploads=0;
  for(int i=0; (bool)(jsdl_document["JobDescription"]["DataStaging"][i]); i++) {
    Arc::XMLNode ds = jsdl_document["JobDescription"]["DataStaging"][i];
    Arc::XMLNode fileSystemNameNode = ds["FilesystemName"];
    if( (bool)fileSystemNameNode ) {
      logger.msg(Arc::ERROR, "FilesystemName defined in job description - all files must be relative to session directory[jsdl:FilesystemName]");
      return false;
    };
    Arc::XMLNode sourceNode = ds["Source"];
    Arc::XMLNode targetNode = ds["Target"];
    Arc::XMLNode filenameNode = ds["FileName"];
    // CreationFlag - ignore
    // DeleteOnTermination - ignore
    if( ( !sourceNode ) && ( !targetNode ) ) {
      // Neither in nor out - must be file generated by job
      FileData fdata( ( (std::string) filenameNode ).c_str(),"" );
      outputdata.push_back(fdata);
    } else {
      if( (bool)sourceNode ) {
        Arc::XMLNode sourceURINode = sourceNode["URI"];
        if( !sourceURINode ) {
          // Source without URL - uploaded by client
          FileData fdata( ( (std::string) filenameNode ).c_str(),"");
          inputdata.push_back(fdata);
        } else {
          FileData fdata( ((std::string) filenameNode ).c_str(),
                          ((std::string) sourceURINode ).c_str());
          inputdata.push_back(fdata);
          if(fdata.has_lfn()) downloads++;
        };
      };
      if( (bool)targetNode ) {
        Arc::XMLNode targetURINode = targetNode["URI"];
        if( !targetURINode ) {
          // Destination without URL - keep after execution
          // TODO: check for DeleteOnTermination
          FileData fdata( ( (std::string) filenameNode ).c_str(),"");
          outputdata.push_back(fdata);
        } else {
          FileData fdata( ((std::string) filenameNode ).c_str(),
                          ((std::string) targetURINode ).c_str());
          outputdata.push_back(fdata);
          if(fdata.has_lfn()) uploads++;
        };
      };
    };
  };
  return true;
}

double JSDLJob::get_limit(Arc::XMLNode range)
{
  if (!range) return 0.0;
  Arc::XMLNode n = range["UpperBound"];
  if ((bool)n) {
     char * pEnd;
     return strtod(((std::string)n).c_str(), &pEnd);
  }
  n = range["LowerBound"];
  if ((bool)n) {
      char * pEnd;
      return strtod(((std::string)n).c_str(), &pEnd);
  }
  return 0.0;
} 

bool JSDLJob::get_memory(int& memory) {

  Arc::XMLNode memoryLimitNode = jsdl_posix["MemoryLimit"];
  if( (bool)memoryLimitNode ) {
    char * pEnd;
    memory=strtol( ((std::string) memoryLimitNode ).c_str(), &pEnd, 10 );
    return true;
  };
  Arc::XMLNode resourceNode = jsdl_document["JobDescription"]["Resources"];
  if( !resourceNode ) return true;
  Arc::XMLNode totalPhysicalMemoryNode = resourceNode["TotalPhysicalMemory"];
  if( (bool)totalPhysicalMemoryNode ) {
    memory=(int)(get_limit( totalPhysicalMemoryNode )+0.5);
    return true;
  };
  Arc::XMLNode individualPhysicalMemoryNode = resourceNode["IndividualPhysicalMemory"];
  if( (bool)individualPhysicalMemoryNode ) {
    memory=(int)(get_limit( individualPhysicalMemoryNode )+0.5);
    return true;
  };
  return true;
}

bool JSDLJob::get_cputime(int& t) {

  Arc::XMLNode CPUTimeLimitNode = jsdl_posix["CPUTimeLimit"];
  if( (bool)CPUTimeLimitNode ) {
    char * pEnd;
    t=strtol( ((std::string) CPUTimeLimitNode).c_str(), &pEnd, 10 );
    return true;
  };
  Arc::XMLNode resourceNode = jsdl_document["JobDescription"]["Resources"];
  if( !resourceNode ) return true;
  Arc::XMLNode totalCPUTimeNode = resourceNode["TotalCPUTime"];
  if( (bool)totalCPUTimeNode ) {
    t=(int)(get_limit( totalCPUTimeNode )+0.5);
    return true;
  };
  Arc::XMLNode individualCPUTimeNode = resourceNode ["IndividualCPUTime"];
  if( (bool)individualCPUTimeNode ) {
    t=(int)(get_limit( individualCPUTimeNode )+0.5);
    return true;
  };
  return true;
}

bool JSDLJob::get_walltime(int& t) {

  Arc::XMLNode wallTimeLimitNode = jsdl_posix["WallTimeLimit"];
  if( (bool)wallTimeLimitNode ) {
    char * pEnd;
    t=strtol(((std::string) wallTimeLimitNode ).c_str(), &pEnd, 10 );
    return true;
  };
  return get_cputime(t);
}

bool JSDLJob::get_count(int& n) {
  Arc::XMLNode resourceNode = jsdl_document["JobDescription"]["Resources"];

  if( !resourceNode ) return true;
  Arc::XMLNode totalCPUCountNode = resourceNode["TotalCPUCount"];
  Arc::XMLNode individualCPUCountNode = resourceNode["IndividualCPUCount"];

  if( (bool)totalCPUCountNode ) {
    n=(int)(get_limit( totalCPUCountNode )+0.5);
    return true;
  };
  if( (bool)individualCPUCountNode ) {
    n=(int)(get_limit( individualCPUCountNode )+0.5);
    return true;
  };
  return true;
}

bool JSDLJob::get_reruns(int& n) {
  Arc::XMLNode rerunNode = jsdl_document["JobDescription"]["Reruns"];
  if( (bool)rerunNode ) { 
    char * pEnd;
    n= strtol( ((std::string) rerunNode ).c_str(), &pEnd, 10 );
  };
  return true;
}

bool JSDLJob::check(void) {
  if(!jsdl_document) {
    logger.msg(Arc::ERROR, "job description is missing");
    return false;
  };

  if( !(jsdl_document["JobDescription"]) ) {
    logger.msg(Arc::ERROR, "job description is missing [JobDescription] element");
    return false;
  };
  if((!jsdl_posix) && (!jsdl_hpcpa)) {
    logger.msg(Arc::ERROR, "job description is missing POSIX and HPCP application elements");
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
  if(l.size() == 0) return false; // Normally this should not happen
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
  set_posix();
  if(!check()) return false;
  std::list<std::string> arguments;
  if(!get_arguments(arguments)) return false;
  if(arguments.size() == 0) return false;
  /* executable can be external, so first check for initial slash */
  char c = arguments.begin()->c_str()[0];
  if((c != '/') && (c != '$')){
    if(canonical_dir(*(arguments.begin())) != 0) {
      logger.msg(Arc::ERROR, "Bad name for executable: %s", (*(arguments.begin())));
      return false;
    };
    fix_file_permissions(session_dir+"/"+(*(arguments.begin())),true);
  };
  std::list<std::string> execs;
  if(!get_execs(execs)) return false;
  for(std::list<std::string>::iterator i = execs.begin();i!=execs.end();++i) {
    if(canonical_dir(*i) != 0) {
      logger.msg(Arc::ERROR, "Bad name for executable: %s", (*i)); 
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
  s=""; if(!get_stdin(s)) return false;
  if(s.empty()) s=NG_RSL_DEFAULT_STDIN;
  f<<"joboption_stdin="<<value_for_shell(s.c_str(),true)<<std::endl;
  s=""; if(!get_stdout(s)) return false;
  if(s.empty()) { s=NG_RSL_DEFAULT_STDOUT; }
  else {
    if(canonical_dir(s) != 0) {
      logger.msg(Arc::WARNING, "Bad name for stdout: %s", s);
      return false;
    };
    s=session_dir+s;
  };
  f<<"joboption_stdout="<<value_for_shell(s.c_str(),true)<<std::endl;
  s=""; if(!get_stderr(s)) return false;
  if(s.length() == 0) { s=NG_RSL_DEFAULT_STDERR; }
  else {
    if(canonical_dir(s) != 0) {
      logger.msg(Arc::WARNING, "Bad name for stderr: %s", s);
      return false;
    };
    s=session_dir+s;
  };
  f<<"joboption_stderr="<<value_for_shell(s.c_str(),true)<<std::endl;
  n=0; if(!get_walltime(n)) return false;
  if(n<=0) { f<<"joboption_walltime="<<std::endl; }
  else { f<<"joboption_walltime="<<numvalue_for_shell(n)<<std::endl; };
  n=0; if(!get_cputime(n)) return false;
  if(n<=0) { f<<"joboption_cputime="<<std::endl; }
  else { f<<"joboption_cputime="<<numvalue_for_shell(n)<<std::endl; };
  n=0; if(!get_memory(n)) return false;
  if(n<=0) { f<<"joboption_memory="<<std::endl; }
  else { f<<"joboption_memory="<<numvalue_for_shell(n)<<std::endl; };
  n=1; if(!get_count(n)) return false;
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

    std::list<std::pair<std::string,std::string> > envs;
    std::string env_name;
    std::string env_value;
    JSDLJob::get_environments(envs);    

    std::list<std::pair<std::string,std::string> >::const_iterator env_iter;

    // TODO: we need to test this code part because I am not sure that
    //       it is working by correct way

    int env_size= envs.size();

    for (int k = 0; k < env_size; env_iter++, k++) { 

       env_name = env_iter->first;
       env_value = env_iter->second;

       f<<"joboption_env_"<<(k/2)<<"='"<<value_for_shell(env_name,false)<<"=";
 
     //  if(tmp_param[i]==NULL) { f<<"'"<<std::endl; break; };

       f<<value_for_shell(env_value,false)<<"'"<<std::endl;

   }

  /*  for(i=0;tmp_param[i]!=NULL;i++) { 
      f<<"joboption_env_"<<(i/2)<<"='"<<value_for_shell(tmp_param[i],false)<<"=";
      i++; if(tmp_param[i]==NULL) { f<<"'"<<std::endl; break; };
      f<<value_for_shell(tmp_param[i],false)<<"'"<<std::endl;
    };

   */

  std::list<std::string> rtes;
  if(!get_RTEs(rtes)) return false;
  n=0;
  for(std::list<std::string>::iterator i = rtes.begin();i!=rtes.end();++i) {
    std::string tmp_s = *i;
    for(unsigned int ii=0;ii<tmp_s.length();++ii) tmp_s[ii]=toupper(tmp_s[ii]);
    if(canonical_dir(tmp_s) != 0) {
      logger.msg(Arc::WARNING, "Bad name for runtime environment: %s", (*i));
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

void JSDLJob::print_to_grami(std::ostream&) {
}
