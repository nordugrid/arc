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
%}

%template(ExecutionTargetList) std::list<Arc::ExecutionTarget>;
%template(JobControllerList) std::list<Arc::JobController *>;
%template(JobList) std::list<Arc::Job>;
%template(JobDescriptionList) std::list<Arc::JobDescription>;
%template(JobStateList) std::list<Arc::JobState>;
%template(FileTypeList) std::list<Arc::FileType>;
%template(ApplicationEnvironmentList) std::list<Arc::ApplicationEnvironment>;
%template(SoftwareList) std::list<Arc::Software>;
%template(SoftwareRequirementList) std::list<Arc::SoftwareRequirement>;
%template(TargetRetrieverList) std::list<Arc::TargetRetriever*>;
%template(BrokerList) std::list<Arc::Broker*>;
%template(SubmitterList) std::list<Arc::Submitter*>;

#ifdef SWIGJAVA
%ignore Arc::SoftwareRequirement::getComparisonOperatorList() const;

%template(ExecutionTargetListIteratorHandler) listiteratorhandler<Arc::ExecutionTarget>;
%template(JobControllerListIteratorHandler) listiteratorhandler<Arc::JobController *>;
%template(JobListIteratorHandler) listiteratorhandler<Arc::Job>;
%template(JobDescriptionListIteratorHandler) listiteratorhandler<Arc::JobDescription>;
%template(JobStateListIteratorHandler) listiteratorhandler<Arc::JobState>;
%template(FileTypeListIteratorHandler) listiteratorhandler<Arc::FileType>;
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

/* These template instantiations must be created after the respective
   template classes have been defined, which is done in the
   JobDescription class
 */
%template(ScalableTimeInt) Arc::ScalableTime<int>;
%template(RangeInt) Arc::Range<int>;
%template(RangeInt64) Arc::Range<int64_t>;
