#ifdef SWIGPYTHON
%module compute

%include "Arc.i"

%import "../src/hed/libs/common/ArcConfig.h"
%import "../src/hed/libs/common/URL.h"
%import "../src/hed/libs/common/XMLNode.h"
%import "../src/hed/libs/common/DateTime.h"
%import "../src/hed/libs/message/PayloadSOAP.h"
%import "../src/hed/libs/message/PayloadRaw.h"
%import "../src/hed/libs/message/PayloadStream.h"
%import "../src/hed/libs/message/MCC_Status.h"
%import "../src/hed/libs/message/Message.h"

%ignore Arc::AutoPointer::operator!;
%warnfilter(SWIGWARN_PARSE_NAMED_NESTED_CLASS) Arc::CountedPointer::Base;
%ignore Arc::CountedPointer::operator!;
%ignore Arc::CountedPointer::operator=(T*);
%ignore Arc::CountedPointer::operator=(const CountedPointer<T>&);
// Ignoring functions from Utils.h since swig thinks they are methods of the CountedPointer class, and thus compilation fails.
%ignore Arc::GetEnv;
%ignore Arc::SetEnv;
%ignore Arc::UnsetEnv;
%ignore Arc::EnvLockAcquire;
%ignore Arc::EnvLockRelease;
%ignore Arc::EnvLockWrap;
%ignore Arc::EnvLockUnwrap;
%ignore Arc::EnvLockUnwrapComplete;
%ignore Arc::EnvLockWrapper;
%ignore Arc::InterruptGuard;
%ignore Arc::StrError;
%ignore PersistentLibraryInit;
/* Swig tries to create functions which return a new CountedPointer object.
 * Those functions takes no arguments, and since there is no default
 * constructor for the CountedPointer compilation fails.
 * Adding a "constructor" (used as a function in the cpp file) which
 * returns a new CountedPointer object with newed T object created
 * Ts default constructor. Thus if T has no default constructor,
 * another work around is needed in order to map that CountedPointer
 * wrapped class with swig.
 */
%extend Arc::CountedPointer {
  CountedPointer() { return new Arc::CountedPointer<T>(new T());}
}
%{
#include <arc/Utils.h>
%}
%include "../src/hed/libs/common/Utils.h"
#endif


// Wrap contents of $(top_srcdir)/src/hed/libs/compute/Software.h
%{
#include <arc/compute/Software.h>
%}
%ignore Arc::SoftwareRequirement::operator=(const SoftwareRequirement&);
%ignore Arc::Software::convert(const ComparisonOperatorEnum& co);
%ignore Arc::Software::toString(ComparisonOperator co);
%ignore operator<<(std::ostream&, const Software&);
%ignore Arc::SoftwareRequirement::SoftwareRequirement(const Software& sw, Software::ComparisonOperator swComOp);
%ignore Arc::SoftwareRequirement::add(const Software& sw, Software::ComparisonOperator swComOp);
%wraplist(Software, Arc::Software);
%wraplist(SoftwareRequirement, Arc::SoftwareRequirement);
#ifdef SWIGJAVA
%ignore Arc::Software::operator();
%ignore Arc::SoftwareRequirement::getComparisonOperatorList() const;
#endif
%include "../src/hed/libs/compute/Software.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/compute/Endpoint.h
%{
#include <arc/compute/Endpoint.h>
%}
%ignore Arc::Endpoint::operator=(const ConfigEndpoint&);
%wraplist(Endpoint, Arc::Endpoint);
%include "../src/hed/libs/compute/Endpoint.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/compute/JobState.h
%{
#include <arc/compute/JobState.h>
%}
%ignore Arc::JobState::operator!;
%ignore Arc::JobState::operator=(const JobState&);
%rename(GetType) Arc::JobState::operator StateType; // works with swig 1.3.40, and higher...
%rename(GetType) Arc::JobState::operator Arc::JobState::StateType; // works with swig 1.3.29
%rename(GetNativeState) Arc::JobState::operator();
%wraplist(JobState, Arc::JobState);
%include "../src/hed/libs/compute/JobState.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/compute/Job.h
%{
#include <arc/compute/Job.h>
%}
%ignore Arc::Job::operator=(XMLNode);
%ignore Arc::Job::operator=(const Job&);
%wraplist(Job, Arc::Job);
%wraplist(JobP, Arc::Job*);
%wraplist(JobConstP, Arc::Job const *);
#ifdef SWIGPYTHON
%ignore Arc::Job::WriteJobIDsToFile(const std::list<Job>&, const std::string&, unsigned = 10, unsigned = 500000); // Clash. It is sufficient to wrap only WriteJobIDsToFile(cosnt std::list<URL>&, ...);
#endif
%include "../src/hed/libs/compute/Job.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/compute/JobControllerPlugin.h
%{
#include <arc/compute/JobControllerPlugin.h>
%}
%ignore Arc::JobControllerPluginPluginArgument::operator const UserConfig&; // works with swig 1.3.40, and higher...
%ignore Arc::JobControllerPluginPluginArgument::operator const Arc::UserConfig&; // works with swig 1.3.29
%wraplist(JobControllerPlugin, Arc::JobControllerPlugin *);
%template(JobControllerPluginMap) std::map<std::string, Arc::JobControllerPlugin *>;
%include "../src/hed/libs/compute/JobControllerPlugin.h"



