/**
 * Note that the order of the "%include" statements are important! If a
 * "%include" depends on other "%include"s, it should be placed after these
 * "%include" dependencies.
 */

%{
#include <arc/client/ClientInterface.h>
#include <arc/client/ClientX509Delegation.h>
#include <arc/client/Submitter.h>
#include <arc/client/Software.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/JobState.h>
#include <arc/client/Job.h>
#include <arc/client/Broker.h>
#include <arc/client/Endpoint.h>
#include <arc/client/JobController.h>
#include <arc/client/JobDescription.h>
#include <arc/client/JobSupervisor.h>
#include <arc/client/EndpointQueryingStatus.h>
#include <arc/client/TestACCControl.h>
#include <arc/client/EntityRetriever.h>
%}

%template(JobControllerList) std::list<Arc::JobController *>;
%template(JobControllerMap) std::map<std::string, Arc::JobController *>;
%template(JobList) std::list<Arc::Job>;
%template(JobDescriptionList) std::list<Arc::JobDescription>;
%template(JobStateList) std::list<Arc::JobState>;
%template(InputFileTypeList) std::list<Arc::InputFileType>;
%template(OutputFileTypeList) std::list<Arc::OutputFileType>;
%template(ApplicationEnvironmentList) std::list<Arc::ApplicationEnvironment>;
%template(SoftwareList) std::list<Arc::Software>;
%template(SoftwareRequirementList) std::list<Arc::SoftwareRequirement>;
%template(BrokerList) std::list<Arc::Broker*>;
%template(SubmitterList) std::list<Arc::Submitter*>;
%template(EndpointList) std::list<Arc::Endpoint>;
%template(ExecutionTargetList) std::list<Arc::ExecutionTarget>;
%template(ComputingServiceList) std::list<Arc::ComputingServiceType>;
%template(ComputingEndpointMap) std::map<int, Arc::ComputingEndpointType>;
%template(ComputingShareMap) std::map<int, Arc::ComputingShareType>;
%template(ComputingManagerMap) std::map<int, Arc::ComputingManagerType>;
%template(ExecutionEnvironmentMap) std::map<int, Arc::ExecutionEnvironmentType>;

#ifdef SWIGPYTHON
/* When instantiating a template of the form
 * Arc::CountedPointer< T<S> > two __nonzero__
 * python methods are created in the generated python module file which
 * causes an swig error. The two __nonzero__ methods probably stems from
 * the "operator bool" and "operator ->" methods. At least in the Arc.i
 * file the "operator bool" method is renamed to "__nonzero__". In
 * order to avoid that name clash the following "operator bool" methods 
 * is ignored.
 */
%ignore Arc::CountedPointer< std::map<std::string, double> >::operator bool;
%ignore Arc::CountedPointer< std::list<Arc::ApplicationEnvironment> >::operator bool;
#endif // SWIGPYTHON

%template(SharedBenchmarkMap) Arc::CountedPointer< std::map<std::string, double> >;
%template(SharedApplicationEnvironmentList) Arc::CountedPointer< std::list<Arc::ApplicationEnvironment> >;

%include "../src/hed/libs/client/GLUE2Entity.h"
%template(GLUE2EntityLocationAttributes) Arc::GLUE2Entity<Arc::LocationAttributes>;
%template(CPLocationAttributes) Arc::CountedPointer<Arc::LocationAttributes>;
%template(GLUE2EntityAdminDomainAttributes) Arc::GLUE2Entity<Arc::AdminDomainAttributes>;
%template(CPAdminDomainAttributes) Arc::CountedPointer<Arc::AdminDomainAttributes>;
%template(GLUE2EntityExecutionEnvironmentAttributes) Arc::GLUE2Entity<Arc::ExecutionEnvironmentAttributes>;
%template(CPExecutionEnvironmentAttributes) Arc::CountedPointer<Arc::ExecutionEnvironmentAttributes>;
%template(GLUE2Entity) Arc::GLUE2Entity<Arc::ComputingManagerAttributes>;
%template(CPComputingManagerAttributes) Arc::CountedPointer<Arc::ComputingManagerAttributes>;
%template(GLUE2EntityComputingShareAttributes) Arc::GLUE2Entity<Arc::ComputingShareAttributes>;
%template(CPComputingShareAttributes) Arc::CountedPointer<Arc::ComputingShareAttributes>;
%template(GLUE2EntityComputingEndpointAttributes) Arc::GLUE2Entity<Arc::ComputingEndpointAttributes>;
%template(CPComputingEndpointAttributes) Arc::CountedPointer<Arc::ComputingEndpointAttributes>;
%template(GLUE2EntityComputingServiceAttributes) Arc::GLUE2Entity<Arc::ComputingServiceAttributes>;
%template(CPComputingServiceAttributes) Arc::CountedPointer<Arc::ComputingServiceAttributes>;


