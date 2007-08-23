#include <sstream>

#include "job_jsdl.h"

#include "jsdl/jsdl_soapStub.h"
#include "jsdl/jsdl_soapH.h"

#ifdef GSOAP_ALWAYS_USES_NAMESPACES_FOR_MEMBER_NAMES
#define JobDescription jsdl__JobDescription
#define Application jsdl__Application
#define JobIdentification jsdl__JobIdentification
#define JobName jsdl__JobName
#define DataStaging jsdl__DataStaging
#define Target jsdl__Target
#define Source jsdl__Source
#define FileName jsdl__FileName
#define FilesystemName jsdl__FilesystemName
#define URI jsdl__URI
#define Resources jsdl__Resources
#define IndividualCPUCount jsdl__IndividualCPUCount
#define TotalCPUCount jsdl__TotalCPUCount
#define IndividualCPUTime jsdl__IndividualCPUTime
#define TotalCPUTime jsdl__TotalCPUTime
#define IndividualPhysicalMemory jsdl__IndividualPhysicalMemory
#define TotalPhysicalMemory jsdl__TotalPhysicalMemory
#define CPUArchitecture jsdl__CPUArchitecture
#define CPUArchitectureName jsdl__CPUArchitectureName
#define TotalDiskSpace jsdl__TotalDiskSpace
#define IndividualDiskSpace jsdl__IndividualDiskSpace

#define Executable jsdlPOSIX__Executable
#define Argument jsdlPOSIX__Argument
#define CPUTimeLimit jsdlPOSIX__CPUTimeLimit
#define WallTimeLimit jsdlPOSIX__WallTimeLimit
#define MemoryLimit jsdlPOSIX__MemoryLimit
#define Input jsdlPOSIX__Input
#define Output jsdlPOSIX__Output
#define Error jsdlPOSIX__Error
#endif

struct Namespace jsdl_namespaces[] = {
    {"SOAP-ENV", "http://schemas.xmlsoap.org/soap/envelope/", "http://www.w3.org/*/soap-envelope", NULL},
    {"SOAP-ENC", "http://schemas.xmlsoap.org/soap/encoding/", "http://www.w3.org/*/soap-encoding", NULL},
    {"xsi", "http://www.w3.org/2001/XMLSchema-instance", "http://www.w3.org/*/XMLSchema-instance", NULL},
    {"xsd", "http://www.w3.org/2001/XMLSchema", "http://www.w3.org/*/XMLSchema", NULL},
    {"jsdl", "http://schemas.ggf.org/jsdl/2005/11/jsdl", NULL, NULL },
    {"jsdlPOSIX", "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix", NULL, NULL },
    {"jsdlARC", "http://www.nordugrid.org/ws/schemas/jsdl-arc", NULL, NULL },
    {NULL, NULL, NULL, NULL}
};

static void strip_spaces(std::string& s) {
  int n;
  for(n=0;s[n];n++) if(!isspace(s[n])) break;
  if(n) s.erase(0,n);
  for(n=s.length()-1;n>=0;n--) if(!isspace(s[n])) break;
  s.resize(n+1);
}

static double get_limit(jsdl__RangeValue_USCOREType* range) {
  if(range == NULL) return 0.0;
  if(range->UpperBoundedRange) return range->UpperBoundedRange->__item;
  if(range->LowerBoundedRange) return range->LowerBoundedRange->__item;
  return 0.0;
}

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

JobRequestJSDL::~JobRequestJSDL(void) {
  if(sp_) {
    soap_destroy(sp_);
    soap_end(sp_);
    soap_done(sp_);
	delete sp_;
  };
  //if(job_) delete job_;
}


