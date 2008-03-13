#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <sstream>

#include "job_jsdl.h"

static void strip_spaces(std::string& s) {
  int n;
  for(n=0;s[n];n++) if(!isspace(s[n])) break;
  if(n) s.erase(0,n);
  for(n=s.length()-1;n>=0;n--) if(!isspace(s[n])) break;
  s.resize(n+1);
}

double JobRequestJSDL::get_limit(Arc::XMLNode range)
{
  if (!range) return 0.0;
  Arc::XMLNode n = range["UpperBound"];
  if (bool(n) == true) {
     char * pEnd;
     return strtod(((std::string)n).c_str(), &pEnd);
  }
  n = range["LowerBound"];
  if (bool(n) == true) {
      char * pEnd;
      return strtod(((std::string)n).c_str(), &pEnd);
  }
  return 0.0;
} 

//-- Constructors --//

JobRequestJSDL::JobRequestJSDL(const char* s) throw(JobRequestError) {
	std::istringstream i(s);
	if(!set(i)) throw JobRequestError("Could not parse job description.");
}

JobRequestJSDL::JobRequestJSDL(const std::string& s) throw(JobRequestError) {
	std::istringstream i(s);
	if(!set(i)) throw JobRequestError("Could not parse job description.");
}

JobRequestJSDL::JobRequestJSDL(std::istream& i) throw(JobRequestError) {
	if(!set(i)) throw JobRequestError("Could not parse job description.");
}

//-- End of Constructors --//

//-- Descructor --//

JobRequestJSDL::~JobRequestJSDL(void) {
  //if(jsdl_document) delete jsdl_document;
}

//-- End of Descructor --//


bool JobRequestJSDL::set(std::istream& s) throw(JobRequestError) {

  std::string xml_document;
  std::string xml_line;

  //Read the input stream into a string by lines
  while (getline(s,xml_line)) xml_document += xml_line;

  (Arc::XMLNode (xml_document)).New(jsdl_document);

  if(!jsdl_document) { return false; }
  Arc::XMLNode jobDescriptionNode = jsdl_document["JobDescription"];
  if( !set( jobDescriptionNode ) ) { return false; }

  return true;

}

