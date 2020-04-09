#include <string>
#include <arc/Utils.h>
#include <arc/Logger.h>


struct cJSON;

namespace Arc {

  class JWSEKeyHolder;

  //! Class for parsing, verifying and extracting information
  //! from Tokens (JWS or JWE encoded).
  class JWSE {
   public:
    static char const * const ClaimNameSubject;
    static char const * const ClaimNameIssuer;
    static char const * const ClaimNameAudience;
    static char const * const ClaimNameNotAfter;
    static char const * const ClaimNameNotBefore;
    static char const * const ClaimNameActivities;
    static char const * const HeaderNameX509CertChain;
    static char const * const HeaderNameJSONWebKey;
    static char const * const HeaderNameJSONWebKeyId;
    static char const * const HeaderNameAlgorithm;
    static char const * const HeaderNameEncryption;

    enum KeyOrigin {
      NoKey,              //< There was no key used for validation
      EmbeddedKey,        //< Signature was validated using embedded key
      ExternalUnsafeKey,  //< Signature was validated using key obtained through HTTP protocol
      ExternalSafeKey,    //< Signature was validated using key obtained through HTTPS protocol
    };

    //! Parse token available as simple string.
    //! Mostly to be used for tokens embedded into something
    //! like HTTP header.
    JWSE(std::string const& jwseCompact);

    //! Default contructor creates valid Token
    //! with empty information.
    JWSE();    

    //! Odinary destructor
    ~JWSE();

    //! Returns true if object represents valid Token
    operator bool() const { return valid_; }

    //! Returns true if object does not represents valid Token
    bool operator!() const { return !valid_; }

    //! Returns number of authorized activities
    int ActivitiesNum() const;

    //! Access authorized activity (name,path) at specified index
    std::pair<std::string,std::string> Activity(int index);

    //! Removes authorized activity at specified index
    void RemoveActivity(int index);

    //! Adds authorized activity at specified index (-1 adds at end)
    void AddActivity(std::pair<std::string,std::string> const& activity, int index = -1);

    //! Adds authorized activity at specified index (-1 adds at end)
    void AddActivity(std::string const& activity, int index = -1);

    //! Access to header parameters by their name.
    cJSON const* HeaderParameter(char const* name) const;

    //! Set specified header parameter to new value.
    void HeaderParameter(char const* name, cJSON const* value);

    //! Set specified header parameter to new value.
    void HeaderParameter(char const* name, char const* value);

    //! Access to claim by its name.
    cJSON const* Claim(char const* name) const;

    //! Set specified claim to new value.
    void Claim(char const* name, cJSON const* value);

    //! Set specified claim to new value.
    void Claim(char const* name, char const* value);

    //! Parses passed Token and stores collected information in this object.
    bool Input(std::string const& jwseCompact);

    //! Returns information about how key used to validate signature is obtained.
    KeyOrigin InputKeyOrigin() const { return keyOrigin_; };

    //! Returns algorithm which was used to sign token (empty if no signature).
    char const * SignatureAlgoritm() const { return signAlg_.c_str(); }

    //! Serializes stored Token into string container.
    bool Output(std::string& jwseCompact) const;

    //! Assigns certificate to use for signing
    void Certificate(char const* certificate = NULL);
 
   private:    

    static Logger logger_;

    bool valid_;

    mutable AutoPointer<cJSON> header_; 

    mutable AutoPointer<JWSEKeyHolder> key_;

    mutable KeyOrigin keyOrigin_;

    mutable std::string signAlg_;

    mutable AutoPointer<cJSON> content_;

    void Cleanup();

    // Propagate information in header_ into key_
    bool ExtractPublicKey() const;

    // Copy content of key_ into header_
    bool InsertPublicKey(bool& keyAdded) const;
  
    bool VerifyHMAC(char const* digestName, void const* message, unsigned int messageSize,
                                            void const* signature, unsigned int signatureSize);

    bool VerifyECDSA(char const* digestName, void const* message, unsigned int messageSize,
                                             void const* signature, unsigned int signatureSize);

    bool VerifyRSASSAPKCS1(char const* digestName, void const* message, unsigned int messageSize,
                                            void const* signature, unsigned int signatureSize);

    bool VerifyRSASSAPSS(char const* digestName, void const* message, unsigned int messageSize,
                                            void const* signature, unsigned int signatureSize);

    bool SignHMAC(char const* digestName, void const* message, unsigned int messageSize, std::string& signature) const;

    bool SignECDSA(char const* digestName, void const* message, unsigned int messageSize, std::string& signature) const;

    bool SignRSASSAPKCS1(char const* digestName, void const* message, unsigned int messageSize, std::string& signature) const;

    bool SignRSASSAPSS(char const* digestName, void const* message, unsigned int messageSize, std::string& signature) const;

  }; // class JWSE

} // namespace Arc



