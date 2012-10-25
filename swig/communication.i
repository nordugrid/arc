// Wrap contents of $(top_srcdir)/src/hed/libs/client/ClientInterface.h
%{
#include <arc/communication/ClientInterface.h>
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
%include "../src/hed/libs/communication/ClientInterface.h"
#ifdef SWIGPYTHON
%clear Arc::PayloadSOAP ** response;
#endif


// Wrap contents of $(top_srcdir)/src/hed/libs/client/ClientX509Delegation.h
%{
#include <arc/communication/ClientX509Delegation.h>
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
%include "../src/hed/libs/communication/ClientX509Delegation.h"
#ifdef SWIGPYTHON
%clear std::string& delegation_id;
#endif
