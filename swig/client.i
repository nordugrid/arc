%{
#include <arc/client/ACC.h>
#include <arc/client/ACCLoader.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/ClientX509Delegation.h>
#include <arc/client/Submitter.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/Broker.h>
#include <arc/client/JobController.h>
#include <arc/client/JobDescription.h>
#include <arc/client/JobSupervisor.h>
#include <arc/client/TargetRetriever.h>
#include <arc/client/UserConfig.h>
%}

#ifdef SWIGPYTHON
%template(ExecutionTargetList) std::list<Arc::ExecutionTarget>;
%template(JobControllerList) std::list<Arc::JobController *>;
%template(JobList) std::list<Arc::Job>;

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

%extend ACCLoader {
/*
  The 'this' keyword in SWIG is replaced by the 'self' keyword.
  In versions >= 1.3.30 it is recommented to use '$self' instead of 'self', but 'self' will work in any 1.3.x version.
*/
  Arc::Broker * getBroker(const std::string& brokername) { return dynamic_cast<Arc::Broker*>( self->getACC(brokername) ); }
  Arc::JobController * getBroker(const std::string& jobcontrollername) { return dynamic_cast<Arc::JobController*>( self->getACC(jobcontrollername) ); }
}
}
#endif


#ifdef SWIGJAVA
%template(ExecutionTargetList) std::list<Arc::ExecutionTarget>;
%template(ExecutionTargetListIteratorHandler) listiteratorhandler<Arc::ExecutionTarget>;
%template(JobControllerList) std::list<Arc::JobController *>;
%template(JobControllerListIteratorHandler) listiteratorhandler<Arc::JobController *>;
%template(JobList) std::list<Arc::Job>;

%inline %{
void stream(std::ostream& os) {
 os << "hello from stream method" << std::endl;
}
std::ostream& getStdout() {
  return std::cout;
}
%} 
#endif


%include "../src/hed/libs/client/ACC.h"
%include "../src/hed/libs/client/ACCLoader.h"
%include "../src/hed/libs/client/ClientInterface.h"
%apply std::string& INOUT { std::string& delegation_id };
%include "../src/hed/libs/client/ClientX509Delegation.h"
%clear std::string& delegation_id;
%include "../src/hed/libs/client/Submitter.h"
%include "../src/hed/libs/client/ExecutionTarget.h"
%include "../src/hed/libs/client/Job.h"
%include "../src/hed/libs/client/TargetGenerator.h"
%include "../src/hed/libs/client/Broker.h"
%include "../src/hed/libs/client/JobController.h"
%include "../src/hed/libs/client/JobDescription.h"
%include "../src/hed/libs/client/JobSupervisor.h"
%include "../src/hed/libs/client/TargetRetriever.h"
%include "../src/hed/libs/client/UserConfig.h"
