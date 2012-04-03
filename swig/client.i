// Wrap contents of $(top_srcdir)/src/hed/libs/client/ClientInterface.h
%{
#include <arc/client/ClientInterface.h>
%}
#ifdef SWIGPYTHON
/* These typemaps tells SWIG that we don't want to use the
 * 'PayloadSOAP ** response' argument from the target language, but we
 * need a temporary PayloadSOAP for internal use, and we want this
 * argument to point to this temporary 'PayloadSOAP *' variable (in).
 * Then after the method finished: we want to return a python tuple (a
 * list) with two members, the first member will be the '*response' and
 * the second member is the original return value, the MCC_Status
 * (argout).
 */
%typemap(in, numinputs=0) Arc::PayloadSOAP ** response (Arc::PayloadSOAP *temp) {
  $1 = &temp;
}
%typemap(argout) Arc::PayloadSOAP ** response {
  $result = PyTuple_Pack(2, SWIG_NewPointerObj(*$1, SWIGTYPE_p_Arc__PayloadSOAP, SWIG_POINTER_OWN | 0 ), $result);
} /* applies to:
 * MCC_Status ClientSOAP::process(PayloadSOAP *request, PayloadSOAP **response);
 * MCC_Status ClientSOAP::process(const std::string& action, PayloadSOAP *request, PayloadSOAP **response);
 */
#endif
%include "../src/hed/libs/client/ClientInterface.h"
#ifdef SWIGPYTHON
%clear Arc::PayloadSOAP ** response;
#endif


// Wrap contents of $(top_srcdir)/src/hed/libs/client/ClientX509Delegation.h
%{
#include <arc/client/ClientX509Delegation.h>
%}
#ifdef SWIGPYTHON
%apply std::string& TUPLEOUTPUTSTRING { std::string& delegation_id };
/* Currently applies to:
 * bool ClientX509Delegation::createDelegation(DelegationType deleg, std::string& delegation_id);
 * bool ClientX509Delegation::acquireDelegation(DelegationType deleg, std::string& delegation_cred, std::string& delegation_id,
                                                const std::string cred_identity = "", const std::string cred_delegator_ip = "",
                                                const std::string username = "", const std::string password = "");
 * Look in Arc.i for a description of the OUTPUT typemap.
 */
#endif
%include "../src/hed/libs/client/ClientX509Delegation.h"
#ifdef SWIGPYTHON
%clear std::string& delegation_id;
#endif


// Wrap contents of $(top_srcdir)/src/hed/libs/client/Submitter.h
%{
#include <arc/client/Submitter.h>
%}
%ignore Arc::SubmitterPluginArgument::operator const UserConfig&;
%template(SubmitterList) std::list<Arc::Submitter*>;
#ifdef SWIGJAVA
%template(SubmitterListIteratorHandler) listiteratorhandler<Arc::Submitter*>;
#endif
%include "../src/hed/libs/client/Submitter.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/client/Software.h
%{
#include <arc/client/Software.h>
%}
%ignore Arc::Software::convert(const ComparisonOperatorEnum& co);
%ignore Arc::Software::toString(ComparisonOperator co);
%ignore Arc::SoftwareRequirement::SoftwareRequirement(const Software& sw, Software::ComparisonOperator swComOp = &Software::operator==);
%ignore Arc::SoftwareRequirement::add(const Software& sw, Software::ComparisonOperator swComOp = &Software::operator==);
%template(SoftwareList) std::list<Arc::Software>;
%template(SoftwareRequirementList) std::list<Arc::SoftwareRequirement>;
#ifdef SWIGJAVA
%ignore Arc::Software::operator();
%ignore Arc::SoftwareRequirement::getComparisonOperatorList() const;
%template(SoftwareListIteratorHandler) listiteratorhandler<Arc::Software>;
%template(SoftwareRequirementListIteratorHandler) listiteratorhandler<Arc::SoftwareRequirement>;
#endif
%include "../src/hed/libs/client/Software.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/client/Endpoint.h
%{
#include <arc/client/Endpoint.h>
%}
%template(EndpointList) std::list<Arc::Endpoint>;
#ifdef SWIGJAVA
%template(EndpointListIteratorHandler) listiteratorhandler<Arc::Endpoint>;
#endif
%include "../src/hed/libs/client/Endpoint.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/client/ExecutionTarget.h
%{
#include <arc/client/ExecutionTarget.h>
%}
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
%include "../src/hed/libs/client/GLUE2Entity.h" // Contains declaration of the GLUE2Entity template, used in ExecutionTarget.h file.
%template(ApplicationEnvironmentList) std::list<Arc::ApplicationEnvironment>;
%template(ExecutionTargetList) std::list<Arc::ExecutionTarget>;
%template(ComputingServiceList) std::list<Arc::ComputingServiceType>;
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
#ifdef SWIGJAVA
%template(ExecutionTargetListIteratorHandler) listiteratorhandler<Arc::ExecutionTarget>;
%template(ApplicationEnvironmentListIteratorHandler) listiteratorhandler<Arc::ApplicationEnvironment>;
%template(ComputingServiceListIteratorHandler) listiteratorhandler<Arc::ComputingServiceType>;
#endif
%include "../src/hed/libs/client/ExecutionTarget.h"
%extend Arc::ComputingServiceType {
  %template(GetExecutionTargetsFromList) GetExecutionTargets< std::list<Arc::ExecutionTarget> >;
  %template(GetExecutionTargetsFromSet) GetExecutionTargets<Arc::ExecutionTargetSet>;
};