bool JobRequestJSDL::set(std::istream& s) throw(JobRequestError) {
	reset();
	sp_=new soap;
	if(!sp_) return false;
	soap_init(sp_);
	sp_->namespaces=jsdl_namespaces;
	soap_begin(sp_);
	sp_->is=&s;
	job_=soap_new_jsdl__JobDefinition_USCOREType(sp_,-1);
	if(!job_) {
		soap_destroy(sp_); soap_end(sp_); soap_done(sp_);
		delete sp_; sp_=NULL; return false;
	};
	job_->soap_default(sp_);
	if(soap_begin_recv(sp_) != SOAP_OK) {
		soap_destroy(sp_); soap_end(sp_); soap_done(sp_);
		delete sp_; sp_=NULL; return false;
	};
	if(!job_->soap_get(sp_,"jsdl:JobDefinition",NULL)) {
		soap_end_recv(sp_);
		soap_destroy(sp_); soap_end(sp_); soap_done(sp_);
		delete sp_; sp_=NULL; return false;
	};
	soap_end_recv(sp_);
	if(!(job_->JobDescription)) {
		soap_destroy(sp_); soap_end(sp_); soap_done(sp_);
		delete sp_; sp_=NULL; return false;
	};
	if(!set(job_->JobDescription)) {
		reset();
		soap_destroy(sp_); soap_end(sp_); soap_done(sp_);
		delete sp_; sp_=NULL; return false;
	};
	return true;
}
// This method is needed for filling attributes in sub-jobs
bool JobRequestJSDL::set(jsdl__JobDescription_USCOREType* job_description_) throw(JobRequestError) {
	// Store fields in parsed stucture
	jsdlPOSIX__POSIXApplication_USCOREType* job_posix_ = NULL;
	if(job_description_->Application &&
	   job_description_->Application->jsdlPOSIX__POSIXApplication) {
		job_posix_=
		      job_description_->Application->jsdlPOSIX__POSIXApplication;
	};
	jsdl__Resources_USCOREType* job_resources_ = job_description_->Resources;

    if(job_description_->JobIdentification) {
      if(job_description_->JobIdentification->JobName) {
        job_name=*(job_description_->JobIdentification->JobName);
      };
    };
	if(job_description_->jsdlARC__AccessControl) {
		if((!(job_description_->jsdlARC__AccessControl->Type)) ||
		   (*(job_description_->jsdlARC__AccessControl->Type) == 
		    jsdlARC__AccessControlType_USCOREType__GACL)) {
			if(job_description_->jsdlARC__AccessControl->Content)
				acl=*(job_description_->jsdlARC__AccessControl->Content);
		};
	};
	if(job_description_->jsdlARC__ProcessingStartTime) {
		start_time=*(job_description_->jsdlARC__ProcessingStartTime);
	};
	if(job_description_->jsdlARC__Reruns) 
		reruns=*(job_description_->jsdlARC__Reruns);
	if(job_description_->jsdlARC__LocalLogging)
	   gmlog=job_description_->jsdlARC__LocalLogging->Directory;
	if(job_description_->jsdlARC__CredentialServer)
	   credentialserver=job_description_->jsdlARC__CredentialServer->URL;

	if(job_resources_) { 
		if(job_resources_->jsdlARC__SessionLifeTime)
			lifetime=*(job_resources_->jsdlARC__SessionLifeTime);
		if(job_resources_->jsdlARC__GridTimeLimit)
			grid_time=*(job_resources_->jsdlARC__GridTimeLimit);
		if(job_resources_->jsdlARC__CandidateTarget) {
			if(job_resources_->jsdlARC__CandidateTarget->HostName) {
				cluster=*(job_resources_->jsdlARC__CandidateTarget->HostName);
			};
			if(job_resources_->jsdlARC__CandidateTarget->QueueName) {
				queue=*(job_resources_->jsdlARC__CandidateTarget->QueueName);
			};
		};
		if(job_resources_->CPUArchitecture) {
			const char* s = soap_jsdl__ProcessorArchitectureEnumeration2s(sp_,job_resources_->CPUArchitecture->CPUArchitectureName);
			if(s) architecture=s;
		};
		std::vector<jsdlARC__RunTimeEnvironment_USCOREType*>& rte =
		                     job_resources_->jsdlARC__RunTimeEnvironment;
		std::vector<jsdlARC__RunTimeEnvironment_USCOREType*>::iterator i_rte =
		                     rte.begin();
		for(;i_rte!=rte.end();++i_rte) {
			std::string s = (*i_rte)->Name;
			if((*i_rte)->Version) {
				// not supported
				if((*i_rte)->Version->UpperExclusive) continue;
				if((*i_rte)->Version->LowerExclusive) continue;
				if(((*i_rte)->Version->Exclusive) &&
				   (!*((*i_rte)->Version->Exclusive))) continue;
				if((*i_rte)->Version->Exact.size() > 1) continue;
				// supported
				if((*i_rte)->Version->Exact.size()) {
					s+="="; s+=(*i_rte)->Version->Exact[0];
				};
			};
			runtime_environments.push_back(RuntimeEnvironment(s));
		};
		if(job_resources_->TotalCPUCount) {
			count=(int)(get_limit(job_resources_->TotalCPUCount)+0.5);
		} else if(job_resources_->IndividualCPUCount) {
			count=(int)(get_limit(job_resources_->IndividualCPUCount)+0.5);
		};
		if(job_resources_->TotalPhysicalMemory) {
			memory=(int)(get_limit(job_resources_->TotalPhysicalMemory)+0.5);
		} else if(job_resources_->IndividualPhysicalMemory) {
			memory=
			  (int)(get_limit(job_resources_->IndividualPhysicalMemory)+0.5);
		};
		if(job_resources_->TotalDiskSpace) {
			disk=(int)(get_limit(job_resources_->TotalDiskSpace)+0.5);
		} else if(job_resources_->IndividualDiskSpace) {
			disk=
			  (int)(get_limit(job_resources_->IndividualDiskSpace)+0.5);
		};
		if(job_resources_->TotalCPUTime) {
			cpu_time=(int)(get_limit(job_resources_->TotalCPUTime)+0.5);
		} else if(job_resources_->IndividualCPUTime) {
			cpu_time=(int)(get_limit(job_resources_->IndividualCPUTime)+0.5);
		};
		std::vector<jsdlARC__Middleware_USCOREType*>& mw =
		                        job_resources_->jsdlARC__Middleware;
		std::vector<jsdlARC__Middleware_USCOREType*>::iterator i_mw =
		                        mw.begin();
		for(;i_mw!=mw.end();++i_mw) {
			std::string s = (*i_mw)->Name;
			if((*i_mw)->Version) {
				// not supported
				if((*i_mw)->Version->UpperExclusive) continue;
				if((*i_mw)->Version->LowerExclusive) continue;
				if(((*i_mw)->Version->Exclusive) &&
				   (!*((*i_mw)->Version->Exclusive))) continue;
				if((*i_mw)->Version->Exact.size() > 1) continue;
				// supported
				if((*i_mw)->Version->Exact.size()) {
					s+="="; s+=(*i_mw)->Version->Exact[0];
				};
			};
			middlewares.push_back(RuntimeEnvironment(s));
		};
	};

	if(job_posix_) {
		if(job_posix_->Input) {
    		sstdin=job_posix_->Input->__item;
			strip_spaces(sstdin);
		};
		if(job_posix_->Output) {
    		sstdout=job_posix_->Output->__item;
			strip_spaces(sstdout);
		};
		if(job_posix_->Error) {
    		sstderr=job_posix_->Error->__item;
			strip_spaces(sstderr);
		};
	    //std::list<std::string> arguments;
		//if(job_posix_->Executable == NULL) {
    	//	odlog(0)<<"job description is missing executable"<<std::endl;
		//	return false;
		//};
		if(job_posix_->Executable) {
			strip_spaces(*(arguments.insert(
			       arguments.end(),job_posix_->Executable->__item)));
		} else {
			arguments.insert(arguments.end(),"");
		};
		for(std::vector<jsdlPOSIX__Argument_USCOREType*>::iterator i = 
	 	     job_posix_->Argument.begin();i!=job_posix_->Argument.end();++i) {
		    if(!(*i)) continue;
			strip_spaces(*(arguments.insert(arguments.end(),(*i)->__item)));
		};
		if(job_posix_->MemoryLimit) {
			memory=job_posix_->MemoryLimit->__item;
		};
		if(job_posix_->CPUTimeLimit) {
			cpu_time=job_posix_->CPUTimeLimit->__item;
		};
		if(job_posix_->WallTimeLimit) {
			wall_time=job_posix_->WallTimeLimit->__item;
		};
	};
	{
		std::vector<jsdlARC__RemoteLogging_USCOREType*>& logs =
		                     job_description_->jsdlARC__RemoteLogging;
		std::vector<jsdlARC__RemoteLogging_USCOREType*>::iterator i_logs = 
		                     logs.begin();
		for(;i_logs!=logs.end();++i_logs) loggers.push_back((*i_logs)->URL);
	};
	{
		std::vector<jsdl__DataStaging_USCOREType*>* ds; 
		std::vector<jsdl__DataStaging_USCOREType*>::iterator i_ds;
		ds=&(job_description_->DataStaging);
		for(i_ds=ds->begin();i_ds!=ds->end();++i_ds) {
			if(!(*i_ds)) continue;
			// FilesystemName - ignore
			// CreationFlag - ignore
			// DeleteOnTermination - ignore
			if(((*i_ds)->Source == NULL) && ((*i_ds)->Target == NULL)) {
				// Neither in nor out - must be file generated by job
				outputdata.push_back(
				          JobRequest::OutputFile((*i_ds)->FileName,""));
			} else {
				if((*i_ds)->Source != NULL) {
					if(((*i_ds)->jsdlARC__IsExecutable) &&
					   (*((*i_ds)->jsdlARC__IsExecutable))) {
						executables.push_back((*i_ds)->FileName);
					};
					if((*i_ds)->Source->URI == NULL) {
						// Source without URL - uploaded by client
						if((*i_ds)->jsdlARC__FileParameters) {
							inputdata.push_back(
							    JobRequest::InputFile((*i_ds)->FileName,
							         *((*i_ds)->jsdlARC__FileParameters)));
						} else {
							inputdata.push_back(
							    JobRequest::InputFile((*i_ds)->FileName,""));
						};
					} else {
						inputdata.push_back(
						        JobRequest::InputFile((*i_ds)->FileName,
						                              *((*i_ds)->Source->URI)));
					};
				};
				if((*i_ds)->Target != NULL) {
					if((*i_ds)->Target->URI == NULL) {
						// Destination without URL - keep after execution
						// TODO: check for DeleteOnTermination
						outputdata.push_back(
						        JobRequest::OutputFile((*i_ds)->FileName,""));
					} else {
						outputdata.push_back(
						        JobRequest::OutputFile((*i_ds)->FileName,
						                              *((*i_ds)->Target->URI)));
					};
				};
			};
		};
	};
	{
		std::vector<jsdlARC__Notify_USCOREType*>& nts =
		                          job_description_->jsdlARC__Notify;
		std::vector<jsdlARC__Notify_USCOREType*>::iterator i_nts = nts.begin();
		for(;i_nts!=nts.end();++i_nts) {
			if((!((*i_nts)->Type)) ||
			   (*((*i_nts)->Type) ==
			    jsdlARC__NotificationType_USCOREType__Email)
			) {
				if((*i_nts)->Endpoint) {
					if((*i_nts)->State.size()) {
						std::string flags;
						for(std::vector<enum jsdlARC__GMState_USCOREType>::iterator i = (*i_nts)->State.begin();i!=(*i_nts)->State.end();++i) {
							switch(*i) {
								case jsdlARC__GMState_USCOREType__PREPARING:
									flags+="b"; break;
								case jsdlARC__GMState_USCOREType__INLRMS:
									flags+="q"; break;
								case jsdlARC__GMState_USCOREType__FINISHING:
									flags+="f"; break;
								case jsdlARC__GMState_USCOREType__FINISHED:
									flags+="e"; break;
								case jsdlARC__GMState_USCOREType__DELETED:
									flags+="d"; break;
								case jsdlARC__GMState_USCOREType__CANCELING:
									flags+="c"; break;
								default: break;
							};
						};
						if(flags.length()) {
							notifications.push_back(Notification(flags,
							                     *((*i_nts)->Endpoint)));
						};
					};
				};
			};
		};
	};
    // Parse sub-jobs
	for(std::vector<jsdl__JobDescription_USCOREType*>::iterator j =
	          job_description_->JobDescription.begin();
	          j!=job_description_->JobDescription.end();++j) {
		if(!(*j)) continue;
		JobRequest* jr = new JobRequest;
		if(!(((JobRequestJSDL*)jr)->set(*j))) return false;
		alternatives.push_back(jr);
	};
	return true;
}

