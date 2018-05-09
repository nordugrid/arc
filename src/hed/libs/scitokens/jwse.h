#include <string>
#include <arc/Utils.h>


struct cJSON;

namespace Arc {

  class JWSEKeyHolder;

  //! Class for parsing, verifying and extracting information
  //! from SciTokens (JWS or JWE encoded).
  class JWSE {
   public:
    static char const * const HeaderNameSubject;
    static char const * const HeaderNameIssuer;
    static char const * const HeaderNameAudience;
    static char const * const HeaderNameX509CertChain;
    static char const * const HeaderNameJSONWebKey;


    //! Parse scitoken available as simple string.
    //! Mostly to be used for scitokens embedded into something
    //! like HTTP header.
    JWSE(std::string const& jwseCompact);

    //! Default contructor creates valid SciToken
    //! with empty information.
    JWSE();    

    //! Odinary destructor
    ~JWSE();

    //! Returns true if object represents valid SciToken
    operator bool() const { return valid_; }

    //! Returns true if object does not represents valid SciToken
    bool operator!() const { return !valid_; }

    //! Content extracted from SciToken
    char const* Content() const;

    //! Sets content to new value
    void Content(char const* content);

    //! Access to header parameters by their name.
    cJSON const* HeaderParameter(char const* name) const;

    //! Set specified header parameter to new value.
    void HeaderParameter(char const* name, cJSON const* value);

    //! Set specified header parameter to new value.
    void HeaderParameter(char const* name, char const* value);

    //! Parses passed SciToken and stores collected information in this object.
    bool Input(std::string const& jwseCompact);

    //! Serializes stored SciToken into string container.
    bool Output(std::string& jwseCompact) const;

   private:    

    bool valid_;

    mutable AutoPointer<cJSON> header_; 

    mutable AutoPointer<JWSEKeyHolder> key_;

    std::string content_;

    void Cleanup();

    // Propagate information in header_ into key_
    bool ExtractPublicKey() const;

    // Copy content of key_ into header_
    bool InsertPublicKey() const;
  
    bool VerifyHMAC(char const* digestName, void const* message, unsigned int messageSize,
                                            void const* signature, unsigned int signatureSize);

    bool VerifyECDSA(char const* digestName, void const* message, unsigned int messageSize,
                                             void const* signature, unsigned int signatureSize);

    bool SignHMAC(char const* digestName, void const* message, unsigned int messageSize, std::string& signature);

    bool SignECDSA(char const* digestName, void const* message, unsigned int messageSize, std::string& signature);

  }; // class JWSE

} // namespace Arc



