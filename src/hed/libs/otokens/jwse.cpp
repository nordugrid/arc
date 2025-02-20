#include <cctype>
#include <cstdlib>
#include <cstring>
#include <time.h>
#include <openssl/evp.h>

#include <arc/Base64.h>
#include <arc/Logger.h>
#include <arc/external/cJSON/cJSON.h>
#include <arc/crypto/OpenSSL.h>

#include "otokens.h"
#include "jwse_private.h"


namespace Arc {

  Logger JWSE::logger_(Logger::getRootLogger(), "JWSE");

  char const * const JWSE::ClaimNameSubject = "sub";
  char const * const JWSE::ClaimNameIssuer = "iss";
  char const * const JWSE::ClaimNameAudience = "aud";
  char const * const JWSE::ClaimNameNotAfter = "exp";
  char const * const JWSE::ClaimNameNotBefore = "nbf";

  char const * const JWSE::ClaimNameActivities = "scp";

  char const* const JWSE::HeaderNameAlgorithm = "alg";
  char const* const JWSE::HeaderNameEncryption = "enc";

  JWSE::JWSE(): valid_(false), header_(NULL, &cJSON_Delete), keyOrigin_(NoKey) {
    OpenSSLInit();

    header_ = cJSON_CreateObject();
    if(!header_)
      return;
    content_ = cJSON_CreateObject();
    if(!content_)
      return;
    // Required items
    cJSON_AddStringToObject(header_.Ptr(), HeaderNameAlgorithm, "none");
    cJSON_AddItemToObject(content_.Ptr(), ClaimNameActivities, cJSON_CreateArray());
    valid_ = true;
  }

  JWSE::JWSE(std::string const& jwseCompact, UserConfig& userconfig): valid_(false), header_(NULL, &cJSON_Delete), keyOrigin_(NoKey) {
    OpenSSLInit();

    (void)Input(jwseCompact, userconfig);
  }