bool JobRequestJSDL::print(std::string& s) throw(JobRequestError) {
	if(!sp_) return false;
	if(!job_) return false;
	soap_set_omode(sp_,SOAP_XML_GRAPH | SOAP_XML_CANONICAL);
	job_->soap_serialize(sp_);
	std::ostringstream f;
    sp_->os=&f;
    soap_begin_send(sp_);
    job_->soap_put(sp_,"jsdl:JobDefinition",NULL);
    soap_end_send(sp_);
	s=f.str();
	return true;
}

JobRequestJSDL::JobRequestJSDL(const JobRequest& j) throw(JobRequestError) {
	sp_=NULL;
	(*this)=j;
}

JobRequest& JobRequestJSDL::operator=(const JobRequest& j) throw (JobRequestError) {
	if(sp_) {
		soap_destroy(sp_); soap_end(sp_); soap_done(sp_); delete sp_;
	};
	JobRequest::operator=(j);
    sp_=new soap;
    if(!sp_) return *this;
    soap_init(sp_);
	sp_->namespaces=jsdl_namespaces;
    soap_begin(sp_);
    job_=soap_new_jsdl__JobDefinition_USCOREType(sp_,-1);
    if(!job_) goto error;
	job_->soap_default(sp_);
	job_->JobDescription=soap_new_jsdl__JobDescription_USCOREType(sp_,-1);
	if(!(job_->JobDescription)) goto error;
	job_->JobDescription->soap_default(sp_);
    if(!set_jsdl(job_->JobDescription,sp_)) goto error;
	return *this;
error:
    if(sp_) {
        soap_destroy(sp_); soap_end(sp_); soap_done(sp_);
		delete sp_; sp_=NULL;
    };
	return *this;
}

