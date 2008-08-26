%{
#include <arc/client/ClientInterface.h>
%}

namespace Arc {

%typemap(in, numinputs=0) PayloadSOAP ** response (PayloadSOAP *temp) {
    $1 = &temp;
}

%typemap(argout) PayloadSOAP ** response {
    $result = SWIG_NewPointerObj(*$1, SWIGTYPE_p_Arc__PayloadSOAP, SWIG_POINTER_OWN | 0 );
}

}

%include "../src/hed/libs/client/ClientInterface.h"