// This method is needed for filling attributes in sub-jobs
bool JobRequestJSDL::set(Arc::XMLNode jsdl_description_) throw(JobRequestError) {
  // Store fields in parsed stucture
  Arc::XMLNode jsdl_posix_ = jsdl_description_["Application"]["POSIXApplication"];
  Arc::XMLNode jsdl_resources_ = jsdl_description_["Resources"];

  if( bool(jsdl_description_["JobIdentification"]["JobName"]) == true ) job_name = (std::string) jsdl_description_["JobIdentification"]["JobName"];
  if( bool(jsdl_description_["ProcessingStartTime"]) == true ) start_time = (std::string) jsdl_description_["ProcessingStartTime"];
  char * pEnd;
  if( bool(jsdl_description_["Reruns"]) == true ) reruns = strtol( ((std::string) jsdl_description_["Reruns"] ).c_str(), &pEnd, 10 );
  if( bool(jsdl_description_["LocalLogging"]["Directory"]) == true ) gmlog = (std::string) jsdl_description_["LocalLogging"]["Directory"];
  if( bool(jsdl_description_["CredentialServer"]["URL"]) == true ) credentialserver = (std::string) jsdl_description_["CredentialServer"]["URL"];

  if( bool(jsdl_description_["AccessControl"]) == true ) {
    if( bool(jsdl_description_["AccessControl"]["Type"]) == true || (std::string) jsdl_description_["AccessControl"]["Type"] == "GACL" ) {
      if( bool( jsdl_description_["AccessControl"]["Content"] ) == true ) acl = (std::string) jsdl_description_["AccessControl"]["Content"];
    };
  };

  if( jsdl_resources_ ) {
    if( bool(jsdl_resources_["SessionLifeTime"]) == true ) lifetime = strtol( ((std::string) jsdl_resources_["SessionLifeTime"]).c_str(),&pEnd,10);
    if( bool(jsdl_resources_["GridTimeLimit"]) == true ) grid_time = strtol( ((std::string) jsdl_resources_["GridTimeLimit"]).c_str(), &pEnd, 10 );
    if( bool(jsdl_resources_["CandidateTarget"]) == true ) {
      if( bool(jsdl_resources_["CandidateTarget"]["HostName"]) == true ) cluster = (std::string) jsdl_resources_["CandidateTarget"]["HostName"];
      if( bool(jsdl_resources_["CandidateTarget"]["QueueName"]) == true ) queue = (std::string) jsdl_resources_["CandidateTarget"]["QueueName"];
    };

    if( bool(jsdl_resources_["CPUArchitecture"]) == true ) architecture = (std::string) jsdl_resources_["CPUArchitecture"];
    for( int i=0; bool(jsdl_resources_["RunTimeEnvironment"][i]) == true; ++i ) {
      std::string s = (std::string) jsdl_resources_["RunTimeEnvironment"][i]["Name"];
      
      Arc::XMLNode versionNode = jsdl_resources_["RunTimeEnvironment"][i]["Version"];
      if( versionNode ) {
        if( bool(versionNode["UpperExclusive"]) == true ) continue; // not supported
        if( bool(versionNode["LowerExclusive"]) == true ) continue; // not supported
        if( ( bool(versionNode["Exclusive"]) == true ) && !( ( (std::string) versionNode["Exclusive"] ) == "true"  ) ) continue; // not supported
        if( bool(versionNode["Exact"][1]) == true ) continue; // not supported
        if( bool(versionNode["Exact"]) == true ) {
          s+="="; s+=(std::string) versionNode["Exact"];
        };
      };
      runtime_environments.push_back(RuntimeEnvironment(s));
    };

    if( bool(jsdl_resources_["TotalCPUCount"]) == true ) count=(int)(get_limit( jsdl_resources_["TotalCPUCount"] )+0.5);
    else {
      if( bool(jsdl_resources_["IndividualCPUCount"]) == true ) count=(int)(get_limit( jsdl_resources_["IndividualCPUCount"] )+0.5);
    }

    if( bool(jsdl_resources_["TotalPhysicalMemory"]) == true ) memory=(int)(get_limit( jsdl_resources_["TotalPhysicalMemory"] )+0.5);
    else {
      if( bool(jsdl_resources_["IndividualPhysicalMemory"]) == true ) memory=(int)(get_limit( jsdl_resources_["IndividualPhysicalMemory"] )+0.5);
    }

    if( bool(jsdl_resources_["TotalDiskSpace"]) == true ) disk=(int)(get_limit( jsdl_resources_["TotalDiskSpace"] )+0.5);
    else {
      if( bool(jsdl_resources_["IndividualDiskSpace"]) == true ) disk=(int)(get_limit( jsdl_resources_["IndividualDiskSpace"] )+0.5);
    }

    if( bool(jsdl_resources_["TotalCPUTime"]) == true ) cpu_time=(int)(get_limit( jsdl_resources_["TotalCPUTime"] )+0.5);
    else {
      if( bool(jsdl_resources_["IndividualCPUTime"]) == true ) cpu_time=(int)(get_limit( jsdl_resources_["IndividualCPUTime"] )+0.5);
    }

    for( int i=0; bool(jsdl_resources_["Middleware"][i]) == true; ++i ) {
      std::string s = (std::string) jsdl_resources_["Middleware"][i]["Name"];
      
      Arc::XMLNode versionNode = jsdl_resources_["Middleware"][i]["Version"];
      if( versionNode ) {
        if( bool(versionNode["UpperExclusive"]) == true ) continue; // not supported
        if( bool(versionNode["LowerExclusive"]) == true ) continue; // not supported
        if( ( bool(versionNode["Exclusive"]) == true ) && !( ( (std::string) versionNode["Exclusive"] ) == "true"  ) ) continue; // not supported
        if( bool(versionNode["Exact"][1]) == true ) continue; // not supported
        if( bool(versionNode["Exact"]) == true ) {
          s+="="; s+=(std::string) versionNode;
        };
      };
      middlewares.push_back(RuntimeEnvironment(s));
    };
  };

  if(jsdl_posix_) {
    if( bool(jsdl_posix_["Input"]) == true ) {
      sstdin = (std::string) jsdl_posix_["Input"];
      strip_spaces(sstdin);
    };

    if( bool(jsdl_posix_["Output"]) == true ) {
      sstdout = (std::string) jsdl_posix_["Output"];
      strip_spaces(sstdout);
    };

    if( bool(jsdl_posix_["Error"]) == true ) {
      sstderr = (std::string) jsdl_posix_["Error"];
      strip_spaces(sstderr);
    };

    if( bool(jsdl_posix_["Executable"]) == true ) strip_spaces(*(arguments.insert(arguments.end(), (std::string) jsdl_posix_["Executable"] )));
    else {
      arguments.insert(arguments.end(),"");
    }

    for( int i=0; bool(jsdl_posix_["Argument"][i]) == true; ++i ) {
      strip_spaces(*(arguments.insert(arguments.end(),(std::string) jsdl_posix_["Argument"][i] )));
    };

    char * pEnd;
    if( bool(jsdl_posix_["MemoryLimit"]) == true )  memory= strtol( ((std::string) jsdl_posix_["MemoryLimit"] ).c_str(), &pEnd, 10 );
    if( bool(jsdl_posix_["CPUTimeLimit"]) == true ) cpu_time = strtol( ((std::string) jsdl_posix_["CPUTimeLimit"] ).c_str(), &pEnd, 10 );
    if( bool(jsdl_posix_["WallTimeLimit"]) == true ) wall_time = strtol( ((std::string) jsdl_posix_["WallTimeLimit"] ).c_str(), &pEnd, 10 );

    for( int i=0 ; bool(jsdl_description_["RemoteLogging"][i]) == true ; ++i ) {
      loggers.push_back( (std::string) jsdl_description_["RemoteLogging"]["URL"] );
    }

    for( int i=0 ; bool(jsdl_description_["DataStaging"][i]) == true ; ++i ) { 

      // FilesystemName - ignore
      // CreationFlag - ignore
      // DeleteOnTermination - ignore

      Arc::XMLNode sourceNode = jsdl_description_["DataStaging"][i]["Source"];
      Arc::XMLNode targetNode = jsdl_description_["DataStaging"][i]["Target"];
      Arc::XMLNode filenameNode = jsdl_description_["DataStaging"][i]["FileName"];

      if( ( bool(sourceNode) != true ) && ( bool(targetNode) != true ) ) {
        // Neither in nor out - must be file generated by job
        outputdata.push_back(JobRequest::OutputFile( (std::string) filenameNode ,""));
      } else {
        if( bool(sourceNode) == true ) {
          if( bool(jsdl_description_["DataStaging"][i]["Source"]["IsExecutable"]) == true &&
              ( (std::string) jsdl_description_["DataStaging"][i]["Source"]["IsExecutable"] ) == "true" ) {
            executables.push_back( (std::string) filenameNode );
          };
          if( bool(sourceNode["URI"]) != true ) {
            // Source without URL - uploaded by client
            Arc::XMLNode fileParameterNode = jsdl_description_["DataStaging"][i]["FileParamters"];
            if( bool(fileParameterNode) == true ) {
              inputdata.push_back(JobRequest::InputFile( (std::string) filenameNode, (std::string) fileParameterNode ) );
            } else {
              inputdata.push_back(JobRequest::InputFile( (std::string) filenameNode,""));
            };
          } else {
            inputdata.push_back(JobRequest::InputFile( (std::string) filenameNode, (std::string) sourceNode["URI"] ) );
          };
        };
        if( bool(targetNode) == true ) {
          if( bool(targetNode["URI"]) != true ) {
            // Destination without URL - keep after execution
            // TODO: check for DeleteOnTermination
            outputdata.push_back(JobRequest::OutputFile( (std::string) filenameNode ,""));
          } else {
            outputdata.push_back(JobRequest::OutputFile( (std::string) filenameNode,  (std::string) targetNode["URI"] ) );
          };
        };
      };
    };

  for( int i=0; bool(jsdl_document["JobDescription"]["Notify"][i]) == true; ++i ) {
    Arc::XMLNode typeNode = jsdl_document["JobDescription"]["Notify"][i]["Type"];
    Arc::XMLNode endpointNode = jsdl_document["JobDescription"]["Notify"][i]["Endpoint"];
    Arc::XMLNode stateNode = jsdl_document["JobDescription"]["Notify"][i]["State"];
    if( ( bool(typeNode) != true || ( (std::string) typeNode ) == "Email") && bool(endpointNode) == true && bool(stateNode) == true) {
      std::string flags;
      for( int j=0; bool(stateNode[i]) == true; ++j) {
        std::string value = (std::string) stateNode[i];
        if ( value == "PREPARING" ) {
          flags+="b";
        } else if ( value == "INLRMS" ) {
          flags+="q";
        } else if ( value == "FINISHING" ) {
          flags+="f";
        } else if ( value == "FINISHED" ) {
          flags+="e";
        } else if ( value == "DELETED" ) {
          flags+="d";
        } else if ( value == "CANCELING" ) {
          flags+="c";
        };
      };
      if(flags.length()) {
          notifications.push_back(Notification(flags, (std::string) endpointNode ));
      };
    };
  };

  // Parse sub-jobs
  for(int i=0; bool( jsdl_description_["jsdl:JobDescription"][i]) == true; ++i) {
    JobRequest* jr = new JobRequest;
    if(!(((JobRequestJSDL*)jr)->set(jsdl_description_["jsdl:JobDescription"][i]))) return false;
    alternatives.push_back(jr);
  };
  }
  return true;
}