// Wrap contents of $(top_srcdir)/src/hed/libs/compute/EndpointQueryingStatus.h
%{
#include <arc/compute/EndpointQueryingStatus.h>
%}
%ignore Arc::EndpointQueryingStatus::operator!;
%ignore Arc::EndpointQueryingStatus::operator=(EndpointQueryingStatusType);
%ignore Arc::EndpointQueryingStatus::operator=(const EndpointQueryingStatus&);
%include "../src/hed/libs/compute/EndpointQueryingStatus.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/compute/JobDescription.h
%{
#include <arc/compute/JobDescription.h>
%}
%ignore Arc::JobDescriptionResult::operator!;
%ignore Arc::Range<int>::operator int;
%ignore Arc::OptIn::operator=(const OptIn<T>&);
%ignore Arc::OptIn::operator=(const T&);
%ignore Arc::Range::operator=(const Range<T>&);
%ignore Arc::Range::operator=(const T&);
%ignore Arc::SourceType::operator=(const URL&);
%ignore Arc::SourceType::operator=(const std::string&);
%ignore Arc::JobDescription::operator=(const JobDescription&);
#ifdef SWIGPYTHON
%apply std::string& TUPLEOUTPUTSTRING { std::string& product }; /* Applies to:
 * JobDescriptionResult JobDescription::UnParse(std::string& product, std::string language, const std::string& dialect = "") const;
 */
#endif
#ifdef SWIGJAVA
%ignore Arc::JobDescription::GetAlternatives() const;
#endif
%include "../src/hed/libs/compute/JobDescription.h"
%wraplist(JobDescription, Arc::JobDescription);
%wraplist(StringPair, std::pair<std::string, std::string>);
%wraplist(ExecutableType, Arc::ExecutableType);
%wraplist(RemoteLoggingType, Arc::RemoteLoggingType);
%wraplist(NotificationType, Arc::NotificationType);
%wraplist(InputFileType, Arc::InputFileType);
%wraplist(OutputFileType, Arc::OutputFileType);
%wraplist(SourceType, Arc::SourceType);
%wraplist(TargetType, Arc::TargetType);
%template(ScalableTimeInt) Arc::ScalableTime<int>;
%template(RangeInt) Arc::Range<int>;
%template(StringOptIn) Arc::OptIn<std::string>;
#ifdef SWIGPYTHON
%clear std::string& product;
#endif