#ifdef SWIGJAVA
%ignore Arc::SoftwareRequirement::getComparisonOperatorList() const;

%template(ExecutionTargetListIteratorHandler) listiteratorhandler<Arc::ExecutionTarget>;
%template(JobControllerListIteratorHandler) listiteratorhandler<Arc::JobController *>;
%template(JobListIteratorHandler) listiteratorhandler<Arc::Job>;
%template(JobDescriptionListIteratorHandler) listiteratorhandler<Arc::JobDescription>;
%template(JobStateListIteratorHandler) listiteratorhandler<Arc::JobState>;
%template(InputFileTypeListIteratorHandler) listiteratorhandler<Arc::InputFileType>;
%template(OutputFileTypeListIteratorHandler) listiteratorhandler<Arc::OutputFileType>;
%template(ApplicationEnvironmentListIteratorHandler) listiteratorhandler<Arc::ApplicationEnvironment>;
%template(SoftwareListIteratorHandler) listiteratorhandler<Arc::Software>;
%template(SoftwareRequirementListIteratorHandler) listiteratorhandler<Arc::SoftwareRequirement>;
%template(BrokerListIteratorHandler) listiteratorhandler<Arc::Broker*>;
%template(SubmitterListIteratorHandler) listiteratorhandler<Arc::Submitter*>;
#endif


#ifdef SWIGPYTHON
namespace Arc {

/* this typemap tells SWIG that we don't want to use the 'PayloadSOAP ** response' argument from the target language,
but we need a temporary PayloadSOAP for internal use,
and we want this argument to point to this temporary 'PayloadSOAP *' variable */
%typemap(in, numinputs=0) PayloadSOAP ** response (PayloadSOAP *temp) {
    $1 = &temp;
}

/* this typemap tells SWIG what we want to do with the 'PayloadSOAP ** response' argument after the method finished:
we want to return a python tuple (a list) with two members,
the first member will be the '*response' and the second member is the original return value, the MCC_Status. */
%typemap(argout) PayloadSOAP ** response {
    PyObject *o, *tuple;
    o = SWIG_NewPointerObj(*$1, SWIGTYPE_p_Arc__PayloadSOAP, SWIG_POINTER_OWN | 0 );
    tuple = PyTuple_New(2);
    PyTuple_SetItem(tuple,0,o);
    PyTuple_SetItem(tuple,1,$result);
    $result = tuple;
}

%extend JobDescription {
  bool Parse(const std::string& str, std::list<JobDescription>& jobdescs, const std::string& lang = "", const std::string& dialect = "") { return Arc::JobDescription::Parse(str, jobdescs, lang, dialect); }
};

%rename(_BrokerTestACCControl) BrokerTestACCControl;
%rename(_JobDescriptionParserTestACCControl) JobDescriptionParserTestACCControl;
%rename(_JobControllerTestACCControl) JobControllerTestACCControl;
%rename(_SubmitterTestACCControl) SubmitterTestACCControl;
%rename(_ServiceEndpointRetrieverPluginTESTControl) ServiceEndpointRetrieverPluginTESTControl;
%rename(_TargetInformationRetrieverPluginTESTControl) TargetInformationRetrieverPluginTESTControl;

}

#endif

#ifdef SWIGJAVA
/* The std::cout object will always exist, so do not set any references. See
 * comments in Arc.i.
 */
%typemap(javaout) std::ostream& getStdout {
  return new $javaclassname($jnicall, $owner);
}

%inline %{
std::ostream& getStdout() {
  return std::cout;
}
%}