  bool JWSE::Input(std::string const& jwseCompact, UserConfig& userconfig) {
    Cleanup();

    logger_.msg(DEBUG, "JWSE::Input: token: %s", jwseCompact);
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
    {
      char* headerStr = cJSON_Print(header_.Ptr());
      logger_.msg(DEBUG, "JWSE::Input: header: %s", headerStr);
      std::free(headerStr);
    }
    cJSON* algObject = cJSON_GetObjectItem(header_.Ptr(), HeaderNameAlgorithm);
    if(algObject == NULL) return false; // Neither JWS nor JWE
    if(algObject->type != cJSON_String) return false;
    if(algObject->valuestring == NULL) return false;
    cJSON* encObject = cJSON_GetObjectItem(header_.Ptr(), HeaderNameEncryption);
    if (encObject == NULL) {
      //      JWS
      char const* payloadStart = pos;
      while(*pos != '.') {
        if(*pos == '\0') return false;
        ++pos;
      }
      char const* payloadEnd = pos;
      ++pos;
      std::string payload = Base64::decodeURLSafe(payloadStart, payloadEnd-payloadStart);
      if(!payload.empty()) {
        content_ = cJSON_Parse(payload.c_str());
      } else {
        content_ = cJSON_CreateObject();
      }
      if(!content_) return false;
      {
        char* contentStr = cJSON_Print(content_.Ptr());
        logger_.msg(DEBUG, "JWSE::Input: JWS content: %s", contentStr);
        std::free(contentStr);
      }

      // Time
      cJSON* notBefore = cJSON_GetObjectItem(content_.Ptr(), ClaimNameNotBefore);
      if(notBefore) {
        if(notBefore->type != cJSON_Number) return false;
        time_t notBeforeTime = static_cast<time_t>(notBefore->valueint);
        if(static_cast<int>(time(NULL)-notBeforeTime) < 0) {
          logger_.msg(DEBUG, "JWSE::Input: JWS: token too young");
          return false;
        }
      }
      cJSON* notAfter = cJSON_GetObjectItem(content_.Ptr(), ClaimNameNotAfter);
      if(notAfter) {
        if(notAfter->type != cJSON_Number) return false;
        time_t notAfterTime = static_cast<time_t>(notAfter->valueint);
        if(static_cast<int>(notAfterTime - time(NULL)) < 0) {
          logger_.msg(DEBUG, "JWSE::Input: JWS: token too old");
          return false;
        }
      }

      // Signature
      if(!ExtractPublicKey(userconfig)) return false;
      char const* signatureStart = pos;
      char const* signatureEnd = jwseCompact.c_str() + jwseCompact.length();
      std::string signature = Base64::decodeURLSafe(signatureStart, signatureEnd-signatureStart);
      bool verifyResult = false;
      logger_.msg(DEBUG, "JWSE::Input: JWS: signature algorithm: %s", algObject->valuestring);
      signAlg_ = algObject->valuestring;
      if(strcmp(algObject->valuestring, "none") == 0) {
        verifyResult = signature.empty(); // expecting empty signature if no protection is requested
        keyOrigin_ = NoKey;
        signAlg_.clear();
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
      } else if(strcmp(algObject->valuestring, "RS256") == 0) {
        verifyResult = VerifyRSASSAPKCS1("SHA256", joseStart, payloadEnd-joseStart,
                         reinterpret_cast<unsigned char const*>(signature.c_str()), signature.length());
      } else if(strcmp(algObject->valuestring, "RS384") == 0) {
        verifyResult = VerifyRSASSAPKCS1("SHA384", joseStart, payloadEnd-joseStart,
                         reinterpret_cast<unsigned char const*>(signature.c_str()), signature.length());
      } else if(strcmp(algObject->valuestring, "RS512") == 0) {
        verifyResult = VerifyRSASSAPKCS1("SHA512", joseStart, payloadEnd-joseStart,
                         reinterpret_cast<unsigned char const*>(signature.c_str()), signature.length());
      } else if(strcmp(algObject->valuestring, "PS256") == 0) {
        verifyResult = VerifyRSASSAPSS("SHA256", joseStart, payloadEnd-joseStart,
                         reinterpret_cast<unsigned char const*>(signature.c_str()), signature.length());
      } else if(strcmp(algObject->valuestring, "PS384") == 0) {
        verifyResult = VerifyRSASSAPSS("SHA384", joseStart, payloadEnd-joseStart,
                         reinterpret_cast<unsigned char const*>(signature.c_str()), signature.length());
      } else if(strcmp(algObject->valuestring, "PS512") == 0) {
        verifyResult = VerifyRSASSAPSS("SHA512", joseStart, payloadEnd-joseStart,
                         reinterpret_cast<unsigned char const*>(signature.c_str()), signature.length());
      } else {
        logger_.msg(DEBUG, "JWSE::Input: JWS: signature algorithn not supported: %s",algObject->valuestring);
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

      if(!verifyResult) {
        logger_.msg(DEBUG, "JWSE::Input: JWS: signature verification failed");
        return false;
      }
    } else {
      // JWE - not yet
      header_ = NULL;
      logger_.msg(DEBUG, "JWSE::Input: JWE: not supported yet");
      return false;
    }      
    valid_ = true;
    return true;
  }

  bool JWSE::Output(std::string& jwseCompact) const {
    jwseCompact.clear();
    if(!valid_)
      return false;

    bool keyAdded(true);
    if(!InsertPublicKey(keyAdded))
      return false;
    cJSON_DeleteItemFromObject(header_.Ptr(), HeaderNameAlgorithm);
    if(keyAdded) {
      cJSON_AddStringToObject(header_.Ptr(), HeaderNameAlgorithm, "ES512");
    } else {     
      cJSON_AddStringToObject(header_.Ptr(), HeaderNameAlgorithm, "none");
    }

    char* joseStr = cJSON_PrintUnformatted(header_.Ptr());
    if(joseStr == NULL)
      return false;
    jwseCompact += Base64::encodeURLSafe(joseStr);
    std::free(joseStr); 
    jwseCompact += '.';
    char* payloadStr = cJSON_PrintUnformatted(content_.Ptr());
    if(payloadStr == NULL)
      return false;
    jwseCompact += Base64::encodeURLSafe(payloadStr);
    std::free(payloadStr); 

    std::string signature;
    if(keyAdded) {
      if(!SignECDSA("SHA512", jwseCompact.c_str(), jwseCompact.length(), signature))
        return false;
    }

    jwseCompact += '.';
    jwseCompact += Base64::encodeURLSafe(signature.c_str(), signature.length());

    return true;
  }

  void JWSE::Cleanup() {
    header_ = NULL;
    content_ = NULL;
    key_ = NULL;
  }

  JWSE::~JWSE() {
    Cleanup();
  }

  //char const* JWSE::Content() const {
  //  return content_.c_str();
  //}

  int JWSE::ActivitiesNum() const {
    if(!content_)
      return 0;
    cJSON const* activities = cJSON_GetObjectItem(const_cast<cJSON*>(content_.Ptr()), ClaimNameActivities);
    if(!activities)
      return 0;
    if(activities->type != cJSON_Array)
      return 0;
    return cJSON_GetArraySize(const_cast<cJSON*>(activities));
  }

  std::pair<std::string,std::string> JWSE::Activity(int index) {
    std::pair<std::string,std::string> activity;
    if(index < 0)
      return activity;
    if(!content_)
      return activity;
    cJSON const* activities = cJSON_GetObjectItem(const_cast<cJSON*>(content_.Ptr()), ClaimNameActivities);
    if(!activities)
      return activity;
    if(activities->type != cJSON_Array)
      return activity;
    cJSON* activityObj = cJSON_GetArrayItem(const_cast<cJSON*>(activities), index);
    if(!activityObj)
      return activity;
    if(activityObj->type != cJSON_String)
      return activity;
    activity.first.assign(activityObj->valuestring);
    std::string::size_type sep = activity.first.find(':');
    if(sep != std::string::npos) {
      activity.second.assign(activity.first.c_str()+sep+1);
      activity.first.resize(sep);
    };
    return activity;
  }

  void JWSE::RemoveActivity(int index) {
    if(index < 0)
      return;
    if(!content_)
      return;
    cJSON* activities = cJSON_GetObjectItem(const_cast<cJSON*>(content_.Ptr()), ClaimNameActivities);
    if(!activities)
      return;
    if(activities->type != cJSON_Array)
      return;
    cJSON_DeleteItemFromArray(activities, index);
  }


  void JWSE::AddActivity(std::pair<std::string,std::string> const& activity, int index) {
    if(!content_)
      return;
    cJSON* activities = cJSON_GetObjectItem(const_cast<cJSON*>(content_.Ptr()), ClaimNameActivities);
    if(!activities) {
      activities = cJSON_CreateArray();
      cJSON_AddItemToObject(content_.Ptr(), ClaimNameActivities, activities);
    }
    if(index > cJSON_GetArraySize(activities)) index = -1;
    std::string activityStr = activity.first;
    if(!activity.second.empty()) {
      activityStr += ":";
      activityStr += activity.second;
    };
    if(index < 0) {
      cJSON_AddItemToArray(activities, cJSON_CreateString(activityStr.c_str()));
    } else {

    };
  }

  void JWSE::AddActivity(std::string const& activity, int index) {
    AddActivity(std::pair<std::string,std::string>(activity,""), index);
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

  std::set<std::string> JWSE::ClaimNames() const {
    std::set<std::string> names;
    if(!content_)
      return names;
    for(int n = 0;; ++n) {
      cJSON const * param = cJSON_GetArrayItem(content_.Ptr(),n);
      if(!param) break;
      if(param->string) {
	names.insert(param->string);
      }
    }

    return names;
  }

  cJSON const* JWSE::Claim(char const* name) const {
    if(!name)
      return NULL;
    if(!content_)
      return NULL;
    cJSON const* param = cJSON_GetObjectItem(const_cast<cJSON*>(content_.Ptr()), name);
    if(param == NULL)
      return NULL;
    return param;
  }

  void JWSE::Claim(char const* name, cJSON const* value) {
    if(!content_)
      return;
    if(name == NULL)
      return;
    if(value == NULL) {
      cJSON_AddItemToObject(content_.Ptr(), name, cJSON_CreateNull());
      return;
    }
    cJSON_AddItemToObject(content_.Ptr(), name, cJSON_Duplicate(const_cast<cJSON*>(value), 1));
  }

  void JWSE::Claim(char const* name, char const* value) {
    if(!content_)
      return;
    if(name == NULL)
      return;
    if(value == NULL) {
      cJSON_AddItemToObject(content_.Ptr(), name, cJSON_CreateNull());
      return;
    }
    cJSON_AddItemToObject(content_.Ptr(), name, cJSON_CreateString(value));
  }

/*



   private:
    cJSON* header_;

     std::string content_;
*/


} // namespace Arc