// Wrap contents of $(top_srcdir)/src/hed/libs/compute/ExecutionTarget.h
%{
#include <arc/compute/ExecutionTarget.h>
%}
%ignore Arc::ApplicationEnvironment::operator=(const Software&);
%ignore Arc::ExecutionTarget::operator=(const ExecutionTarget&);
%ignore Arc::GLUE2Entity<Arc::LocationAttributes>::operator->() const;
%ignore Arc::GLUE2Entity<Arc::AdminDomainAttributes>::operator->() const;
%ignore Arc::GLUE2Entity<Arc::ExecutionEnvironmentAttributes>::operator->() const;
%ignore Arc::GLUE2Entity<Arc::ComputingManagerAttributes>::operator->() const;
%ignore Arc::GLUE2Entity<Arc::ComputingShareAttributes>::operator->() const;
%ignore Arc::GLUE2Entity<Arc::ComputingEndpointAttributes>::operator->() const;
%ignore Arc::GLUE2Entity<Arc::ComputingServiceAttributes>::operator->() const;
#ifdef SWIGPYTHON
/* When instantiating a template of the form
 * Arc::CountedPointer< T<S> > two __nonzero__
 * python methods are created in the generated python module file which
 * causes an swig error. The two __nonzero__ methods probably stems from
 * the "operator bool" and "operator ->" methods. At least in the Arc.i
 * file the "operator bool" method is renamed to "__nonzero__". In
 * order to avoid that name clash the following "operator bool" methods 
 * are ignored.
 */
%ignore Arc::CountedPointer< std::map<std::string, double> >::operator bool;
%ignore Arc::CountedPointer< std::list<Arc::ApplicationEnvironment> >::operator bool;
#endif // SWIGPYTHON
#ifdef SWIGJAVA
%ignore Arc::GLUE2Entity<Arc::LocationAttributes>::operator*() const;
%ignore Arc::GLUE2Entity<Arc::AdminDomainAttributes>::operator*() const;
%ignore Arc::GLUE2Entity<Arc::ExecutionEnvironmentAttributes>::operator*() const;
%ignore Arc::GLUE2Entity<Arc::ComputingManagerAttributes>::operator*() const;
%ignore Arc::GLUE2Entity<Arc::ComputingShareAttributes>::operator*() const;
%ignore Arc::GLUE2Entity<Arc::ComputingEndpointAttributes>::operator*() const;
%ignore Arc::GLUE2Entity<Arc::ComputingServiceAttributes>::operator*() const;
#endif
%include "../src/hed/libs/compute/GLUE2Entity.h" // Contains declaration of the GLUE2Entity template, used in ExecutionTarget.h file.
%wraplist(ApplicationEnvironment, Arc::ApplicationEnvironment);
%wraplist(ExecutionTarget, Arc::ExecutionTarget);
%wraplist(ComputingService, Arc::ComputingServiceType);
%wraplist(SharedComputingEndpointAttributes, Arc::CountedPointer<Arc::ComputingEndpointAttributes>);
%template(ComputingEndpointMap) std::map<int, Arc::ComputingEndpointType>;
%template(ComputingShareMap) std::map<int, Arc::ComputingShareType>;
%template(ComputingManagerMap) std::map<int, Arc::ComputingManagerType>;
%template(ExecutionEnvironmentMap) std::map<int, Arc::ExecutionEnvironmentType>;
%template(SharedBenchmarkMap) Arc::CountedPointer< std::map<std::string, double> >;
%template(SharedApplicationEnvironmentList) Arc::CountedPointer< std::list<Arc::ApplicationEnvironment> >;
%template(GLUE2EntityLocationAttributes) Arc::GLUE2Entity<Arc::LocationAttributes>;
%template(CPLocationAttributes) Arc::CountedPointer<Arc::LocationAttributes>;
%template(GLUE2EntityAdminDomainAttributes) Arc::GLUE2Entity<Arc::AdminDomainAttributes>;
%template(CPAdminDomainAttributes) Arc::CountedPointer<Arc::AdminDomainAttributes>;
%template(GLUE2EntityExecutionEnvironmentAttributes) Arc::GLUE2Entity<Arc::ExecutionEnvironmentAttributes>;
%template(CPExecutionEnvironmentAttributes) Arc::CountedPointer<Arc::ExecutionEnvironmentAttributes>;
%template(GLUE2EntityComputingManagerAttributes) Arc::GLUE2Entity<Arc::ComputingManagerAttributes>;
%template(CPComputingManagerAttributes) Arc::CountedPointer<Arc::ComputingManagerAttributes>;
%template(GLUE2EntityComputingShareAttributes) Arc::GLUE2Entity<Arc::ComputingShareAttributes>;
%template(CPComputingShareAttributes) Arc::CountedPointer<Arc::ComputingShareAttributes>;
%template(GLUE2EntityComputingEndpointAttributes) Arc::GLUE2Entity<Arc::ComputingEndpointAttributes>;
%template(CPComputingEndpointAttributes) Arc::CountedPointer<Arc::ComputingEndpointAttributes>;
%template(GLUE2EntityComputingServiceAttributes) Arc::GLUE2Entity<Arc::ComputingServiceAttributes>;
%template(CPComputingServiceAttributes) Arc::CountedPointer<Arc::ComputingServiceAttributes>;
%include "../src/hed/libs/compute/ExecutionTarget.h"
%extend Arc::ComputingServiceType {
  %template(GetExecutionTargetsFromList) GetExecutionTargets< std::list<Arc::ExecutionTarget> >;
};


