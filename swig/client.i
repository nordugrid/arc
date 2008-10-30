%{
#include <arc/client/ClientInterface.h>
#include <arc/client/Submitter.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/client/JobController.h>
#include <arc/client/JobDescription.h>
#include <arc/client/JobSupervisor.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/TargetRetriever.h>
#include <arc/client/UserConfig.h>
%}

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

%include "../src/hed/libs/client/ClientInterface.h"
%include "../src/hed/libs/client/Submitter.h"
%include "../src/hed/libs/client/ExecutionTarget.h"
%include "../src/hed/libs/client/Job.h"
%include "../src/hed/libs/client/JobController.h"
%include "../src/hed/libs/client/JobDescription.h"
%include "../src/hed/libs/client/JobSupervisor.h"
%include "../src/hed/libs/client/TargetGenerator.h"
%include "../src/hed/libs/client/TargetRetriever.h"
%include "../src/hed/libs/client/UserConfig.h"