bool JobRequestJSDL::print(std::string& s) throw(JobRequestError) {
  if(!jsdl_document) return false;
  jsdl_document.GetXML(s);
  return true;
}

JobRequestJSDL::JobRequestJSDL(const JobRequest& j) throw(JobRequestError) {
	(*this)=j;
}


JobRequest& JobRequestJSDL::operator=(const JobRequest& /*j*/) throw (JobRequestError) {
throw JobRequestError("CRITICAL ERROR!!!!: JobRequest& JobRequestJSDL::operator=(const JobRequest& j) operator isn't implemented!");
/*
  JobRequest::operator=(j);
  (Arc::XMLNode (jsdl_namespaces, "jsdl:JobDefinition")).New(jsdl_document);
  if(!jobDefinition) return *this;
  Arc::XMLNode jobDescription = jobDefinition.NewChild("jsdl:JobDescription");
  if(!(jobDescription)) return *this;
  if(!set_jsdl(jobDescription)) return *this;
*/
  return *this;
}



/*

static jsdl__RangeValue_USCOREType* set_limit(struct soap* sp_,double v) {
	jsdl__RangeValue_USCOREType* range = soap_new_jsdl__RangeValue_USCOREType(sp_,-1);
	if(!range) return NULL;
	range->soap_default(sp_);
	range->UpperBoundedRange=soap_new_jsdl__Boundary_USCOREType(sp_,-1);
	if(!range->UpperBoundedRange) return NULL;
	range->UpperBoundedRange->soap_default(sp_);
	range->UpperBoundedRange->__item=v;
	return range;
}

*/