// Wrap contents of $(top_srcdir)/src/hed/libs/compute/EntityRetriever.h
%{
#include <arc/compute/EntityRetriever.h>
%}
#ifdef SWIGJAVA
%rename(_wait) Arc::EntityRetriever::wait;
#endif
%include "../src/hed/libs/compute/EntityRetriever.h"
%template() Arc::EntityConsumer<Arc::Endpoint>;
%template(EndpointContainer) Arc::EntityContainer<Arc::Endpoint>;
%template(ServiceEndpointQueryOptions) Arc::EndpointQueryOptions<Arc::Endpoint>;
%template(ServiceEndpointRetriever) Arc::EntityRetriever<Arc::Endpoint>;
%template() Arc::EntityConsumer<Arc::ComputingServiceType>;
%template(ComputingServiceContainer) Arc::EntityContainer<Arc::ComputingServiceType>;
%template(ComputingServiceQueryOptions) Arc::EndpointQueryOptions<Arc::ComputingServiceType>;
%template(TargetInformationRetriever) Arc::EntityRetriever<Arc::ComputingServiceType>;
%template() Arc::EntityConsumer<Arc::Job>;
%template(JobContainer) Arc::EntityContainer<Arc::Job>;
%template(JobListQueryOptions) Arc::EndpointQueryOptions<Arc::Job>;
%template(JobListRetriever) Arc::EntityRetriever<Arc::Job>;


// Wrap contents of $(top_srcdir)/src/hed/libs/compute/SubmitterPlugin.h
%{
#include <arc/compute/SubmitterPlugin.h>
%}
%ignore Arc::SubmitterPluginArgument::operator const UserConfig&; // works with swig 1.3.40, and higher...
%ignore Arc::SubmitterPluginArgument::operator const Arc::UserConfig&; // works with swig 1.3.29
%wraplist(SubmitterPlugin, Arc::SubmitterPlugin*);
%include "../src/hed/libs/compute/SubmitterPlugin.h"

// Wrap contents of $(top_srcdir)/src/hed/libs/compute/Submitter.h
%{
#include <arc/compute/Submitter.h>
%}
#ifdef SWIGPYTHON
#if SWIG_VERSION <= 0x010329
/* Swig version 1.3.29 cannot handle mapping a template of a "const *" type, so
 * adding a traits_from::from method taking a "const *" as taken from
 * swig-1.3.31 makes it possible to handle such types.
 */
