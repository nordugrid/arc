#include <cctype>
#include <cstdlib>
#include <cstring>
#include <time.h>
#include <openssl/evp.h>

#include <arc/Base64.h>
#include <arc/Logger.h>
#include <arc/external/cJSON/cJSON.h>
#include <arc/communication/ClientInterface.h>
#include <arc/message/MCC.h>
#include <arc/XMLNode.h>
#include <arc/JSON.h>

#include "openid_metadata.h"


namespace Arc {

  Logger OpenIDMetadata::logger_(Logger::getRootLogger(), "OpenIDMetadata");
  Logger OpenIDMetadataFetcher::logger_(Logger::getRootLogger(), "OpenIDMetadataFetcher");
  Logger OpenIDTokenFetcher::logger_(Logger::getRootLogger(), "OpenIDTokenFetcher");

  OpenIDMetadata::OpenIDMetadata(): valid_(false), metadata_(NULL, &cJSON_Delete) {
    metadata_ = cJSON_CreateObject();
    if(!metadata_)
      return;
    // TODO: Add required items
    // cJSON_AddStringToObject(header_.Ptr(), HeaderNameAlgorithm, "none");
    valid_ = true;
  }

  OpenIDMetadata::OpenIDMetadata(std::string const& metadata): valid_(false), metadata_(NULL, &cJSON_Delete) {
    (void)Input(metadata);
  }

  bool OpenIDMetadata::Input(std::string const& metadata) {
    Cleanup();

    logger_.msg(DEBUG, "Input: metadata: %s", metadata);
    metadata_ = cJSON_Parse(metadata.c_str());
    if(!metadata_) return false;
    {
      char* metadataStr = cJSON_Print(metadata_.Ptr());
      logger_.msg(DEBUG, "Input: metadata: %s", metadataStr);
      std::free(metadataStr);
    }

    // TODO: check for presence of required parameters

    valid_ = true;
    return true;
  }

  bool OpenIDMetadata::Output(std::string& metadata) const {
    metadata.clear();
    if(!valid_)
      return false;

    char* jsonStr = cJSON_PrintUnformatted(metadata_.Ptr());
    if(jsonStr == NULL)
      return false;
    metadata.assign(jsonStr);
    std::free(jsonStr); 

    return true;
  }

  void OpenIDMetadata::Cleanup() {
    metadata_ = NULL;
    valid_ = false;
  }

  OpenIDMetadata::~OpenIDMetadata() {
    Cleanup();
  }

  cJSON const * OpenIDMetadata::Parameter(char const* name) const {
    if(!metadata_)
      return NULL;
    cJSON const* param = cJSON_GetObjectItem(const_cast<cJSON*>(metadata_.Ptr()), name);
    if(param == NULL)
      return NULL;
    return param;
  }

  char const * OpenIDMetadata::Parameter(char const* name, int idx) const {
    cJSON const * obj = Parameter(name);
    if(!obj)
      return NULL;
    if(obj->type == cJSON_String) {
      if((idx == 0) || (idx == -1)) {
        return obj->valuestring;
      }
    }
    else if(obj->type == cJSON_Array) {
      int arraySize = cJSON_GetArraySize(const_cast<cJSON*>(obj));
      if((idx >= 0) && (idx < arraySize)) {
        obj = cJSON_GetArrayItem(const_cast<cJSON*>(obj), idx);
        if(obj && (obj->type == cJSON_String)) {
          return obj->valuestring;
        }
      }
    }
    return NULL;
  }


  /*
  void OpenIDMetadata::Parameter(char const* name, cJSON const* value) const {
    if(!metadata_)
      return;
    if(name == NULL)
      return;
    if(value == NULL) const {
      cJSON_AddItemToObject(metadata_.Ptr(), name, cJSON_CreateNull());
      return;
    }
    cJSON_AddItemToObject(metadata_.Ptr(), name, cJSON_Duplicate(const_cast<cJSON*>(value), 1));
  }

  void OpenIDMetadata::Parameter(char const* name, char const* value) const {
    if(!header_)
      return;
    if(name == NULL)
      return;
    if(value == NULL) const {
      cJSON_AddItemToObject(metadata_.Ptr(), name, cJSON_CreateNull());
      return;
    }
    cJSON_AddItemToObject(metadata_.Ptr(), name, cJSON_CreateString(value));
  }
  */

