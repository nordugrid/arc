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
#include <arc/client/TargetGenerator.h>
#include <arc/client/Broker.h>
#include <arc/client/JobController.h>
#include <arc/client/JobDescription.h>
#include <arc/client/JobSupervisor.h>
#include <arc/client/TargetRetriever.h>
#include <arc/client/TestACCControl.h>
#include <arc/client/EndpointQueryingStatus.h>
#include <arc/client/EndpointRetriever.h>
#include <arc/client/ServiceEndpointRetriever.h>
#include <arc/client/TargetInformationRetriever.h>
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
%template(TargetRetrieverList) std::list<Arc::TargetRetriever*>;
%template(BrokerList) std::list<Arc::Broker*>;
%template(SubmitterList) std::list<Arc::Submitter*>;
%template(ServiceEndpointList) std::list<Arc::ServiceEndpoint>;
%template(ExecutionTargetList) std::list<Arc::ExecutionTarget>;

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
%template(TargetRetrieverListIteratorHandler) listiteratorhandler<Arc::TargetRetriever*>;
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

%rename(_TargetRetrieverTestACCControl) TargetRetrieverTestACCControl;
%rename(_BrokerTestACCControl) BrokerTestACCControl;
%rename(_JobDescriptionParserTestACCControl) JobDescriptionParserTestACCControl;
%rename(_JobControllerTestACCControl) JobControllerTestACCControl;
%rename(_SubmitterTestACCControl) SubmitterTestACCControl;
%rename(_TargetRetrieverTestACCControl) TargetRetrieverTestACCControl;
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

#endif

%include "../src/hed/libs/client/ClientInterface.h"
%apply std::string& INOUT { std::string& delegation_id };
%include "../src/hed/libs/client/ClientX509Delegation.h"
%clear std::string& delegation_id;
%include "../src/hed/libs/client/Submitter.h"
%include "../src/hed/libs/client/Software.h"
%include "../src/hed/libs/client/ExecutionTarget.h"
%include "../src/hed/libs/client/JobState.h"
%include "../src/hed/libs/client/Job.h"
%include "../src/hed/libs/client/TargetGenerator.h"
%include "../src/hed/libs/client/Broker.h"
%include "../src/hed/libs/client/JobController.h"
%include "../src/hed/libs/client/JobDescription.h"
%include "../src/hed/libs/client/JobSupervisor.h"
%include "../src/hed/libs/client/TargetRetriever.h"
%include "../src/hed/libs/client/TestACCControl.h"
%include "../src/hed/libs/client/EndpointQueryingStatus.h"
%include "../src/hed/libs/client/EndpointRetriever.h"
#ifdef SWIGJAVA
%rename(waitUntilDone) wait;
#endif
%template(EndpointRetrieverRS) Arc::EndpointRetriever<Arc::RegistryEndpoint, Arc::ServiceEndpoint>;
%template(TargetInformationRetriever) Arc::EndpointRetriever<Arc::ComputingInfoEndpoint, Arc::ExecutionTarget>;
%template(ServiceEndpointContainer) Arc::EndpointContainer<Arc::ServiceEndpoint>;
%template(ExecutionTargetContainer) Arc::EndpointContainer<Arc::ExecutionTarget>;
%include "../src/hed/libs/client/ServiceEndpointRetriever.h"
%include "../src/hed/libs/client/TargetInformationRetriever.h"
%template(ServiceEndpointFilter) Arc::EndpointFilter<Arc::ServiceEndpoint>;
%template(ExecutionTargetFilter) Arc::EndpointFilter<Arc::ExecutionTarget>;



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
TargetRetrieverTestACCControl = StaticPropertyWrapper(_TargetRetrieverTestACCControl)
ServiceEndpointRetrieverPluginTESTControl = StaticPropertyWrapper(_ServiceEndpointRetrieverPluginTESTControl)
TargetInformationRetrieverPluginTESTControl = StaticPropertyWrapper(_TargetInformationRetrieverPluginTESTControl)

%}

}
#endif