%{
namespace swig {
template <class Type> struct traits_from<const Type *> {
  static PyObject *from(const Type* val) {
    return traits_from_ptr<Type>::from(const_cast<Type*>(val), 0);
  }
};
}
%}
#endif
#endif
%wraplist(JobDescriptionPtr, const Arc::JobDescription *);
%template(EndpointQueryingStatusMap) std::map<Arc::Endpoint, Arc::EndpointQueryingStatus>;
%template(EndpointSubmissionStatusMap) std::map<Arc::Endpoint, Arc::EndpointSubmissionStatus>;
%include "../src/hed/libs/compute/Submitter.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/compute/ComputingServiceRetriever.h
%{
#include <arc/compute/ComputingServiceRetriever.h>
%}
#ifdef SWIGJAVA
%rename(_wait) Arc::ComputingServiceRetriever::wait;
#endif
#ifdef SWIGPYTHON
%extend Arc::ComputingServiceRetriever {
  const std::list<ComputingServiceType>& getResults() { return *self; }  

  %insert("python") %{
    def __iter__(self):
      return self.getResults().__iter__()
  %}
}
%pythonprepend Arc::ComputingServiceRetriever::GetExecutionTargets %{
        etList = ExecutionTargetList()
        args = args + (etList,)
%}
%pythonappend Arc::ComputingServiceRetriever::GetExecutionTargets %{
        return etList
%}
#endif
%include "../src/hed/libs/compute/ComputingServiceRetriever.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/compute/Broker.h
%{
#include <arc/compute/Broker.h>
%}
%ignore Arc::Broker::operator=(const Broker&);
%ignore Arc::BrokerPluginArgument::operator const UserConfig&; // works with swig 1.3.40, and higher...
%ignore Arc::BrokerPluginArgument::operator const Arc::UserConfig&; // works with swig 1.3.29
#ifdef SWIGJAVA
%rename(compare) Arc::Broker::operator()(const ExecutionTarget&, const ExecutionTarget&) const;
%rename(compare) Arc::BrokerPlugin::operator()(const ExecutionTarget&, const ExecutionTarget&) const;
#endif
%include "../src/hed/libs/compute/Broker.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/compute/JobSupervisor.h
%{
#include <arc/compute/JobSupervisor.h>
%}
%include "../src/hed/libs/compute/JobSupervisor.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/compute/TestACCControl.h
%{
#include <arc/compute/TestACCControl.h>
%}
#ifdef SWIGPYTHON
%warnfilter(SWIGWARN_TYPEMAP_SWIGTYPELEAK) Arc::SubmitterPluginTestACCControl::submitJob;
%warnfilter(SWIGWARN_TYPEMAP_SWIGTYPELEAK) Arc::SubmitterPluginTestACCControl::migrateJob;
%rename(_BrokerPluginTestACCControl) Arc::BrokerPluginTestACCControl;
%rename(_JobDescriptionParserTestACCControl) Arc::JobDescriptionParserTestACCControl;
%rename(_JobControllerPluginTestACCControl) Arc::JobControllerPluginTestACCControl;
%rename(_SubmitterPluginTestACCControl) Arc::SubmitterPluginTestACCControl;
%rename(_ServiceEndpointRetrieverPluginTESTControl) Arc::ServiceEndpointRetrieverPluginTESTControl;
%rename(_TargetInformationRetrieverPluginTESTControl) Arc::TargetInformationRetrieverPluginTESTControl;
#endif
%include "../src/hed/libs/compute/TestACCControl.h"
#ifdef SWIGPYTHON
%pythoncode %{
BrokerPluginTestACCControl = StaticPropertyWrapper(_BrokerPluginTestACCControl)
JobDescriptionParserTestACCControl = StaticPropertyWrapper(_JobDescriptionParserTestACCControl)
JobControllerPluginTestACCControl = StaticPropertyWrapper(_JobControllerPluginTestACCControl)
SubmitterPluginTestACCControl = StaticPropertyWrapper(_SubmitterPluginTestACCControl)
ServiceEndpointRetrieverPluginTESTControl = StaticPropertyWrapper(_ServiceEndpointRetrieverPluginTESTControl)
TargetInformationRetrieverPluginTESTControl = StaticPropertyWrapper(_TargetInformationRetrieverPluginTESTControl)
%}
#endif