  char const * const OpenIDMetadata::ErrorTag = "error";
  char const * const OpenIDMetadata::IssuerTag = "issuer";
  char const * const OpenIDMetadata::AuthorizationEndpointTag = "authorization_endpoint";
  char const * const OpenIDMetadata::TokenEndpointTag = "token_endpoint";
  char const * const OpenIDMetadata::UserinfoEndpointTag = "userinfo_endpoint";
  char const * const OpenIDMetadata::JWKSURITag = "jwks_uri";
  char const * const OpenIDMetadata::RegistrationEndpointTag = "registration_endpoint";
  char const * const OpenIDMetadata::ScopesSupportedTag = "scopes_supported";
  char const * const OpenIDMetadata::ClaimsSupportedTag = "claims_supported";
  char const * const OpenIDMetadata::ClaimTypesSupportedTag = "claim_types_supported";
  char const * const OpenIDMetadata::ResponseTypesSupportedTag = "response_types_supported";
  char const * const OpenIDMetadata::ResponseModesSupportedTag = "response_modes_supported";
  char const * const OpenIDMetadata::GrantTypesSupportedTag = "grant_types_supported";
  char const * const OpenIDMetadata::ACRValuesSupportedTag = "acr_values_supported";
  char const * const OpenIDMetadata::SubjectTypesSupportedTag = "subject_types_supported";
  char const * const OpenIDMetadata::IdTokenSigningAlgValuesSupportedTag = "id_token_signing_alg_values_supported";
  char const * const OpenIDMetadata::IdTokenEncryptionAlgValuesSupportedTag = "id_token_encryption_alg_values_supported";
  char const * const OpenIDMetadata::IdTokenEncryptionEncValuesSupportedTag = "id_token_encryption_enc_values_supported";
  char const * const OpenIDMetadata::UserinfoSigningAlgValuesSupportedTag = "userinfo_signing_alg_values_supported";
  char const * const OpenIDMetadata::UserinfoEncryptionAlgValuesSupportedTag = "userinfo_encryption_alg_values_supported";
  char const * const OpenIDMetadata::UserinfoEncryptionEncValuesSupportedTag = "userinfo_encryption_enc_values_supported";
  char const * const OpenIDMetadata::RequestObjectSigningAlgValuesSupportedTag = "request_object_signing_alg_values_supported";
  char const * const OpenIDMetadata::RequestObjectEncryptionAlgValuesSupportedTag = "request_object_encryption_alg_values_supported";
  char const * const OpenIDMetadata::RequestObjectEncryptionEncValuesSupportedTag = "request_object_encryption_enc_values_supported";
  char const * const OpenIDMetadata::TokenEndpointAuthMethodsSupportedTag = "token_endpoint_auth_methods_supported";
  char const * const OpenIDMetadata::TokenEndpointAuthSigningAlgValuesSupportedTag = "token_endpoint_auth_signing_alg_values_supported";
  char const * const OpenIDMetadata::DisplayValuesSupportedTag = "display_values_supported";
  char const * const OpenIDMetadata::ClaimsLocalesSupportedTag = "claims_locales_supported";
  char const * const OpenIDMetadata::UILocalesSupportedTag = "ui_locales_supported";
  char const * const OpenIDMetadata::ServiceDocumentationTag = "service_documentation";
  char const * const OpenIDMetadata::OpPolicyURITag = "op_policy_uri";
  char const * const OpenIDMetadata::OpTosURITag = "op_tos_uri";
  char const * const OpenIDMetadata::ClaimsParameterSupportedTag = "claims_parameter_supported";
  char const * const OpenIDMetadata::RequestParameterSupportedTag = "request_parameter_supported";
  char const * const OpenIDMetadata::RequestURIParameterSupportedTag = "request_uri_parameter_supported";
  char const * const OpenIDMetadata::RequireRequestURIRegistrationTag = "require_request_uri_registration";

  char const* OpenIDMetadata::Error() const {
    return Parameter(ErrorTag, -1);
  }

  char const* OpenIDMetadata::Issuer() const {
    return Parameter(IssuerTag, -1);
  }

  char const* OpenIDMetadata::AuthorizationEndpoint() const {
    return Parameter(AuthorizationEndpointTag, -1);
  }

  char const* OpenIDMetadata::TokenEndpoint() const {
    return Parameter(TokenEndpointTag, -1);
  }

  char const* OpenIDMetadata::UserinfoEndpoint() const {
    return Parameter(UserinfoEndpointTag, -1);
  }

  char const* OpenIDMetadata::RegistrationEndpoint() const {
    return Parameter(RegistrationEndpointTag, -1);
  }

  char const* OpenIDMetadata::JWKSURI() const {
    return Parameter(JWKSURITag, -1);
  }

  char const* OpenIDMetadata::ScopeSupported(int idx) const {
    return Parameter(ScopesSupportedTag, idx);
  }

  char const* OpenIDMetadata::ClaimSupported(int idx) const {
    return Parameter(ClaimsSupportedTag, idx);
  }

  char const* OpenIDMetadata::ClaimTypeSupported(int idx) const {
    return Parameter(ClaimTypesSupportedTag, idx);
  }

  char const* OpenIDMetadata::ResponseTypeSupported(int idx) const {
    return Parameter(ResponseTypesSupportedTag, idx);
  }