// Wrap contents of $(top_srcdir)/src/hed/libs/client/JobState.h
%{
#include <arc/client/JobState.h>
%}
%rename(GetType) Arc::JobState::operator StateType;
%rename(GetNativeState) Arc::JobState::operator();
%template(JobStateList) std::list<Arc::JobState>;
#ifdef SWIGJAVA
%template(JobStateListIteratorHandler) listiteratorhandler<Arc::JobState>;
#endif
%include "../src/hed/libs/client/JobState.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/client/Job.h
%{
#include <arc/client/Job.h>
%}
%template(JobList) std::list<Arc::Job>;
#ifdef SWIGPYTHON
%ignore Arc::Job::WriteJobIDsToFile(const std::list<Job>&, const std::string&, unsigned = 10, unsigned = 500000); // Clash. It is sufficient to wrap only WriteJobIDsToFile(cosnt std::list<URL>&, ...);
#endif
#ifdef SWIGJAVA
%template(JobListIteratorHandler) listiteratorhandler<Arc::Job>;
#endif
%include "../src/hed/libs/client/Job.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/client/JobController.h
%{
#include <arc/client/JobController.h>
%}
%ignore Arc::JobControllerPluginArgument::operator const UserConfig&;
%template(JobControllerList) std::list<Arc::JobController *>;
%template(JobControllerMap) std::map<std::string, Arc::JobController *>;
#ifdef SWIGJAVA
%template(JobControllerListIteratorHandler) listiteratorhandler<Arc::JobController *>;
#endif
%include "../src/hed/libs/client/JobController.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/client/JobSupervisor.h
%{
#include <arc/client/JobSupervisor.h>
%}
%include "../src/hed/libs/client/JobSupervisor.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/client/EndpointQueryingStatus.h
%{
#include <arc/client/EndpointQueryingStatus.h>
%}
%include "../src/hed/libs/client/EndpointQueryingStatus.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/client/TestACCControl.h
%{
#include <arc/client/TestACCControl.h>
%}
#ifdef SWIGPYTHON
%warnfilter(SWIGWARN_TYPEMAP_SWIGTYPELEAK) Arc::SubmitterTestACCControl::submitJob;
%warnfilter(SWIGWARN_TYPEMAP_SWIGTYPELEAK) Arc::SubmitterTestACCControl::migrateJob;
%rename(_BrokerPluginTestACCControl) Arc::BrokerPluginTestACCControl;
%rename(_JobDescriptionParserTestACCControl) Arc::JobDescriptionParserTestACCControl;
%rename(_JobControllerTestACCControl) Arc::JobControllerTestACCControl;
%rename(_SubmitterTestACCControl) Arc::SubmitterTestACCControl;
%rename(_ServiceEndpointRetrieverPluginTESTControl) Arc::ServiceEndpointRetrieverPluginTESTControl;
%rename(_TargetInformationRetrieverPluginTESTControl) Arc::TargetInformationRetrieverPluginTESTControl;
%pythoncode %{
BrokerPluginTestACCControl = StaticPropertyWrapper(_BrokerPluginTestACCControl)
JobDescriptionParserTestACCControl = StaticPropertyWrapper(_JobDescriptionParserTestACCControl)
JobControllerTestACCControl = StaticPropertyWrapper(_JobControllerTestACCControl)
SubmitterTestACCControl = StaticPropertyWrapper(_SubmitterTestACCControl)
ServiceEndpointRetrieverPluginTESTControl = StaticPropertyWrapper(_ServiceEndpointRetrieverPluginTESTControl)
TargetInformationRetrieverPluginTESTControl = StaticPropertyWrapper(_TargetInformationRetrieverPluginTESTControl)
%}
#endif
%include "../src/hed/libs/client/TestACCControl.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/client/JobDescription.h
%{
#include <arc/client/JobDescription.h>
%}
%ignore Arc::Range<int>::operator int;
#ifdef SWIGPYTHON
%apply std::string& TUPLEOUTPUTSTRING { std::string& product }; /* Applies to:
 * JobDescriptionResult JobDescription::UnParse(std::string& product, std::string language, const std::string& dialect = "") const;
 */
