#ifdef SWIGPYTHON
%module credential

%include "Arc.i"
// Import common module in order to access common template types.
%import(module="common") "common.i"

%import "../src/hed/libs/common/UserConfig.h"
%import "../src/hed/libs/common/DateTime.h"
%import "../src/hed/libs/common/URL.h"
%import "../src/hed/libs/common/Logger.h"
#endif


/* STACK_OF, which is used in 'Credential.h' and 'VOMSUtil.h' is not
 * known to swig. Providing definition below. If definition is not
 * provided swig encounters a syntax error.
 */
#ifdef STACK_OF
#undef STACK_OF
#endif
#define STACK_OF( A ) void

// Wrap contents of $(top_srcdir)/src/hed/libs/crypto/OpenSSL.h
%{
#include <arc/crypto/OpenSSL.h>
%}
%include "../src/hed/libs/crypto/OpenSSL.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/credential/PasswordSource.h
%{
#include <arc/credential/PasswordSource.h>
%}
%include "../src/hed/libs/credential/PasswordSource.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/credential/Credential.h
%{
#include <arc/credential/Credential.h>
%}
%ignore Arc::Credential::GetLifetime;
%include "../src/hed/libs/credential/Credential.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/common/ArcRegex.h
%{
#include <arc/ArcRegex.h>
%}
%ignore Arc::RegularExpression::operator=(const RegularExpression&);
%include "../src/hed/libs/common/ArcRegex.h"


// Wrap contents of $(top_srcdir)/src/hed/libs/credential/VOMSUtil.h
#ifdef SWIGPYTHON
/* 'from' is a python keyword, renaming to _from.
 */
%rename(_from) Arc::VOMSACInfo::from;
#endif
%{
#include <arc/credential/VOMSUtil.h>
%}
%include "../src/hed/libs/credential/VOMSUtil.h"
%template(VOMSACInfoVector) std::vector<Arc::VOMSACInfo>;
%template(StringVectorVector) std::vector< std::vector<std::string> >;


// Wrap contents of $(top_srcdir)/src/hed/libs/credentialstore/CredentialStore.h
%{
#include <arc/credentialstore/CredentialStore.h>
%}
%ignore Arc::CredentialStore::operator!;
%include "../src/hed/libs/credentialstore/CredentialStore.h"
