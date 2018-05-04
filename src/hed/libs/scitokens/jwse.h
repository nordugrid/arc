#include <string>


struct cJSON;

struct evp_pkey_st;

struct ec_pkey_st;

namespace Arc {

  //! Class for parsing, verifying and extracting information
  //! from SciTokens (JWS or JWE encoded).
  class JWSE {
   public:
    static char const* HeaderNameSubject;
    static char const* HeaderNameIssuer;
    static char const* HeaderNameAudience;


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

    mutable cJSON* header_; 

    std::string content_;

    mutable evp_pkey_st* publicKey_;

    void Cleanup();

    // Not implemented yet.
    evp_pkey_st* GetSharedKey() const { return NULL; };

    evp_pkey_st* GetPublicKey() const;
  
    bool VerifyHMAC(char const* digestName, void const* message, unsigned int messageSize,
                                            void const* signature, unsigned int signatureSize);

    bool VerifyECDSA(char const* digestName, void const* message, unsigned int messageSize,
                                             void const* signature, unsigned int signatureSize);

  }; // class JWSE

} // namespace Arc