  char const* OpenIDMetadata::ResponseModeSupported(int idx) const {
    return Parameter(ResponseModesSupportedTag, idx);
  }

  char const* OpenIDMetadata::GrantTypeSupported(int idx) const {
    return Parameter(GrantTypesSupportedTag, idx);
  }

  char const* OpenIDMetadata::ACRValueSupported(int idx) const {
    return Parameter(ACRValuesSupportedTag, idx);
  }

  char const* OpenIDMetadata::SubjectTypeSupported(int idx) const {
    return Parameter(SubjectTypesSupportedTag, idx);
  }

  char const* OpenIDMetadata::IdTokenSigningAlgValueSupported(int idx) const {
    return Parameter(IdTokenSigningAlgValuesSupportedTag, idx);
  }

  char const* OpenIDMetadata::IdTokenEncryptionAlgValueSupported(int idx) const {
    return Parameter(IdTokenEncryptionAlgValuesSupportedTag, idx);
  }

  char const* OpenIDMetadata::IdTokenEncryptionEncValueSupported(int idx) const {
    return Parameter(IdTokenEncryptionEncValuesSupportedTag, idx);
  }

  char const* OpenIDMetadata::UserinfoSigningAlgValueSupported(int idx) const {
    return Parameter(UserinfoSigningAlgValuesSupportedTag, idx);
  }

  char const* OpenIDMetadata::UserinfoEncryptionAlgValueSupported(int idx) const {
    return Parameter(UserinfoEncryptionAlgValuesSupportedTag, idx);
  }

  char const* OpenIDMetadata::UserinfoEncryptionEncValueSupported(int idx) const {
    return Parameter(UserinfoEncryptionEncValuesSupportedTag, idx);
  }

  char const* OpenIDMetadata::RequestObjectSigningAlgValueSupported(int idx) const {
    return Parameter(RequestObjectSigningAlgValuesSupportedTag, idx);
  }

  char const* OpenIDMetadata::RequestObjectEncryptionAlgValueSupported(int idx) const {
    return Parameter(RequestObjectEncryptionAlgValuesSupportedTag, idx);
  }

  char const* OpenIDMetadata::RequestObjectEncryptionEncValueSupported(int idx) const {
    return Parameter(RequestObjectEncryptionEncValuesSupportedTag, idx);
  }

  char const* OpenIDMetadata::TokenEndpointAuthMethodSupported(int idx) const {
    return Parameter(TokenEndpointAuthMethodsSupportedTag, idx);
  }

  char const* OpenIDMetadata::TokenEndpointAuthSigningAlgValueSupported(int idx) const {
    return Parameter(TokenEndpointAuthSigningAlgValuesSupportedTag, idx);
  }

  char const* OpenIDMetadata::DisplayValueSupported(int idx) const {
    return Parameter(DisplayValuesSupportedTag, idx);
  }

  char const* OpenIDMetadata::ClaimsLocaleSupported(int idx) const {
    return Parameter(ClaimsLocalesSupportedTag, idx);
  }

  char const* OpenIDMetadata::UILocaleSupported(int idx) const {
    return Parameter(UILocalesSupportedTag, idx);
  }

  char const* OpenIDMetadata::ServiceDocumentation(int idx) const {
    return Parameter(ServiceDocumentationTag, idx);
  }

  char const* OpenIDMetadata::OpPolicyURI() const {
    return Parameter(OpPolicyURITag, -1);
  }

  char const* OpenIDMetadata::OpTosURI() const {
    return Parameter(OpTosURITag, -1);
  }

  bool OpenIDMetadata::ClaimsParameterSupported() const {
    cJSON const * obj = Parameter(ClaimsParameterSupportedTag);
    if(obj && (obj->type == cJSON_True)) return true;
    return false;
  }

  bool OpenIDMetadata::RequestParameterSupported() const {
    cJSON const * obj = Parameter(RequestParameterSupportedTag);
    if(obj && (obj->type == cJSON_True)) return true;
    return false;
  }

  bool OpenIDMetadata::RequestURIParameterSupported() const {
    cJSON const * obj = Parameter(RequestURIParameterSupportedTag);
    if(obj && (obj->type == cJSON_False)) return false;
    return true;
  }

  bool OpenIDMetadata::RequireRequestURIRegistration() const {
    cJSON const * obj = Parameter(RequireRequestURIRegistrationTag);
    if(obj && (obj->type == cJSON_True)) return true;
    return false;
  }

  static URL url_no_cred(char const * url_str) {
    URL url(url_str);
    url.AddOption("tlscred=none");
    return url;
  }

  static Arc::MCCConfig make_config(UserConfig& userconfig) {
    Arc::MCCConfig config;
    userconfig.ApplyToConfig(config);
    return config;
  }

