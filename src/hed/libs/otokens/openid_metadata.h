#include <string>
#include <arc/Utils.h>
#include <arc/URL.h>
#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/communication/ClientInterface.h>


struct cJSON;

namespace Arc {

  //! Class for parsing, verifying and extracting information
  //! from OpenID Metadata.
  class OpenIDMetadata {
   public:
    static char const * const ErrorTag;
    static char const * const IssuerTag;
    static char const * const AuthorizationEndpointTag;
    static char const * const TokenEndpointTag;
    static char const * const UserinfoEndpointTag;
    static char const * const JWKSURITag;
    static char const * const RegistrationEndpointTag;
    static char const * const ScopesSupportedTag;
    static char const * const ClaimsSupportedTag;
    static char const * const ClaimTypesSupportedTag;
    static char const * const ResponseTypesSupportedTag;
    static char const * const ResponseModesSupportedTag;
    static char const * const GrantTypesSupportedTag;
    static char const * const ACRValuesSupportedTag;
    static char const * const SubjectTypesSupportedTag;
    static char const * const IdTokenSigningAlgValuesSupportedTag;
    static char const * const IdTokenSigningEncValuesSupportedTag;
    static char const * const IdTokenEncryptionAlgValuesSupportedTag;
    static char const * const IdTokenEncryptionEncValuesSupportedTag;
    static char const * const UserinfoSigningAlgValuesSupportedTag;
    static char const * const UserinfoEncryptionAlgValuesSupportedTag;
    static char const * const UserinfoEncryptionEncValuesSupportedTag;
    static char const * const RequestObjectSigningAlgValuesSupportedTag;
    static char const * const RequestObjectEncryptionAlgValuesSupportedTag;
    static char const * const RequestObjectEncryptionEncValuesSupportedTag;
    static char const * const TokenEndpointAuthMethodsSupportedTag;
    static char const * const TokenEndpointAuthSigningAlgValuesSupportedTag;
    static char const * const DisplayValuesSupportedTag;
    static char const * const ClaimsLocalesSupportedTag;
    static char const * const UILocalesSupportedTag;
    static char const * const ServiceDocumentationTag;
    static char const * const OpPolicyURITag;
    static char const * const OpTosURITag;
    static char const * const ClaimsParameterSupportedTag;
    static char const * const RequestParameterSupportedTag;
    static char const * const RequestURIParameterSupportedTag;
    static char const * const RequireRequestURIRegistrationTag;

    //! Parse metadata as simple string.
    OpenIDMetadata(std::string const& jwseCompact);

    //! Default contructor creates empty metadata.
    OpenIDMetadata();    

    //! Ordinary destructor
    ~OpenIDMetadata();

    cJSON const * Parameter(char const* name) const;

    char const * Parameter(char const* name, int idx) const;

    //! Returns true if object represents valid SciToken
    operator bool() const { return valid_; }

    //! Returns true if object does not represents valid SciToken
    bool operator!() const { return !valid_; }

    //! Parses passed JSON and stores collected information in this object.
    bool Input(std::string const& metadata);

    //! Serializes stored SciToken into string container.
    bool Output(std::string& metadta) const;

    char const* Error() const;

    char const* Issuer() const;

    char const* AuthorizationEndpoint() const;

    char const* TokenEndpoint() const;

    char const* UserinfoEndpoint() const;

    char const* RegistrationEndpoint() const;

    char const* JWKSURI() const;

    char const* ScopeSupported(int idx) const;

    char const* ClaimSupported(int idx) const;

    char const* ClaimTypeSupported(int idx) const;

    char const* ResponseTypeSupported(int idx) const;

    char const* ResponseModeSupported(int idx) const;

    char const* GrantTypeSupported(int idx) const;

    char const* ACRValueSupported(int idx) const;

    char const* SubjectTypeSupported(int idx) const;

    char const* IdTokenSigningAlgValueSupported(int idx) const;

    char const* IdTokenEncryptionAlgValueSupported(int idx) const;

    char const* IdTokenEncryptionEncValueSupported(int idx) const;

    char const* UserinfoSigningAlgValueSupported(int idx) const;

    char const* UserinfoEncryptionAlgValueSupported(int idx) const;

    char const* UserinfoEncryptionEncValueSupported(int idx) const;

    char const* RequestObjectSigningAlgValueSupported(int idx) const;

    char const* RequestObjectEncryptionAlgValueSupported(int idx) const;

    char const* RequestObjectEncryptionEncValueSupported(int idx) const;

    char const* TokenEndpointAuthMethodSupported(int idx) const;

    char const* TokenEndpointAuthSigningAlgValueSupported(int idx) const;

    char const* DisplayValueSupported(int idx) const;

    char const* ClaimsLocaleSupported(int idx) const;

    char const* UILocaleSupported(int idx) const;

    char const* ServiceDocumentation(int idx) const;

    char const* OpPolicyURI() const;

    char const* OpTosURI() const;

    bool ClaimsParameterSupported() const;

    bool RequestParameterSupported() const;

    bool RequestURIParameterSupported() const;

    bool RequireRequestURIRegistration() const;

   private:    

    static Logger logger_;

    bool valid_;

    mutable AutoPointer<cJSON> metadata_; 

    void Cleanup();

    OpenIDMetadata(OpenIDMetadata const &);

    OpenIDMetadata& operator=(OpenIDMetadata const &);

  }; // class OpenIDMetadata

  class OpenIDMetadataFetcher {
   public:
    OpenIDMetadataFetcher(char const * issuer_url, UserConfig& userconfig);
    bool Fetch(OpenIDMetadata& metadata);
    static bool Import(char const * content, OpenIDMetadata& metadata);
   private:
    URL url_;
    ClientHTTP client_;
    static Logger logger_;
  };

  class OpenIDTokenFetcher {
   public:
    typedef std::list< std::pair<std::string, std::string> > TokenList;
    OpenIDTokenFetcher(char const * token_endpoint, UserConfig& userconfig, char const * id, char const * secret);
    bool Fetch(std::string const & grant,
               std::string const & subject,
               std::list<std::string> const & scope,
               std::list<std::string> const & audience,
               TokenList & tokens);
    static bool Import(char const * content, TokenList & tokens);
   private:
    URL url_;
    std::string client_id_;
    std::string client_secret_;
    ClientHTTP client_;
    static Logger logger_;
  };


} // namespace Arc



