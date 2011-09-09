/**
 * Note that the order of the "%include" statements are important! If a
 * "%include" depends on other "%include"s, it should be placed after these
 * "%include" dependencies.
 */

%{
#include <arc/crypto/OpenSSL.h>
#include <arc/credential/Credential.h>
#include <arc/ArcRegex.h>
#include <arc/credential/VOMSUtil.h>
#include <arc/credentialstore/CredentialStore.h>
%}

%template(VOMSACInfoVector) std::vector<Arc::VOMSACInfo>;
%template(StringVector) std::vector<std::string>;

%import <openssl/safestack.h>
%include "../src/hed/libs/crypto/OpenSSL.h"
%include "../src/hed/libs/credential/Credential.h"
%include "../src/hed/libs/common/ArcRegex.h"
%include "../src/hed/libs/credential/VOMSUtil.h"
%include "../src/hed/libs/credentialstore/CredentialStore.h"