#endif
#ifdef SWIGJAVA
%ignore Arc::JobDescription::GetAlternatives() const;
#endif
%include "../src/hed/libs/client/JobDescription.h"
%template(JobDescriptionList) std::list<Arc::JobDescription>;
%template(InputFileTypeList) std::list<Arc::InputFileType>;
%template(OutputFileTypeList) std::list<Arc::OutputFileType>;
%template(SourceTypeList) std::list<Arc::SourceType>;
%template(TargetTypeList) std::list<Arc::TargetType>;
%template(ScalableTimeInt) Arc::ScalableTime<int>;
%template(RangeInt) Arc::Range<int>;
%template(StringOptIn) Arc::OptIn<std::string>;
#ifdef SWIGPYTHON
%clear std::string& product;
#endif
#ifdef SWIGJAVA
%template(JobDescriptionListIteratorHandler) listiteratorhandler<Arc::JobDescription>;
%template(InputFileTypeListIteratorHandler) listiteratorhandler<Arc::InputFileType>;
%template(OutputFileTypeListIteratorHandler) listiteratorhandler<Arc::OutputFileType>;
%template(SourceTypeListIteratorHandler) listiteratorhandler<Arc::SourceType>;
%template(TargetTypeListIteratorHandler) listiteratorhandler<Arc::TargetType>;
#endif


// Wrap contents of $(top_srcdir)/src/hed/libs/client/EntityRetriever.h
%{
#include <arc/client/EntityRetriever.h>
%}
#ifdef SWIGJAVA
%rename(_wait) Arc::EntityRetriever::wait;
#endif
%include "../src/hed/libs/client/EntityRetriever.h"
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


// Wrap contents of $(top_srcdir)/src/hed/libs/client/ComputingServiceRetriever.h
%{
#include <arc/client/ComputingServiceRetriever.h>
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
%include "../src/hed/libs/client/ComputingServiceRetriever.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/client/Broker.h
%{
#include <arc/client/Broker.h>
%}
%ignore Arc::BrokerPluginArgument::operator const UserConfig&;
/* Currently the CountedBroker cannot be wrapped since a default
 * constructor (no arguments) for the CountedBroker and Broker classes
 * is required when wrapping. More investigation is needed.
 */
%ignore Arc::CountedBroker;
%warnfilter(SWIGWARN_TYPE_UNDEFINED_CLASS) Arc::CountedBroker;
%ignore Arc::ExecutionTargetSet;
%warnfilter(SWIGWARN_TYPE_UNDEFINED_CLASS) Arc::ExecutionTargetSet;
//%template() Arc::CountedPointer<Arc::Broker>;
#ifdef SWIGPYTHON
//%template() std::set<Arc::ExecutionTarget, Arc::CountedBroker>;
%ignore Arc::ExecutionTargetSet::ExecutionTargetSet(const Broker&, const std::list<ComputingServiceType>&); // Use Arc::ExecutionTargetSet::ExecutionTargetSet(const Broker&, const std::list<ComputingServiceType>&, const std::list<URL>& rejectEndpoints) with empty list instead.
#endif
#ifdef SWIGJAVA
/* For java, swig doesn't provide means for wrapping a std::set, thus a
 * interface must be made before the Arc::ExecutionTargetSet class can
 * be wrapped.
 */
%warnfilter(SWIGWARN_TYPE_UNDEFINED_CLASS) Arc::ExecutionTargetSet;
%rename(compare) Arc::Broker::operator()(const ExecutionTarget&, const ExecutionTarget&) const;
%rename(compare) Arc::BrokerPlugin::operator()(const ExecutionTarget&, const ExecutionTarget&) const;
%rename(compare) Arc::CountedBroker::operator()(const ExecutionTarget&, const ExecutionTarget&) const;
#endif
%include "../src/hed/libs/client/Broker.h"