  OpenIDMetadataFetcher::OpenIDMetadataFetcher(char const * issuer_url, UserConfig& userconfig):
       url_(issuer_url?url_no_cred(issuer_url):URL()), client_(make_config(userconfig), url_) {
    client_.RelativeURI(true);
  }

  bool OpenIDMetadataFetcher::Fetch(OpenIDMetadata& metadata) {
    HTTPClientInfo info;
    PayloadRaw request;
    PayloadRawInterface* response(NULL);
    std::string path = url_.Path();
    if (path.empty() || (path[path.length()-1] != '/')) path += '/';
    path += ".well-known/openid-configuration";
    MCC_Status status = client_.process("GET", path, &request, &info, &response);
    if(!status)
      return false;
    if(!response)
      return false;
    if(!(response->Content()))
      return false;
    return metadata.Input(response->Content());
  }

  bool OpenIDMetadataFetcher::Import(char const * content, OpenIDMetadata& metadata) {
    if(!content) return false;
    return metadata.Input(content);
  }



  OpenIDTokenFetcher::OpenIDTokenFetcher(char const * token_endpoint, UserConfig& userconfig, char const * id, char const * secret):
       url_(token_endpoint?url_no_cred(token_endpoint):URL()), client_(make_config(userconfig), url_),
       client_id_(id?id:""), client_secret_(secret?secret:"") {
    client_.RelativeURI(true);
  }


  bool OpenIDTokenFetcher::Fetch(std::string const & grant,
                                 std::string const & subject,
                                 std::list<std::string> const & scope,
                                 std::list<std::string> const & audience,
                                 TokenList& tokens) {
    HTTPClientInfo info;
    PayloadRawInterface* response(NULL);

    std::multimap<std::string, std::string> attributes;
 
    if((!client_id_.empty()) || (!client_secret_.empty())) {
      std::string credAttr("Basic ");
      credAttr.append(Base64::encode(client_id_+":"+client_secret_));
      attributes.insert(std::make_pair("Authorization", credAttr));
    }

    PayloadRaw request;
    std::string tokenReq;

    if(!grant.empty()) {
      if(!tokenReq.empty()) tokenReq.append("&");
      tokenReq.append("grant_type=");
      tokenReq.append(uri_encode(grant, true));
    } else if ((!client_id_.empty()) || (!client_secret_.empty())) {
      if(!tokenReq.empty()) tokenReq.append("&");
      tokenReq.append("grant_type=");
      tokenReq.append(uri_encode("client_credentials", true));
    }

    if(!subject.empty()) {
      if(!tokenReq.empty()) tokenReq.append("&");
      tokenReq.append("subject_token=");
      tokenReq.append(uri_encode(subject, true));
    }

    if(!audience.empty()) {
      if(!tokenReq.empty()) tokenReq.append("&");
      tokenReq.append("audience=");
      tokenReq.append(uri_encode(join(audience, " "), true));
    }

    if(!scope.empty()) {
      if(!tokenReq.empty()) tokenReq.append("&");
      tokenReq.append("scope=");
      tokenReq.append(uri_encode(join(scope, " "), true));
    }

    if(!tokenReq.empty()) {
      attributes.insert(std::make_pair("Content-Type", "application/x-www-form-urlencoded"));
      request.Insert(tokenReq.c_str());
    }

    ClientHTTPAttributes attr("POST", attributes);
    MCC_Status status = client_.process(attr, &request, &info, &response);

    // Response looks like 
    //  {
    //    "access_token":"eyJra...ByTX0",
    //    "token_type":"Bearer",
    //    "scope":"wlcg"
    //  }

    if(!status)
      return false;

    if(info.code != 200) {
      logger_.msg(DEBUG, "Fetch: response code: %u %s", info.code, info.reason);
      char const * responseContent = response ? response->Content() : NULL;
      if(responseContent) logger_.msg(DEBUG, "Fetch: response body: %s", responseContent);
      return false;
    }

    if(response) {
      char const * responseContent = response->Content();
      if(responseContent) {
        XMLNode respXml;
        if(JSON::Parse(respXml, responseContent)) {
          for(int n = 0; ;++n) {
            XMLNode node = respXml.Child(n);
            if(!node) break;
            tokens.push_back(std::make_pair(node.Name(), (std::string)node));            
          }
        }
      }
    }

    return true;
  }

  bool OpenIDTokenFetcher::Import(char const * content, TokenList& tokens) {
    if(!content) return false;
    XMLNode respXml;
    if(!JSON::Parse(respXml, content)) return false;
    for(int n = 0; ;++n) {
      XMLNode node = respXml.Child(n);
      if(!node) break;
      tokens.push_back(std::make_pair(node.Name(), (std::string)node));            
    }
    return true;
  }


} // namespace Arc