%ignore Arc::LocationType::operator->;
%ignore Arc::LocationType::operator*;
%ignore Arc::AdminDomainType::operator->;
%ignore Arc::AdminDomainType::operator*;
%ignore Arc::ExecutionEnvironmentType::operator->;
%ignore Arc::ExecutionEnvironmentType::operator*;
%ignore Arc::ComputingManagerType::operator->;
%ignore Arc::ComputingManagerType::operator*;
%ignore Arc::ComputingShareType::operator->;
%ignore Arc::ComputingShareType::operator*;
%ignore Arc::ComputingEndpointType::operator->;
%ignore Arc::ComputingEndpointType::operator*;
%ignore Arc::ComputingServiceType::operator->;
%ignore Arc::ComputingServiceType::operator*;


#endif

%include "../src/hed/libs/client/ClientInterface.h"
%apply std::string& INOUT { std::string& delegation_id };
%include "../src/hed/libs/client/ClientX509Delegation.h"
%clear std::string& delegation_id;
%include "../src/hed/libs/client/Submitter.h"
%include "../src/hed/libs/client/Software.h"
%include "../src/hed/libs/client/Endpoint.h"
%include "../src/hed/libs/client/ExecutionTarget.h"
%include "../src/hed/libs/client/JobState.h"
%include "../src/hed/libs/client/Job.h"
%include "../src/hed/libs/client/Broker.h"
%include "../src/hed/libs/client/JobController.h"
%include "../src/hed/libs/client/JobDescription.h"
%include "../src/hed/libs/client/JobSupervisor.h"
%include "../src/hed/libs/client/EndpointQueryingStatus.h"
%include "../src/hed/libs/client/TestACCControl.h"
#ifdef SWIGJAVA
%rename(waitUntilDone) wait;
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
%include "../src/hed/libs/client/EntityRetriever.h"
%template(ServiceEndpointRetriever) Arc::EntityRetriever<Arc::Endpoint>;
%template(TargetInformationRetriever) Arc::EntityRetriever<Arc::ComputingServiceType>;
%template(JobListRetriever) Arc::EntityRetriever<Arc::Job>;
%template(EndpointContainer) Arc::EntityContainer<Arc::Endpoint>;
%template(ComputingServiceContainer) Arc::EntityContainer<Arc::ComputingServiceType>;
%template(JobContainer) Arc::EntityContainer<Arc::Job>;
%template(ServiceEndpointQueryOptions) Arc::EndpointQueryOptions<Arc::Endpoint>;
%template(ComputingServiceQueryOptions) Arc::EndpointQueryOptions<Arc::ComputingServiceType>;


/* These template instantiations must be created after the respective
   template classes have been defined, which is done in the
   JobDescription class
 */
%template(ScalableTimeInt) Arc::ScalableTime<int>;
%template(RangeInt) Arc::Range<int>;
%template(StringOptIn) Arc::OptIn<std::string>;

#ifdef SWIGPYTHON
namespace Arc {
%pythoncode %{
class StaticPropertyWrapper(object):
    def __init__(self, wrapped_class):
        object.__setattr__(self, "wrapped_class", wrapped_class)

    def __getattr__(self, name):
        orig_attr = getattr(self.wrapped_class, name)
        if isinstance(orig_attr, property):
            return orig_attr.fget()
        else:
            return orig_attr

    def __setattr__(self, name, value):
        orig_attr = getattr(self.wrapped_class, name)
        if isinstance(orig_attr, property):
            orig_attr.fset(value)
        else:
            setattr(self.wrapped_class, name, value)

BrokerTestACCControl = StaticPropertyWrapper(_BrokerTestACCControl)
JobDescriptionParserTestACCControl = StaticPropertyWrapper(_JobDescriptionParserTestACCControl)
JobControllerTestACCControl = StaticPropertyWrapper(_JobControllerTestACCControl)
SubmitterTestACCControl = StaticPropertyWrapper(_SubmitterTestACCControl)
ServiceEndpointRetrieverPluginTESTControl = StaticPropertyWrapper(_ServiceEndpointRetrieverPluginTESTControl)
TargetInformationRetrieverPluginTESTControl = StaticPropertyWrapper(_TargetInformationRetrieverPluginTESTControl)

%}

}
#endif