/*
bool JobRequestJSDL::set_jsdl(Arc::XMLNode jobDescription) {
  // Creating necessary items
  Arc::XMLNode application = jobDescription.NewChild("jsdl:Application");
  if(!application) return false;

  Arc::XMLNode jobPosix = application.NewChild("jsdl-posix:POSIXApplication");
  if(!jobPosix) return false;

  Arc::XMLNode resources = jobDescription.NewChild("jsdl:Resources");
  if(!resources) return false;

  // Making items on demand
  if(job_name.length()) {
    Arc::XMLNode identification = jobDescription.NewChild("jsdl:JobIdentification");
    if(!identification) return false;
    Arc::XMLNode jobName = identification.NewChild("jsdl:JobName");
    if(!jobName) return false;
    jobName.Set(job_name);
  };
/////////////////////////////-//---------------------------------//-/////////////////////////////

  if(acl.length()) {
    job_description_->jsdlARC__AccessControl=soap_new_jsdlARC__AccessControl_USCOREType(sp_,-1);
    if(!(job_description_->jsdlARC__AccessControl)) goto error;
    job_description_->jsdlARC__AccessControl->soap_default(sp_);
    job_description_->jsdlARC__AccessControl->Type=(jsdlARC__AccessControlType_USCOREType*)soap_malloc(sp_,sizeof(jsdlARC__AccessControlType_USCOREType));
    if(!(job_description_->jsdlARC__AccessControl->Type)) goto error;
    *(job_description_->jsdlARC__AccessControl->Type)=jsdlARC__AccessControlType_USCOREType__GACL;
    job_description_->jsdlARC__AccessControl->Content=soap_new_std__string(sp_,-1);
    if(!(job_description_->jsdlARC__AccessControl->Content)) goto error;
    *(job_description_->jsdlARC__AccessControl->Content)=acl;
  };

  if(start_time != Arc::Time(-1)) {
    job_description_->jsdlARC__ProcessingStartTime=(time_t*)soap_malloc(sp_,sizeof(time_t));
    if(!(job_description_->jsdlARC__ProcessingStartTime)) goto error;
    *(job_description_->jsdlARC__ProcessingStartTime)=start_time.GetTime();
  };

  if(sstdin.length()) {
    job_posix_->Input=soap_new_jsdlPOSIX__FileName_USCOREType(sp_,-1);
    if(!(job_posix_->Input)) goto error;
    job_posix_->Input->soap_default(sp_);
    job_posix_->Input->__item=sstdin;
  };

  if(sstdout.length()) {
    job_posix_->Output=soap_new_jsdlPOSIX__FileName_USCOREType(sp_,-1);
    if(!(job_posix_->Output)) goto error;
    job_posix_->Output->soap_default(sp_);
    job_posix_->Output->__item=sstdout;
  };

  if(sstderr.length()) {
    job_posix_->Error=soap_new_jsdlPOSIX__FileName_USCOREType(sp_,-1);
    if(!(job_posix_->Error)) goto error;
    job_posix_->Error->soap_default(sp_);
    job_posix_->Error->__item=sstderr;
  };

  if(reruns >= 0) {
    job_description_->jsdlARC__Reruns=(jsdlARC__Reruns_USCOREType*)soap_malloc(sp_,sizeof(jsdlARC__Reruns_USCOREType));
    if(!(job_description_->jsdlARC__Reruns)) goto error;
    *(job_description_->jsdlARC__Reruns)=reruns;
  };

  if(gmlog.length()) {
    job_description_->jsdlARC__LocalLogging=soap_new_jsdlARC__LocalLogging_USCOREType(sp_,-1);
    if(!(job_description_->jsdlARC__LocalLogging)) goto error;
    job_description_->jsdlARC__LocalLogging->soap_default(sp_);
    job_description_->jsdlARC__LocalLogging->Directory=gmlog;
  };

  if(credentialserver.length()) {
    job_description_->jsdlARC__CredentialServer=soap_new_jsdlARC__CredentialServer_USCOREType(sp_,-1);
    if(!(job_description_->jsdlARC__CredentialServer)) goto error;
    job_description_->jsdlARC__CredentialServer->soap_default(sp_);
    job_description_->jsdlARC__CredentialServer->URL=credentialserver;
  };

  if(lifetime>=0) {
    job_resources_->jsdlARC__SessionLifeTime=(jsdlARC__SessionLifeTime_USCOREType*)soap_malloc(sp_,sizeof(jsdlARC__SessionLifeTime_USCOREType));
    if(!(job_resources_->jsdlARC__SessionLifeTime)) goto error;
    *(job_resources_->jsdlARC__SessionLifeTime)=lifetime;
  };

  if(cluster.length() || queue.length()) {
    job_resources_->jsdlARC__CandidateTarget=soap_new_jsdlARC__CandidateTarget_USCOREType(sp_,-1);
    if(!(job_resources_->jsdlARC__CandidateTarget)) goto error;
    job_resources_->jsdlARC__CandidateTarget->soap_default(sp_);
    if(cluster.length()) {
      job_resources_->jsdlARC__CandidateTarget->HostName=soap_new_std__string(sp_,-1);
      if(!(job_resources_->jsdlARC__CandidateTarget->HostName)) goto error;
      *(job_resources_->jsdlARC__CandidateTarget->HostName)=cluster;
    };
    if(queue.length()) {
      job_resources_->jsdlARC__CandidateTarget->QueueName=soap_new_std__string(sp_,-1);
      if(!(job_resources_->jsdlARC__CandidateTarget->QueueName)) goto error;
      *(job_resources_->jsdlARC__CandidateTarget->QueueName)=queue;
    };
  };

  if(architecture.length()) {
    job_resources_->CPUArchitecture=soap_new_jsdl__CPUArchitecture_USCOREType(sp_,-1);
    if(!(job_resources_->CPUArchitecture)) goto error;
    job_resources_->CPUArchitecture->soap_default(sp_);
    if(soap_s2jsdl__ProcessorArchitectureEnumeration(sp_,architecture.c_str(),&(job_resources_->CPUArchitecture->CPUArchitectureName)) != SOAP_OK) {
      job_resources_->CPUArchitecture->CPUArchitectureName=jsdl__ProcessorArchitectureEnumeration__other;
    };
  };

  if(grid_time>=0) {
    job_resources_->jsdlARC__GridTimeLimit=(jsdlARC__GridTimeLimit_USCOREType*)soap_malloc(sp_,sizeof(jsdlARC__GridTimeLimit_USCOREType));
    if(!(job_resources_->jsdlARC__GridTimeLimit)) goto error;
    *(job_resources_->jsdlARC__GridTimeLimit)=grid_time;
  };

  if(count >= 0) {
    job_resources_->TotalCPUCount=set_limit(sp_,count);
    if(!job_resources_->TotalCPUCount) goto error;
  };

  if(memory>=0) {
    job_posix_->MemoryLimit=soap_new_jsdlPOSIX__Limits_USCOREType(sp_,-1);
    if(!job_posix_->MemoryLimit) goto error;
    job_posix_->MemoryLimit->soap_default(sp_);
    job_posix_->MemoryLimit->__item=memory;
  };

  if(disk>=0) {
    job_resources_->TotalDiskSpace=set_limit(sp_,disk);
    if(!job_resources_->TotalDiskSpace) goto error;
  };

  if(cpu_time>=0) {
    job_posix_->CPUTimeLimit=soap_new_jsdlPOSIX__Limits_USCOREType(sp_,-1);
    if(!job_posix_->CPUTimeLimit) goto error;
    job_posix_->CPUTimeLimit->soap_default(sp_);
    job_posix_->CPUTimeLimit->__item=cpu_time;
  };

  if(wall_time>=0) {
    job_posix_->WallTimeLimit=soap_new_jsdlPOSIX__Limits_USCOREType(sp_,-1);
    if(!job_posix_->WallTimeLimit) goto error;
    job_posix_->WallTimeLimit->soap_default(sp_);
    job_posix_->WallTimeLimit->__item=wall_time;
  };

  if(inputdata.size()) {
    std::vector<jsdl__DataStaging_USCOREType*>& ds = job_description_->DataStaging;
    for(std::list<JobRequest::InputFile>::iterator i = inputdata.begin(); i!=inputdata.end();++i) {
      jsdl__DataStaging_USCOREType* item = soap_new_jsdl__DataStaging_USCOREType(sp_,-1);
      if(!item) goto error;
      item->soap_default(sp_);
      item->FileName=i->name;
      std::string src = i->source.str();
      if(src.length()) {
        jsdl__SourceTarget_USCOREType* source = soap_new_jsdl__SourceTarget_USCOREType(sp_,-1);
        if(!source) goto error;
        source->soap_default(sp_);
        source->URI=soap_new_xsd__anyURI(sp_,-1);
        if(!(source->URI)) goto error;
        *(source->URI)=src;
        item->Source=source;
      };
      if(i->parameters.length()) {
        std::string* parameters = soap_new_std__string(sp_,-1);
        if(!parameters) goto error;
        *(parameters)=i->parameters;
        item->jsdlARC__FileParameters=parameters;
      };
      ds.push_back(item);
    };
  };

  if(outputdata.size()) {
    std::vector<jsdl__DataStaging_USCOREType*>& ds = job_description_->DataStaging;
    for(std::list<JobRequest::OutputFile>::iterator i = outputdata.begin(); i!=outputdata.end();++i) {
      jsdl__DataStaging_USCOREType* item = soap_new_jsdl__DataStaging_USCOREType(sp_,-1);
      if(!item) goto error;
      item->FileName=i->name;
      std::string dest = i->destination.str();
      if(dest.length()) {
        jsdl__SourceTarget_USCOREType* target = soap_new_jsdl__SourceTarget_USCOREType(sp_,-1);
        if(!target) goto error;
        target->soap_default(sp_);
        target->URI=soap_new_xsd__anyURI(sp_,-1);
        if(!(target->URI)) goto error;
        *(target->URI)=dest;
        item->Target=target;
      };
      ds.push_back(item);
    };
  };

  if(loggers.size()) {
    std::vector<jsdlARC__RemoteLogging_USCOREType*>& logs = job_description_->jsdlARC__RemoteLogging;
    for(std::list<std::string>::iterator i = loggers.begin(); i!=loggers.end();++i) {
      jsdlARC__RemoteLogging_USCOREType* log = soap_new_jsdlARC__RemoteLogging_USCOREType(sp_,-1);
      if(!log) goto error;
      log->soap_default(sp_);
      log->URL=*i;
      logs.push_back(log);
    };
  };

  if(arguments.size()) {
    std::list<std::string>::iterator arg = arguments.begin();
    job_posix_->Executable=soap_new_jsdlPOSIX__FileName_USCOREType(sp_,-1);
    if(!job_posix_->Executable) goto error;
    job_posix_->Executable->soap_default(sp_);
    job_posix_->Executable->__item=*arg;
    ++arg; for(;arg!=arguments.end();++arg) {
      jsdlPOSIX__Argument_USCOREType* a=soap_new_jsdlPOSIX__Argument_USCOREType(sp_,-1);
      if(!a) goto error;
      a->__item=*arg;
      job_posix_->Argument.push_back(a);
    };
  };

  if(runtime_environments.size()) {
    std::vector<jsdlARC__RunTimeEnvironment_USCOREType*>& rtes =
    job_resources_->jsdlARC__RunTimeEnvironment;
    for(std::list<RuntimeEnvironment>::iterator i = runtime_environments.begin();i!=runtime_environments.end();++i) {
      jsdlARC__RunTimeEnvironment_USCOREType* rte = soap_new_jsdlARC__RunTimeEnvironment_USCOREType(sp_,-1);
      if(!rte) goto error; 
      rte->soap_default(sp_);
      rte->Name=i->Name();
      rte->Version=soap_new_jsdlARC__Version_USCOREType(sp_,-1);
      if(!(rte->Version)) goto error;
      rte->Version->soap_default(sp_);
      rte->Version->Exact.push_back(i->Version());
      rtes.push_back(rte);
    };
  };

  if(middlewares.size()) {
    std::vector<jsdlARC__Middleware_USCOREType*>& mws = job_resources_->jsdlARC__Middleware;
    for(std::list<RuntimeEnvironment>::iterator i = middlewares.begin(); i!=middlewares.end();++i) {
      jsdlARC__Middleware_USCOREType* mw = soap_new_jsdlARC__Middleware_USCOREType(sp_,-1);
      if(!mw) goto error;
      mw->soap_default(sp_);
      mw->Name=i->Name();
      mw->Version=soap_new_jsdlARC__Version_USCOREType(sp_,-1);
      if(!(mw->Version)) goto error;
      mw->Version->soap_default(sp_);
      mw->Version->Exact.push_back(i->Version());
      mws.push_back(mw);
    };
  };

  if(notifications.size()) {
    std::vector<jsdlARC__Notify_USCOREType*>& nts =job_description_->jsdlARC__Notify;
    for(std::list<Notification>::iterator i = notifications.begin(); i!=notifications.end();++i) {
      jsdlARC__Notify_USCOREType* nt = soap_new_jsdlARC__Notify_USCOREType(sp_,-1);
      if(!nt) goto error;
      nt->soap_default(sp_);
      nt->Type=(jsdlARC__NotificationType_USCOREType*)soap_malloc(sp_,sizeof(jsdlARC__NotificationType_USCOREType));
      if(!(nt->Type)) goto error;
      *(nt->Type)=jsdlARC__NotificationType_USCOREType__Email;
      nt->Endpoint=soap_new_std__string(sp_,-1);
      if(!(nt->Endpoint)) goto error;
      *(nt->Endpoint)=i->email;
      for(unsigned int n = 0;n<(i->flags.length());++n) {
        switch((i->flags.c_str())[n]) {
          case 'b':
            nt->State.push_back(jsdlARC__GMState_USCOREType__PREPARING);
            break;
          case 'q':
            nt->State.push_back(jsdlARC__GMState_USCOREType__INLRMS);
            break;
          case 'f':
            nt->State.push_back(jsdlARC__GMState_USCOREType__FINISHING);
            break;
          case 'e':
            nt->State.push_back(jsdlARC__GMState_USCOREType__FINISHED);
            break;
          case 'd':
            nt->State.push_back(jsdlARC__GMState_USCOREType__DELETED);
            break;
          case 'c':
            nt->State.push_back(jsdlARC__GMState_USCOREType__CANCELING);
            break;
          default: break;
        };
      };
      nts.push_back(nt);
    };
  };

  for(std::list<JobRequest*>::const_iterator i = alternatives.begin();i!=alternatives.end();++i) {
    if(!(*i)) continue;
    jsdl__JobDescription_USCOREType* job_description__ = soap_new_jsdl__JobDescription_USCOREType(sp_,-1);
    if(!(job_description__)) goto error;
    job_description__->soap_default(sp_);
    if(!((JobRequestJSDL*)(*i))->set_jsdl(job_description__,sp_)) goto error;
    job_description_->JobDescription.push_back(job_description__);
  };

//    executables.clear();

  return true;
//error:
//  return false;
}
*/