bool JobRequestJSDL::set_jsdl(jsdl__JobDescription_USCOREType* job_description_,struct soap* sp_) {
	// Creating necessary items
    jsdlPOSIX__POSIXApplication_USCOREType* job_posix_;
    jsdl__Resources_USCOREType* job_resources_;
	job_description_->Application=soap_new_jsdl__Application_USCOREType(sp_,-1);
	if(!(job_description_->Application)) goto error;
	job_description_->Application->soap_default(sp_);

    job_posix_=soap_new_jsdlPOSIX__POSIXApplication_USCOREType(sp_,-1);
	if(!job_posix_) goto error;
	job_posix_->soap_default(sp_);
	job_description_->Application->jsdlPOSIX__POSIXApplication=job_posix_;

    job_resources_ = soap_new_jsdl__Resources_USCOREType(sp_,-1);
	if(!job_resources_) goto error;
	job_resources_->soap_default(sp_);
	job_description_->Resources=job_resources_;

	// Making items on demand
	if(job_name.length()) {
		job_description_->JobIdentification=soap_new_jsdl__JobIdentification_USCOREType(sp_,-1);
		if(!(job_description_->JobIdentification)) goto error;
		job_description_->JobIdentification->soap_default(sp_);
		job_description_->JobIdentification->JobName=soap_new_std__string(sp_,-1);
		if(!(job_description_->JobIdentification->JobName)) goto error;
		//job_description_->JobIdentification->JobName->soap_default(sp_);
		job_description_->JobIdentification->JobName=soap_new_std__string(sp_,-1);
		if(!(job_description_->JobIdentification->JobName)) goto error;
		*(job_description_->JobIdentification->JobName)=job_name;
	};
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
		for(std::list<JobRequest::InputFile>::iterator i = inputdata.begin();
		                                              i!=inputdata.end();++i) {
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
        for(std::list<JobRequest::OutputFile>::iterator i = outputdata.begin();
                                                     i!=outputdata.end();++i) {
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
        std::vector<jsdlARC__RemoteLogging_USCOREType*>& logs =
                             job_description_->jsdlARC__RemoteLogging;
		for(std::list<std::string>::iterator i = loggers.begin();
		                                       i!=loggers.end();++i) {
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
		for(std::list<RuntimeEnvironment>::iterator i =
		     runtime_environments.begin();i!=runtime_environments.end();++i) {
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
        std::vector<jsdlARC__Middleware_USCOREType*>& mws =
                               job_resources_->jsdlARC__Middleware;
		for(std::list<RuntimeEnvironment>::iterator i = middlewares.begin();
		                                       i!=middlewares.end();++i) {
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
		std::vector<jsdlARC__Notify_USCOREType*>& nts =
		                          job_description_->jsdlARC__Notify;


		for(std::list<Notification>::iterator i = notifications.begin();
		                                i!=notifications.end();++i) {
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
						nt->State.push_back(
						     jsdlARC__GMState_USCOREType__PREPARING);
						break;
					case 'q':
						nt->State.push_back(
						     jsdlARC__GMState_USCOREType__INLRMS);
						break;
					case 'f':
						nt->State.push_back(
						     jsdlARC__GMState_USCOREType__FINISHING);
						break;
					case 'e':
						nt->State.push_back(
						     jsdlARC__GMState_USCOREType__FINISHED);
						break;
					case 'd':
						nt->State.push_back(
						     jsdlARC__GMState_USCOREType__DELETED);
						break;
					case 'c':
						nt->State.push_back(
						     jsdlARC__GMState_USCOREType__CANCELING);
						break;
					default: break;
				};
			};
			nts.push_back(nt);
		};
	};
	for(std::list<JobRequest*>::const_iterator i = alternatives.begin();
	                                          i!=alternatives.end();++i) {
		if(!(*i)) continue;
		jsdl__JobDescription_USCOREType* job_description__ = soap_new_jsdl__JobDescription_USCOREType(sp_,-1);
		if(!(job_description__)) goto error;
		job_description__->soap_default(sp_);
		if(!((JobRequestJSDL*)(*i))->set_jsdl(job_description__,sp_)) goto error;
		job_description_->JobDescription.push_back(job_description__);
	};
/*
    executables.clear();
*/
	return true;
error:
    return false;
}

