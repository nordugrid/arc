#include <string>


struct cJSON;

struct evp_pkey_st;

struct ec_pkey_st;

namespace Arc {
  class JWSE {
   public:
    JWSE(std::string const& jwseCompact);

    ~JWSE();

    operator bool() const { return valid_; }

    bool operator!() const { return !valid_; }

    char const* Content() const;

    cJSON const* HeaderParameter(char const* name) const;

   private:    
    bool valid_;

    mutable cJSON* header_; 

    std::string content_;

    mutable evp_pkey_st* publicKey_;

    evp_pkey_st* GetSharedKey() const { return NULL; };

    evp_pkey_st* GetPublicKey() const;
  
    bool VerifyHMAC(char const* digestName, void const* message, unsigned int messageSize,
                                            void const* signature, unsigned int signatureSize);

    bool VerifyECDSA(char const* digestName, void const* message, unsigned int messageSize,
                                             void const* signature, unsigned int signatureSize);

  }; // class JWSE
} // namespace Arc



