#include <cctype>
#include <cstdlib>
#include <cstring>
#include <openssl/evp.h>

#include <arc/Base64.h>
#include <arc/external/cJSON/cJSON.h>

#include "jwse.h"
#include "jwse_private.h"


namespace Arc {

  char const * const JWSE::HeaderNameSubject = "sub";
  char const * const JWSE::HeaderNameIssuer = "iss";
  char const * const JWSE::HeaderNameAudience = "aud";

  static char const* HeaderNameAlgorithm = "alg";

  JWSE::JWSE(): valid_(false), header_(NULL, &cJSON_Delete) {
    header_ = cJSON_CreateObject();
    if(!header_)
      return;
    cJSON* algStr = cJSON_CreateString("none");
    if(algStr == NULL) {
      Cleanup();
      return;
    }
    cJSON_AddItemToObject(header_.Ptr(), HeaderNameAlgorithm, algStr);
    valid_ = true;
  }

  JWSE::JWSE(std::string const& jwseCompact): valid_(false), header_(NULL, &cJSON_Delete) {
    (void)Input(jwseCompact);
  }

  bool JWSE::Input(std::string const& jwseCompact) {
    Cleanup();

    char const* pos = jwseCompact.c_str();
    while(std::isspace(*pos) != 0) {
      if(*pos == '\0') return false;
      ++pos;
    }
    // Identify header
    char const* joseStart = pos;
    while(*pos != '.') {
      if(*pos == '\0') return false;
      ++pos;
    }
    char const* joseEnd = pos;
    ++pos;
    // Decode header so we know if we have JWS or JWE
    std::string joseStr = Base64::decodeURLSafe(joseStart, joseEnd-joseStart);
    header_ = cJSON_Parse(joseStr.c_str());
    if(!header_) return false;
    cJSON* algObject = cJSON_GetObjectItem(header_.Ptr(), HeaderNameAlgorithm);
    if(algObject == NULL) return false; // Neither JWS nor JWE
    if(algObject->type != cJSON_String) return false;
    if(algObject->valuestring == NULL) return false;
    cJSON* encObject = cJSON_GetObjectItem(header_.Ptr(), "enc");
    if (encObject == NULL) {
      // JWS
      if(!ExtractPublicKey()) return false;
      char const* payloadStart = pos;
      while(*pos != '.') {
        if(*pos == '\0') return false;
        ++pos;
      }
      char const* payloadEnd = pos;
      ++pos;
      content_ = Base64::decodeURLSafe(payloadStart, payloadEnd-payloadStart);
      char const* signatureStart = pos;
      char const* signatureEnd = jwseCompact.c_str() + jwseCompact.length();
      std::string signature = Base64::decodeURLSafe(payloadStart, payloadEnd-payloadStart);
      bool verifyResult = false;
      if(strcmp(algObject->valuestring, "none") == 0) {
        verifyResult = signature.empty(); // expecting empty signature if no protection is requested
      } else if(strcmp(algObject->valuestring, "HS256") == 0) {
        verifyResult = VerifyHMAC("SHA256", joseStart, payloadEnd-joseStart,
                         reinterpret_cast<unsigned char const*>(signature.c_str()), signature.length());
      } else if(strcmp(algObject->valuestring, "HS384") == 0) {
        verifyResult = VerifyHMAC("SHA384", joseStart, payloadEnd-joseStart,
                         reinterpret_cast<unsigned char const*>(signature.c_str()), signature.length());
      } else if(strcmp(algObject->valuestring, "HS512") == 0) {
        verifyResult = VerifyHMAC("SHA512", joseStart, payloadEnd-joseStart,
                         reinterpret_cast<unsigned char const*>(signature.c_str()), signature.length());
      } else if(strcmp(algObject->valuestring, "ES256") == 0) {
        verifyResult = VerifyECDSA("SHA256", joseStart, payloadEnd-joseStart,
                         reinterpret_cast<unsigned char const*>(signature.c_str()), signature.length());
      } else if(strcmp(algObject->valuestring, "ES384") == 0) {
        verifyResult = VerifyECDSA("SHA384", joseStart, payloadEnd-joseStart,
                         reinterpret_cast<unsigned char const*>(signature.c_str()), signature.length());
      } else if(strcmp(algObject->valuestring, "ES512") == 0) {
        verifyResult = VerifyECDSA("SHA512", joseStart, payloadEnd-joseStart,
                         reinterpret_cast<unsigned char const*>(signature.c_str()), signature.length());
      }
/*
   | RS256        | RSASSA-PKCS1-v1_5 using       | Recommended        |
   |              | SHA-256                       |                    |
   | RS384        | RSASSA-PKCS1-v1_5 using       | Optional           |
   |              | SHA-384                       |                    |
   | RS512        | RSASSA-PKCS1-v1_5 using       | Optional           |
   |              | SHA-512                       |                    |
   | PS256        | RSASSA-PSS using SHA-256 and  | Optional           |
   |              | MGF1 with SHA-256             |                    |
   | PS384        | RSASSA-PSS using SHA-384 and  | Optional           |
   |              | MGF1 with SHA-384             |                    |
   | PS512        | RSASSA-PSS using SHA-512 and  | Optional           |
   |              | MGF1 with SHA-512             |                    |
*/

      if(!verifyResult) return false;
    } else {
      // JWE - not yet
      header_ = NULL;
      return false;
    }      
    valid_ = true;
    return true;
  }

  bool JWSE::Output(std::string& jwseCompact) const {
    jwseCompact.clear();
    if(!valid_)
      return false;

    char* joseStr = cJSON_PrintUnformatted(header_.Ptr());
    if(joseStr == NULL)
      return false;
    jwseCompact += Base64::encodeURLSafe(joseStr);
    std::free(joseStr); 
    jwseCompact += '.';
    jwseCompact += Base64::encodeURLSafe(content_.c_str());
    jwseCompact += '.';
    // No signature
    return true;
  }

  void JWSE::Cleanup() {
    header_ = NULL;
    key_ = NULL;
  }

  JWSE::~JWSE() {
    Cleanup();
  }

  char const* JWSE::Content() const {
    return content_.c_str();
  }

  cJSON const* JWSE::HeaderParameter(char const* name) const {
    if(!header_)
      return NULL;
    cJSON const* param = cJSON_GetObjectItem(const_cast<cJSON*>(header_.Ptr()), name);
    if(param == NULL)
      return NULL;
    return param;
  }


  void JWSE::HeaderParameter(char const* name, cJSON const* value) {
    if(!header_)
      return;
    if(name == NULL)
      return;
    if(value == NULL) {
      cJSON_AddItemToObject(header_.Ptr(), name, cJSON_CreateNull());
      return;
    }
    cJSON_AddItemToObject(header_.Ptr(), name, cJSON_Duplicate(const_cast<cJSON*>(value), 1));
  }

  void JWSE::HeaderParameter(char const* name, char const* value) {
    if(!header_)
      return;
    if(name == NULL)
      return;
    if(value == NULL) {
      cJSON_AddItemToObject(header_.Ptr(), name, cJSON_CreateNull());
      return;
    }
    cJSON_AddItemToObject(header_.Ptr(), name, cJSON_CreateString(value));
  }

/*



   private:
    cJSON* header_;

     std::string content_;
*/


} // namespace Arc
