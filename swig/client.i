%{
#include <arc/client/ClientInterface.h>
%}

namespace Arc {

%typemap(in, numinputs=0) PayloadSOAP ** response (PayloadSOAP *temp) {
    $1 = &temp;
}

%typemap(argout) PayloadSOAP ** response {
    PyObject *o, *tuple;
    o = SWIG_NewPointerObj(*$1, SWIGTYPE_p_Arc__PayloadSOAP, SWIG_POINTER_OWN | 0 );
    if ((!$result) || ($result == Py_None)) {
        $result = o;
    } else {
        tuple = PyTuple_New(2);
        PyTuple_SetItem(tuple,0,o);
        PyTuple_SetItem(tuple,1,$result);
        $result = tuple;
    }
}

}

%include "../src/hed/libs/client/ClientInterface.h"
