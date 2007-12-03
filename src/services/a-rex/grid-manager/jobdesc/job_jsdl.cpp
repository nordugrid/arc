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
/*
//Had to change because of the parameter type - the function isn't mentioned in the header file so it was possible
static double get_limit(jsdl__RangeValue_USCOREType* range) {
  if(range == NULL) return 0.0;
  if(range->UpperBoundedRange) return range->UpperBoundedRange->__item;
  if(range->LowerBoundedRange) return range->LowerBoundedRange->__item;
  return 0.0;
}
*/

double JobRequestJSDL::get_limit(Arc::XMLNode range) {
  if(!range) return 0.0;
  std::list<Arc::XMLNode> upper = range.XPathLookup("//jsdl:UpperBound", jsdl_namespaces);
  std::list<Arc::XMLNode> lower = range.XPathLookup("//jsdl:LowerBound", jsdl_namespaces);
  if( upper.size() != 0 ) return atof( ( (std::string) upper.front() ).c_str() );
  if( lower.size() != 0 ) return atof( ( (std::string) lower.front() ).c_str() );
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

  jsdl_namespaces["jsdl"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl";
  jsdl_namespaces["jsdl-posix"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
  jsdl_namespaces["jsdl-arc"] = "http://www.nordugrid.org/ws/schemas/jsdl-arc";  

  std::string xml_document;
  std::string xml_line;

  //Read the input stream into a string by lines
  while (getline(s,xml_line)) xml_document += xml_line;

  (Arc::XMLNode (xml_document)).New(jsdl_document);

  if(!jsdl_document) { return false; }
  //std::list<Arc::XMLNode> jobDefinitionNodes = jsdl_document.XPathLookup("/jsdl:JobDefinition", jsdl_namespaces);
  //if( jobDefinitionNodes.size() == 0 ){ return false; }
  std::list<Arc::XMLNode> jobDescriptionNodes = jsdl_document.XPathLookup("//jsdl:JobDescription", jsdl_namespaces);
  if( !set( jobDescriptionNodes.front() ) ) { return false; }

  return true;

}

// This method is needed for filling attributes in sub-jobs
bool JobRequestJSDL::set(Arc::XMLNode jsdl_description_) throw(JobRequestError) {
  // Store fields in parsed stucture
  Arc::XMLNode jsdl_posix_ = jsdl_description_.XPathLookup("//jsdl:Application//jsdl-posix:POSIXApplication", jsdl_namespaces).front();
  Arc::XMLNode jsdl_resources_ = jsdl_description_.XPathLookup("//jsdl:Resources", jsdl_namespaces).front();

  std::list<Arc::XMLNode> jobNameNodes = jsdl_description_.XPathLookup("//jsdl:JobIdentification//jsdl:JopName", jsdl_namespaces);
  if( jobNameNodes.size() != 0 ) {
    job_name = (std::string) jobNameNodes.front();
  };

  std::list<Arc::XMLNode> aclNodes = jsdl_description_.XPathLookup("//jsdl-arc:AccessControl", jsdl_namespaces);
  if( aclNodes.size() != 0 ) {
    std::list<Arc::XMLNode> aclTypeNodes = aclNodes.front().XPathLookup("//jsdl-arc:Type", jsdl_namespaces);
    if( aclTypeNodes.size() != 0 || (std::string) aclTypeNodes.front() == "GACL" ) {
      std::list<Arc::XMLNode> aclContentNodes = aclNodes.front().XPathLookup("//jsdl-arc:Content", jsdl_namespaces);
      if( aclContentNodes.size() != 0 )
        acl = (std::string) aclContentNodes.front();
    };
  };

  std::list<Arc::XMLNode> pstNodes = jsdl_description_.XPathLookup("//jsdl-arc:ProcessingStartTime", jsdl_namespaces);
  if( pstNodes.size() != 0 )
    start_time = (std::string) pstNodes.front();

  std::list<Arc::XMLNode> rrNodes = jsdl_description_.XPathLookup("//jsdl-arc:Reruns", jsdl_namespaces);
  if( rrNodes.size() != 0 ) 
    reruns = atoi( ((std::string) rrNodes.front()).c_str() );

  std::list<Arc::XMLNode> llNodes = jsdl_description_.XPathLookup("//jsdl-arc:LocalLogging", jsdl_namespaces);
  if( llNodes.size() != 0 ) {
    std::list<Arc::XMLNode> lldNodes = jsdl_description_.XPathLookup("//jsdl-arc:LocalLogging//jsdl-arc:Directory", jsdl_namespaces);
    gmlog = (std::string) lldNodes.front();
  };

  std::list<Arc::XMLNode> csNodes = jsdl_description_.XPathLookup("//jsdl-arc:CredentialServer", jsdl_namespaces);
  if( csNodes.size() != 0 ) {
    std::list<Arc::XMLNode> csURLNodes = jsdl_description_.XPathLookup("//jsdl-arc:CredentialServer//URL", jsdl_namespaces);
    credentialserver = (std::string) csURLNodes.front();
  }

  if( jsdl_resources_ ) {
    std::list<Arc::XMLNode> sltNodes = jsdl_resources_.XPathLookup("//jsdl-arc:SessionLifeTime", jsdl_namespaces);
    if( sltNodes.size() != 0 )
      lifetime = atoi( ((std::string) sltNodes.front()).c_str() );

    std::list<Arc::XMLNode> gtlNodes = jsdl_resources_.XPathLookup("//jsdl-arc:GridTimeLimit", jsdl_namespaces);
    if( gtlNodes.size() != 0 )
      grid_time = atoi( ((std::string) gtlNodes.front()).c_str() );

    std::list<Arc::XMLNode> ctNodes = jsdl_resources_.XPathLookup("//jsdl-arc:CandidateTarget", jsdl_namespaces);
    if( ctNodes.size() != 0 ) {
      std::list<Arc::XMLNode> hnctNodes = ctNodes.front().XPathLookup("//jsdl-arc:HostName", jsdl_namespaces);
      if( hnctNodes.size() != 0 ) {
        cluster= (std::string) hnctNodes.front();
      };
      std::list<Arc::XMLNode> qnctNodes = ctNodes.front().XPathLookup("//jsdl-arc:QueueName", jsdl_namespaces);
      if(  qnctNodes.size() != 0 ) {
        queue = (std::string) qnctNodes.front();
      };
    };

    std::list<Arc::XMLNode> cpuaNodes = jsdl_resources_.XPathLookup("//jsdl:CPUArchitecture", jsdl_namespaces);
    if( cpuaNodes.size() != 0 ) {
      architecture = (std::string) cpuaNodes.front();
    };

    std::list<Arc::XMLNode> rteNodes = jsdl_resources_.XPathLookup("//jsdl-arc:RunTimeEnvironment", jsdl_namespaces);
    for(std::list<Arc::XMLNode>::iterator i_rteNodes = rteNodes.begin(); i_rteNodes!=rteNodes.end();++i_rteNodes) {
      std::string s = (std::string) (*i_rteNodes).XPathLookup("//jsdl-arc:Name", jsdl_namespaces).front();

      Arc::XMLNode versionNode = (*i_rteNodes).XPathLookup("//jsdl-arc:Version",jsdl_namespaces).front();
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
          s+="="; s+=(std::string) exactNodes.front();
        };
      };
      runtime_environments.push_back(RuntimeEnvironment(s));
    };

    std::list<Arc::XMLNode> tcpucNodes = jsdl_resources_.XPathLookup("//jsdl:TotalCPUCount", jsdl_namespaces);
    if( tcpucNodes.size() != 0 ) {
      count=(int)(get_limit( tcpucNodes.front() )+0.5);
    } else {
      std::list<Arc::XMLNode> icpucNodes = jsdl_resources_.XPathLookup("//jsdl:IndividualCPUCount", jsdl_namespaces);
      if( icpucNodes.size() != 0 ) {
        count=(int)(get_limit( icpucNodes.front() )+0.5);
      }
    }

    std::list<Arc::XMLNode> tpmNodes = jsdl_resources_.XPathLookup("//jsdl:TotalPhysicalMemory", jsdl_namespaces);
    if( tpmNodes.size() != 0 ) {
      memory=(int)(get_limit( tpmNodes.front() )+0.5);
    } else {
      std::list<Arc::XMLNode> ipmNodes = jsdl_resources_.XPathLookup("//jsdl:IndividualPhysicalMemory", jsdl_namespaces);
      if( ipmNodes.size() != 0 ) {
        memory=(int)(get_limit( ipmNodes.front() )+0.5);
      }
    }

    std::list<Arc::XMLNode> tdsNodes = jsdl_resources_.XPathLookup("//jsdl:TotalDiskSpace", jsdl_namespaces);
    if( tdsNodes.size() != 0 ) {
      disk=(int)(get_limit( tdsNodes.front() )+0.5);
    } else {
      std::list<Arc::XMLNode> idsNodes = jsdl_resources_.XPathLookup("//jsdl:IndividualDiskSpace", jsdl_namespaces);
      if( idsNodes.size() != 0 ) {
        disk=(int)(get_limit( idsNodes.front() )+0.5);
      }
    }

    std::list<Arc::XMLNode> tcputNodes = jsdl_resources_.XPathLookup("//jsdl:TotalCPUTime", jsdl_namespaces);
    if( tcputNodes.size() != 0 ) {
      cpu_time=(int)(get_limit( tcputNodes.front() )+0.5);
    } else {
      std::list<Arc::XMLNode> icputNodes = jsdl_resources_.XPathLookup("//jsdl:IndividualCPUTime", jsdl_namespaces);
      if( icputNodes.size() != 0 ) {
        cpu_time=(int)(get_limit( icputNodes.front() )+0.5);
      }
    }

    std::list<Arc::XMLNode> mw = jsdl_resources_.XPathLookup("//jsdl-arc:Middleware", jsdl_namespaces);
    for(std::list<Arc::XMLNode>::iterator i_mw = mw.begin(); i_mw!=mw.end(); ++i_mw) {
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
      middlewares.push_back(RuntimeEnvironment(s));
    };
  };

  if(jsdl_posix_) {
    std::list<Arc::XMLNode> inputNodes = jsdl_posix_.XPathLookup("//jsdl-posix:Input", jsdl_namespaces);
    if( inputNodes.size() != 0 ) {
      sstdin = (std::string) inputNodes.front();
      strip_spaces(sstdin);
    };

    std::list<Arc::XMLNode> outputNodes = jsdl_posix_.XPathLookup("//jsdl-posix:Output", jsdl_namespaces);
    if( outputNodes.size() != 0 ) {
      sstdout = (std::string) outputNodes.front();
      strip_spaces(sstdout);
    };

    std::list<Arc::XMLNode> errorNodes = jsdl_posix_.XPathLookup("//jsdl-posix:Error", jsdl_namespaces);
    if( errorNodes.size() != 0 ) {
      sstderr = (std::string) errorNodes.front();
      strip_spaces(sstderr);
    };

    std::list<Arc::XMLNode> executableNodes = jsdl_posix_.XPathLookup("//jsdl-posix:Executable", jsdl_namespaces);
    if( executableNodes.size() != 0 ) {
      strip_spaces(*(arguments.insert(arguments.end(), (std::string) executableNodes.front() )));
    } else {
      arguments.insert(arguments.end(),"");
    };

    std::list<Arc::XMLNode> argument = jsdl_posix_.XPathLookup("//jsdl-posix:Argument", jsdl_namespaces);
    for(std::list<Arc::XMLNode>::iterator i = argument.begin();i!=argument.end();++i) {
      if(!(*i)) continue;
      strip_spaces(*(arguments.insert(arguments.end(),(std::string) (*i) )));
    };

    std::list<Arc::XMLNode> mlNodes = jsdl_posix_.XPathLookup("//jsdl-posix:MemoryLimit", jsdl_namespaces);
    if( mlNodes.size() != 0 ) {
      memory= atoi( ((std::string) mlNodes.front()).c_str() );
    };

    std::list<Arc::XMLNode> cputlNodes = jsdl_posix_.XPathLookup("//jsdl-posix:CPUTimeLimit", jsdl_namespaces);
    if( cputlNodes.size() != 0 ) {
      cpu_time = atoi( ((std::string) cputlNodes.front()).c_str() );
    };

    std::list<Arc::XMLNode> wtlNodes = jsdl_posix_.XPathLookup("//jsdl-posix:WallTimeLimit", jsdl_namespaces);
    if( wtlNodes.size() != 0 ) {
      wall_time = atoi( ((std::string) wtlNodes.front()).c_str() );
    };
  };
  {
    std::list<Arc::XMLNode> logs = jsdl_description_.XPathLookup("//jsdl-arc:RemoteLogging", jsdl_namespaces);
    std::list<Arc::XMLNode>::iterator i_logs = logs.begin();
    for(;i_logs!=logs.end();++i_logs) {
      std::list<Arc::XMLNode> urlNodes = (*i_logs).XPathLookup("//jsdl-arc:URL", jsdl_namespaces);
      loggers.push_back( (std::string) urlNodes.front() );
    }
  };
  {
    std::list<Arc::XMLNode> ds = jsdl_description_.XPathLookup("//jsdl:DataStaging", jsdl_namespaces);
    for(std::list<Arc::XMLNode>::iterator i_ds = ds.begin(); i_ds!=ds.end();++i_ds) {
      if(!(*i_ds)) continue;
      // FilesystemName - ignore
      // CreationFlag - ignore
      // DeleteOnTermination - ignore

      std::list<Arc::XMLNode> sourceNodes = (*i_ds).XPathLookup("//jsdl:Source", jsdl_namespaces);
      std::list<Arc::XMLNode> targetNodes = (*i_ds).XPathLookup("//jsdl:Target", jsdl_namespaces);
      std::list<Arc::XMLNode> filenameNodes = (*i_ds).XPathLookup("//jsdl:FileName", jsdl_namespaces);

      if( ( sourceNodes.size() == 0 ) && ( targetNodes.size() == 0 ) ) {
        // Neither in nor out - must be file generated by job
        outputdata.push_back(JobRequest::OutputFile( (std::string) filenameNodes.front() ,""));
      } else {
        if( sourceNodes.size() != 0 ) {
          std::list<Arc::XMLNode> isExecutableNodes = (*i_ds).XPathLookup("//jsdl:Source//jsdl-arc:IsExecutable", jsdl_namespaces);
          if( isExecutableNodes.size() != 0 && ( (std::string) isExecutableNodes.front() ) == "true" ) {
            executables.push_back( (std::string) filenameNodes.front() );
          };
          std::list<Arc::XMLNode> sourceURINodes = sourceNodes.front().XPathLookup("//jsdl:URI", jsdl_namespaces);
          if( sourceURINodes.size() == 0 ) {
            // Source without URL - uploaded by client
            std::list<Arc::XMLNode> fileParameterNodes = (*i_ds).XPathLookup("//jsdl-arc:FileParamters", jsdl_namespaces);
            if( fileParameterNodes.size() != 0 ) {
              inputdata.push_back(JobRequest::InputFile( (std::string) filenameNodes.front() , (std::string) fileParameterNodes.front() ) );
            } else {
              inputdata.push_back(JobRequest::InputFile( (std::string) filenameNodes.front() ,""));
            };
          } else {
            inputdata.push_back(JobRequest::InputFile( (std::string) filenameNodes.front() , (std::string) sourceURINodes.front() ) );
          };
        };
        if( targetNodes.size() != 0 ) {
          std::list<Arc::XMLNode> targetURINodes = sourceNodes.front().XPathLookup("//jsdl:URI", jsdl_namespaces);
          if( targetURINodes.size() == 0 ) {
            // Destination without URL - keep after execution
            // TODO: check for DeleteOnTermination
            outputdata.push_back(JobRequest::OutputFile( (std::string) filenameNodes.front() ,""));
          } else {
            outputdata.push_back(JobRequest::OutputFile( (std::string) filenameNodes.front() ,  (std::string) targetURINodes.front() ) );
          };
        };
      };
    };
  };

  {
  std::list<Arc::XMLNode> nts = jsdl_document.XPathLookup("//jsdl:JobDescription//jsdl-arc:Notify", jsdl_namespaces);
  for(std::list<Arc::XMLNode>::iterator i_nts = nts.begin();i_nts!=nts.end();++i_nts) {
    std::list<Arc::XMLNode> typeNodes = (*i_nts).XPathLookup("//jsdl-arc:Type", jsdl_namespaces);
    std::list<Arc::XMLNode> endpointNodes = (*i_nts).XPathLookup("//jsdl-arc:Endpoint", jsdl_namespaces);
    std::list<Arc::XMLNode> stateNodes = (*i_nts).XPathLookup("//jsdl-arc:State", jsdl_namespaces);
    if( (typeNodes.size() == 0 || ( (std::string) typeNodes.front() ) == "Email") && endpointNodes.size() != 0 && stateNodes.size() != 0) {
      std::string flags;
        for(std::list<Arc::XMLNode>::iterator i = stateNodes.begin(); i!=stateNodes.end(); ++i) {
          std::string value = (*i);
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
            notifications.push_back(Notification(flags, (std::string) endpointNodes.front() ));
        };
      };
    };
  };
    // Parse sub-jobs
  std::list<Arc::XMLNode> j_ = jsdl_description_.XPathLookup("jsdl:JobDescription", jsdl_namespaces);
  for(std::list<Arc::XMLNode>::iterator j = j_.begin(); j!=j_.end();++j) {
    if(!(*j)) continue;
    JobRequest* jr = new JobRequest;
    if(!(((JobRequestJSDL*)jr)->set(*j))) return false;
    alternatives.push_back(jr);
  };
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


JobRequest& JobRequestJSDL::operator=(const JobRequest& j) throw (JobRequestError) {
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
