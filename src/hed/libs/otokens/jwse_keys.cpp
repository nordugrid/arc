#include <cctype>
#include <cstdlib>
#include <cstring>

#include <arc/Utils.h>
#include <arc/Base64.h>
#include <arc/StringConv.h>
#include <arc/external/cJSON/cJSON.h>
#include <arc/message/MCC.h>
#include <arc/credential/Credential.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>

#include "otokens.h"
#include "jwse_private.h"
#include "openid_metadata.h"

static EVP_PKEY* X509_get_privkey(X509*) {
  return NULL;
}


namespace Arc {
 
  class IssuerInfo {
   private:
    time_t const DefaultValidTime = 3600;
   public:
    IssuerInfo() {
      validTill = time(nullptr) + DefaultValidTime;
      isNonexpiring = false;
    }

    bool IsExpired() const {
      if(isNonexpiring) return false;
      return (static_cast< std::make_signed_t<time_t> >(time(nullptr) - validTill) > 0);
    }

    time_t validTill;
    bool   isNonexpiring;
    bool   isSafe;
    Arc::AutoPointer<OpenIDMetadata> metadata;
    Arc::AutoPointer<JWSEKeyHolderList> keys;
  };

  static std::map<std::string, IssuerInfo> issuersInfo;
  static Glib::Mutex issuersInfoLock;

  char const * const JWSE::HeaderNameX509CertChain = "x5c";
  char const * const JWSE::HeaderNameJSONWebKey = "jwk";
  char const * const JWSE::HeaderNameJSONWebKeyId = "kid";

  static void sk_x509_deallocate(STACK_OF(X509)* o) {
    sk_X509_pop_free(o, X509_free);
  }

  static void BIO_deallocate(BIO* bio) {
    (void)BIO_free(bio);
  }

  #define RETURNWARN(value, message) { \
    logger.msg(WARNING, message); \
    return value; \
  }

  static EVP_PKEY* jwkECParse(cJSON* jwkObject, Logger& logger) {
    cJSON* xObject = cJSON_GetObjectItem(jwkObject, "x");
    cJSON* yObject = cJSON_GetObjectItem(jwkObject, "y");
    cJSON* crvObject = cJSON_GetObjectItem(jwkObject, "crv");
    if((xObject == NULL) || (yObject == NULL) || (crvObject == NULL)) RETURNWARN(NULL, "Missing x, y or crv in EC JWK");
    if((xObject->type != cJSON_String) || (yObject->type != cJSON_String) || (crvObject->type != cJSON_String)) RETURNWARN(NULL, "Wrong type of x, y or crv in EC JWK");
    std::string xStr = Base64::decodeURLSafe(xObject->valuestring);
    std::string yStr = Base64::decodeURLSafe(yObject->valuestring);
    int crvNid = EC_curve_nist2nid(crvObject->valuestring);
    AutoPointer<EC_KEY> ecKey(EC_KEY_new_by_curve_name(crvNid),&EC_KEY_free);
    if(!ecKey) RETURNWARN(NULL, "Failed to create EC_KEY for EC JWK");
    AutoPointer<BIGNUM> x(NULL,&BN_free);
    AutoPointer<BIGNUM> y(NULL,&BN_free);
    if(!(x = BN_bin2bn(reinterpret_cast<unsigned char const *>(xStr.c_str()), xStr.length(), NULL))) RETURNWARN(NULL, "Failed to parse x parameter for EC JWK");
    if(!(y = BN_bin2bn(reinterpret_cast<unsigned char const *>(yStr.c_str()), yStr.length(), NULL))) RETURNWARN(NULL, "Failed to parse y parameter for EC JWK");
    if(!EC_KEY_set_public_key_affine_coordinates(ecKey.Ptr(), x.Ptr(), y.Ptr())) RETURNWARN(NULL, "Failed to assign coordinates for EC JWK");
    x.Release(); // ??
    y.Release(); // ??
    AutoPointer<EVP_PKEY> evpKey(EVP_PKEY_new(), &EVP_PKEY_free);
    if(!evpKey) RETURNWARN(NULL, "Failed to create EVP key for EC JWK");
    if(EVP_PKEY_assign_EC_KEY(evpKey.Ptr(), ecKey.Ptr()) != 1) RETURNWARN(NULL, "Failed to assign EC_KEY key for EC JWK");
    ecKey.Release();
    return evpKey.Release();
  }

  static EVP_PKEY* jwkRSAParse(cJSON* jwkObject, Logger& logger) {
    cJSON* modulusObject = cJSON_GetObjectItem(jwkObject, "n");
    cJSON* exponentObject = cJSON_GetObjectItem(jwkObject, "e");
    if((modulusObject == NULL) || (exponentObject == NULL)) RETURNWARN(NULL, "Missing e or n in RSA JWK");
    if((modulusObject->type != cJSON_String) || (exponentObject->type != cJSON_String)) RETURNWARN(NULL, "Wrong type of r or n in RSA JWK");
    std::string modulus = Base64::decodeURLSafe(modulusObject->valuestring);
    std::string exponent = Base64::decodeURLSafe(exponentObject->valuestring);
    AutoPointer<RSA> rsaKey(RSA_new(),&RSA_free);
    if(!rsaKey) RETURNWARN(NULL, "Failed to create RSA key for RSA JWK");;
    AutoPointer<BIGNUM> n(NULL,&BN_free);
    AutoPointer<BIGNUM> e(NULL,&BN_free);
    if(!(n = BN_bin2bn(reinterpret_cast<unsigned char const *>(modulus.c_str()), modulus.length(), NULL))) RETURNWARN(NULL, "Failed to parse n parameter for RSA JWK");
    if(!(e = BN_bin2bn(reinterpret_cast<unsigned char const *>(exponent.c_str()), exponent.length(), NULL))) RETURNWARN(NULL, "Failed to parse e parameter for RSA JWK");
    if(!RSA_set0_key(rsaKey.Ptr(), n.Ptr(), e.Ptr(), NULL)) RETURNWARN(NULL, "Failed to assign modules and exponent for RSA JWK");
    n.Release();
    e.Release();
    AutoPointer<EVP_PKEY> evpKey(EVP_PKEY_new(), &EVP_PKEY_free);
    if(!evpKey) RETURNWARN(NULL, "Failed to create EVP key for RSA JWK");
    if(EVP_PKEY_assign_RSA(evpKey.Ptr(), rsaKey.Ptr()) != 1) RETURNWARN(NULL, "Failed to assign RSA key for RSA JWK");
    rsaKey.Release();
    return evpKey.Release();
  }

  static EVP_PKEY* jwkOctParse(cJSON* jwkObject, Logger& logger) {
    cJSON* keyObject = cJSON_GetObjectItem(jwkObject, "k");
    cJSON* algObject = cJSON_GetObjectItem(jwkObject, "alg");
    if((keyObject == NULL) || (algObject == NULL)) RETURNWARN(NULL, "Missing k or alg in oct JWK");
    if((keyObject->type != cJSON_String) || (algObject->type != cJSON_String)) RETURNWARN(NULL, "Wrong type of k or ald in oct JWK");
    std::string key = Base64::decodeURLSafe(keyObject->valuestring);
    // It looks like RFC does not define any "alg" values with "JWK" usage.
    // TODO: finish implementing
    RETURNWARN(NULL, "The oct JWK is not implemented yet");
  }

  static bool jwkParse(cJSON* jwkObject, JWSEKeyHolder& keyHolder, Logger& logger) {
    if(jwkObject->type != cJSON_Object) return false;

    cJSON* ktyObject = cJSON_GetObjectItem(jwkObject, "kty");
    if(ktyObject == NULL) return false;
    if(ktyObject->type != cJSON_String) return false;

    EVP_PKEY* publicKey(NULL);
    if(strcmp(ktyObject->valuestring, "EC") == 0) {    
      publicKey = jwkECParse(jwkObject, logger);
    } else if(strcmp(ktyObject->valuestring, "RSA") == 0) {    
      publicKey = jwkRSAParse(jwkObject, logger);
    } else if(strcmp(ktyObject->valuestring, "oct") == 0) {    
      publicKey = jwkOctParse(jwkObject, logger);
    }
    if(publicKey == NULL)
      return false;

    keyHolder.PublicKey(publicKey);

    return true;
  }


  static bool jwkRSASerialize(cJSON* jwkObject, EVP_PKEY const * key) {
    if(!key) return false;
    RSA const* rsaKey = EVP_PKEY_get0_RSA(const_cast<EVP_PKEY*>(key));
    if(!rsaKey) return false;
    BIGNUM const* n(NULL);
    BIGNUM const* e(NULL);
    BIGNUM const* d(NULL);
    RSA_get0_key(const_cast<RSA*>(rsaKey), &n, &e, &d);
    if((!n) || (!e)) return false;

    std::string modulus;
    modulus.resize(BN_num_bytes(n));
    modulus.resize(BN_bn2bin(n, reinterpret_cast<unsigned char*>(const_cast<char*>(modulus.c_str()))));
    std::string exponent;
    exponent.resize(BN_num_bytes(e));
    exponent.resize(BN_bn2bin(e, reinterpret_cast<unsigned char*>(const_cast<char*>(exponent.c_str()))));
    if(modulus.empty() || exponent.empty()) return false;

    cJSON_AddStringToObject(jwkObject, "n", Base64::encodeURLSafe(modulus.c_str()).c_str());
    cJSON_AddStringToObject(jwkObject, "e", Base64::encodeURLSafe(exponent.c_str()).c_str());

    return true;
  }

  static cJSON* jwkSerialize(JWSEKeyHolder const& keyHolder) {
    AutoPointer<cJSON> jwkObject(cJSON_CreateObject(), &cJSON_Delete);
    if(jwkRSASerialize(jwkObject.Ptr(), keyHolder.PublicKey())) {
      return jwkObject.Release();
    };
    return NULL;
  }


  static bool x5cParse(cJSON* x5cObject, JWSEKeyHolder& keyHolder) {
    if(x5cObject->type != cJSON_Array) return false;
    // Collect the chain
    AutoPointer<X509> maincert(NULL, *X509_free);
    AutoPointer<STACK_OF(X509)> certchain(sk_X509_new_null(), &sk_x509_deallocate);
    for(int certN = 0; ; ++certN) {
      cJSON* certObject = cJSON_GetArrayItem(x5cObject, certN);
      if(certObject == NULL) break;
      if(certObject->type != cJSON_String) return false;
      // It should be PEM encoded certificate in single string and without header/footer           
      AutoPointer<BIO> mem(BIO_new(BIO_s_mem()), &BIO_deallocate);
      if(!mem) return false;
      BIO_puts(mem.Ptr(), "-----BEGIN CERTIFICATE-----\n");
      BIO_puts(mem.Ptr(), certObject->valuestring);
      BIO_puts(mem.Ptr(), "\n-----END CERTIFICATE-----");
      AutoPointer<X509> cert(PEM_read_bio_X509(mem.Ptr(), NULL, NULL, NULL), *X509_free);
      if(!cert) return false;
      if(certN == 0) {
        maincert = cert.Release();
      } else {
        if(sk_X509_insert(certchain.Ptr(), cert.Release(), certN) == 0) return false; // does sk_X509_insert increase reference?
        (void)cert.Release();
      }
    }
    // Verify the chain





    // Remember collected information
    keyHolder.Certificate(maincert.Release());
    if(sk_X509_num(certchain.Ptr()) > 0)
      keyHolder.CertificateChain(certchain.Release());

    return true;
  }

  static cJSON* x5cSerialize(JWSEKeyHolder const& keyHolder) {
    X509 const* cert = keyHolder.Certificate();
    if(!cert) return NULL;
    const STACK_OF(X509)* chain = keyHolder.CertificateChain();

    AutoPointer<cJSON> x5cObject(cJSON_CreateArray(), &cJSON_Delete);
    int certN = 0;
    while(true) {
      AutoPointer<BIO> mem(BIO_new(BIO_s_mem()), &BIO_deallocate);
      if(!PEM_write_bio_X509(mem.Ptr(), const_cast<X509*>(cert)))
        return NULL;
      std::string certStr;
      std::string::size_type certStrSize = 0;
      while(true) {
        certStr.resize(certStrSize+256);
        int l = BIO_gets(mem.Ptr(), const_cast<char*>(certStr.c_str()+certStrSize), certStr.length()-certStrSize);
        if(l < 0) l = 0;
        certStrSize+=l;
        certStr.resize(certStrSize);
        if(l == 0) break;
      };

      // Must remove header, footer and line breaks
      std::vector<std::string> lines;
      Arc::tokenize(certStr, lines, "\n");
      if(lines.size() < 2) return NULL;
      if(lines[0].find("BEGIN CERTIFICATE") == std::string::npos) return NULL;
      lines.erase(lines.begin());
      if(lines[lines.size()-1].find("END CERTIFICATE") == std::string::npos) return NULL;
      certStr = Arc::join(lines, "");

      cJSON_AddItemToArray(x5cObject.Ptr(), cJSON_CreateString(certStr.c_str()));
      if(!chain) break;
      if(certN >= sk_X509_num(chain)) break;
      cert = sk_X509_value(keyHolder.CertificateChain(), certN);
      if(!cert) break;
      ++certN;
    };

    return x5cObject.Release();
  }

  bool JWSE::ExtractPublicKey(UserConfig& userconfig) const {
    key_.Release();
    // So far we are going to support only embedded keys jwk and x5c and external kid.
    cJSON* x5cObject = cJSON_GetObjectItem(header_.Ptr(), HeaderNameX509CertChain);
    cJSON* jwkObject = cJSON_GetObjectItem(header_.Ptr(), HeaderNameJSONWebKey);
    cJSON* kidObject = cJSON_GetObjectItem(header_.Ptr(), HeaderNameJSONWebKeyId);
    if(x5cObject != NULL) {
      AutoPointer<JWSEKeyHolder> key(new JWSEKeyHolder());
      logger_.msg(DEBUG, "JWSE::ExtractPublicKey: x5c key");
      if(x5cParse(x5cObject, *key)) {
        keyOrigin_ = EmbeddedKey;
        key_ = key;
        return true;
      }
    } else if(jwkObject != NULL) {
      AutoPointer<JWSEKeyHolder> key(new JWSEKeyHolder());
      logger_.msg(DEBUG, "JWSE::ExtractPublicKey: jwk key");
      if(jwkParse(jwkObject, *key, logger_)) {
        keyOrigin_ = EmbeddedKey;
        key_ = key;
        return true;
      }
    } else if(kidObject != NULL) {
      logger_.msg(DEBUG, "JWSE::ExtractPublicKey: external jwk key");
      if(kidObject->type != cJSON_String)
        return false;
      // TODO: authority validation
      cJSON* issuerObj = cJSON_GetObjectItem(content_.Ptr(), ClaimNameIssuer);
      if(!issuerObj || (issuerObj->type != cJSON_String))
        return false;
      bool keyProtocolSafe = true;
      std::string issuerUrl(issuerObj->valuestring);
	  // Issuer can be any application specific string. Typically that is URL.
	  // Sometimes it is hostname (Google tokens). As we need URL anyway to fetch
      // keys let's assume we have either HTTP(S) URL or hostname.	  
      if(strncasecmp("https://", issuerUrl.c_str(), 8) == 0) {
		// Use as is.
	  } else if(strncasecmp("http://", issuerUrl.c_str(), 7) == 0) {
		  keyProtocolSafe = false;
	  } else {
		issuerUrl = "https://" + issuerUrl + "/";
	  }
	  
      {
        Time now;
        Arc::AutoLock<Glib::Mutex> lock(issuersInfoLock);
        for(std::map<std::string, IssuerInfo>::iterator infoIt = issuersInfo.begin(); infoIt != issuersInfo.end();) {
          std::map<std::string, IssuerInfo>::iterator nextIt = infoIt;
          ++nextIt;
          if(infoIt->second.IsExpired()) {
            logger_.msg(DEBUG, "JWSE::ExtractPublicKey: deleting outdated info: %s", infoIt->first); 
            issuersInfo.erase(infoIt);
          }
          infoIt = nextIt;
        }
        std::map<std::string, IssuerInfo>::iterator infoIt = issuersInfo.find(issuerObj->valuestring);
        if(infoIt != issuersInfo.end()) {
          for(JWSEKeyHolderList::iterator keyIt = infoIt->second.keys->begin(); keyIt != infoIt->second.keys->end(); ++keyIt) {
            if(strcmp(kidObject->valuestring, (*keyIt)->Id()) == 0) {
              keyOrigin_ = infoIt->second.isSafe ? ExternalSafeKey : ExternalUnsafeKey;
              key_ = new JWSEKeyHolder(**keyIt);
              return true;
            }
          }
        }
      }

      Arc::AutoPointer<OpenIDMetadata> serviceMetadata(new OpenIDMetadata);
      OpenIDMetadataFetcher metadataFetcher(issuerUrl.c_str(), userconfig);
      if(!metadataFetcher.Fetch(*serviceMetadata))
        return false;
      if(serviceMetadata->Error())
        return false;
      char const * jwksUri = serviceMetadata->JWKSURI();
      if(!jwksUri)
        return false;

      if(strncasecmp("https://", jwksUri, 8) != 0) keyProtocolSafe = false;

      logger_.msg(DEBUG, "JWSE::ExtractPublicKey: fetching jws key from %s", jwksUri);
      JWSEKeyFetcher keyFetcher(jwksUri, userconfig);
      Arc::AutoPointer<JWSEKeyHolderList> keys(new JWSEKeyHolderList);
      if(!keyFetcher.Fetch(*keys, logger_))
        return false;
      bool keyFound = false;
      Time validTill;
      for(JWSEKeyHolderList::iterator keyIt = keys->begin(); keyIt != keys->end(); ++keyIt) {
        Time certValid = (*keyIt)->ValidTill();
        if(keyIt == keys->begin())
          validTill = certValid;
        else if(certValid < validTill)
          validTill = certValid;

        if(!keyFound) {
          if(strcmp(kidObject->valuestring, (*keyIt)->Id()) == 0) {
            if(certValid >= Time()) {
              keyOrigin_ = keyProtocolSafe ? ExternalSafeKey : ExternalUnsafeKey;
              key_ = new JWSEKeyHolder(**keyIt);
              keyFound = true;
            }
          }
        }
      }
      SetIssuerInfo(&validTill, keyProtocolSafe, issuerObj->valuestring, serviceMetadata, keys);
      if(keyFound)
        return true;
    } else {
      logger_.msg(ERROR, "JWSE::ExtractPublicKey: no supported key");
      return true; // No key - no processing
    };
    logger_.msg(ERROR, "JWSE::ExtractPublicKey: key parsing error");

    return false;
  }

  void JWSE::SetIssuerInfo(Time* validTill, bool isSafe, std::string const& issuer, Arc::AutoPointer<OpenIDMetadata>& metadata, Arc::AutoPointer<JWSEKeyHolderList>& keys) {
    Arc::AutoLock<Glib::Mutex> lock(issuersInfoLock);
    IssuerInfo& info(issuersInfo[issuer]);
    info.isSafe = isSafe;
    info.metadata = metadata;
    info.keys = keys;
    if(validTill) {
      if(*validTill < info.validTill)
        info.validTill = validTill->GetTime();
    } else {
      info.isNonexpiring = true;
    }
  }

  bool JWSE::SetIssuerInfo(Time* validTill, bool isSafe, std::string const& issuer, std::string const& metadataStr, std::string const& keysStr, Logger& logger) {
    AutoPointer<OpenIDMetadata> metadata(new OpenIDMetadata);
    if(!OpenIDMetadataFetcher::Import(metadataStr.c_str(), *metadata)) return false;

    AutoPointer<JWSEKeyHolderList> keys(new JWSEKeyHolderList);
    if(!JWSEKeyFetcher::Import(keysStr.c_str(), *keys, logger)) return false;

    SetIssuerInfo(validTill, isSafe, issuer, metadata, keys);
    return true;
  }
  
  bool JWSE::InsertPublicKey(bool& keyAdded) const {
    keyAdded = false;
    cJSON_DeleteItemFromObject(header_.Ptr(), HeaderNameX509CertChain);
    cJSON_DeleteItemFromObject(header_.Ptr(), HeaderNameJSONWebKey);
    if(key_ && key_->Certificate()) {
      cJSON* x5cObject = x5cSerialize(*key_);
      if(x5cObject) {
        cJSON_AddItemToObject(header_.Ptr(), HeaderNameX509CertChain, x5cObject);
        keyAdded = true;
        return true;
      }
    } else if(key_ && key_->PublicKey()) {
      cJSON* jwkObject = jwkSerialize(*key_);
      if(jwkObject) {
        cJSON_AddItemToObject(header_.Ptr(), HeaderNameJSONWebKey, jwkObject);
        keyAdded = true;
        return true;
      }
    } else {
      return true; // No key - no processing
    };

    return false;
  }

  void JWSE::Certificate(char const* certificate) {
    key_.Release();
    key_ = new JWSEKeyHolder(certificate);
    if(!key_->PublicKey()) // Expect at least public key
      key_.Release();
  }


  // ---------------------------------------------------------------------------------------------

  JWSEKeyHolder::JWSEKeyHolder(): certificate_(NULL), certificateChain_(NULL), publicKey_(NULL), privateKey_(NULL) {
  }

  JWSEKeyHolder::JWSEKeyHolder(char const* certificate): certificate_(NULL), certificateChain_(NULL), publicKey_(NULL), privateKey_(NULL) {
    if(certificate != NULL) {
      AutoPointer<BIO> mem(BIO_new(BIO_s_mem()), &BIO_deallocate);
      BIO_puts(mem.Ptr(), certificate);
      certificate_ = PEM_read_bio_X509(mem.Ptr(), NULL, NULL, NULL);
      if(certificate_ == NULL) return;
      publicKey_ = X509_get_pubkey(certificate_);
      privateKey_ = PEM_read_bio_PrivateKey(mem.Ptr(), NULL, NULL, NULL);
      if(privateKey_ == NULL) return;
      // Optional chain
      AutoPointer<STACK_OF(X509)> certchain(sk_X509_new_null(), &sk_x509_deallocate);
      int certN(0);
      while(!BIO_eof(mem.Ptr())) {
        AutoPointer<X509> cert(PEM_read_bio_X509(mem.Ptr(), NULL, NULL, NULL), *X509_free);
        if(sk_X509_insert(certchain.Ptr(), cert.Ptr(), certN) == 0) break;
        (void)cert.Release();
        ++certN;
      }
      if(certN > 0) certificateChain_ = certchain.Release();
    }
  }

  JWSEKeyHolder::JWSEKeyHolder(JWSEKeyHolder const & other): certificate_(NULL), certificateChain_(NULL), publicKey_(NULL), privateKey_(NULL) {
    if(other.certificate_) {
      certificate_ = X509_dup(other.certificate_);
      publicKey_ = X509_get_pubkey(certificate_);
      privateKey_ = X509_get_privkey(certificate_);
    }
    if(other.publicKey_ && !publicKey_) {
      EVP_PKEY_up_ref(publicKey_ = other.publicKey_);
    }
    if(other.privateKey_ && !privateKey_) {
      EVP_PKEY_up_ref(privateKey_ = other.privateKey_);
    }
    if(other.certificateChain_) {
      certificateChain_ = X509_chain_up_ref(other.certificateChain_);
    }
  }

  JWSEKeyHolder::~JWSEKeyHolder() {
    if(publicKey_) EVP_PKEY_free(publicKey_);
    publicKey_ = NULL;
    if(privateKey_) EVP_PKEY_free(privateKey_);
    privateKey_ = NULL;
    if(certificate_) X509_free(certificate_);
    certificate_ = NULL;
    if(certificateChain_) sk_x509_deallocate(certificateChain_);
    certificateChain_ = NULL;
  }

  char const* JWSEKeyHolder::Id() const {
    return id_.c_str();
  }

  void JWSEKeyHolder::Id(char const* id) {
    id_.assign(id ? id : "");
  }

  EVP_PKEY const* JWSEKeyHolder::PublicKey() const {
    return publicKey_;
  }

  void JWSEKeyHolder::PublicKey(EVP_PKEY* publicKey) {
    if(publicKey_) EVP_PKEY_free(publicKey_);
    publicKey_ = publicKey;
    if(certificate_) X509_free(certificate_);
    certificate_ = NULL;
  }

  EVP_PKEY const* JWSEKeyHolder::PrivateKey() const {
    return privateKey_;
  }

  void JWSEKeyHolder::PrivateKey(EVP_PKEY* privateKey) {
    if(privateKey_) EVP_PKEY_free(privateKey_);
    privateKey_ = privateKey;
    if(certificate_) X509_free(certificate_);
    certificate_ = NULL;
  }

  X509 const* JWSEKeyHolder::Certificate() const {
    return certificate_;
  }

  void JWSEKeyHolder::Certificate(X509* certificate) {
    if(publicKey_) EVP_PKEY_free(publicKey_);
    // call increases reference count
    publicKey_ = certificate ? X509_get_pubkey(certificate) : NULL;
    if(privateKey_) EVP_PKEY_free(privateKey_);
    // call increases reference count
    privateKey_ = certificate ? X509_get_privkey(certificate) : NULL;
    if(certificate_) X509_free(certificate_);
    certificate_ = certificate;
  }

  STACK_OF(X509) const* JWSEKeyHolder::CertificateChain() const {
    return certificateChain_;
  }

  void JWSEKeyHolder::CertificateChain(STACK_OF(X509)* certificateChain) {
    if(certificateChain_) sk_x509_deallocate(certificateChain_);
    certificateChain_ = certificateChain;
  }

  Time JWSEKeyHolder::ValidTill() const {
    if(certificate_) {
      Time certValid;
      Period period;
      Credential::GetLifetime(certificateChain_, certificate_, certValid, period);
      certValid = certValid + period;
      return certValid;
    }
    if(publicKey_) {
      Time keyValid = Time() + Period(DefaultValidTime);
      return keyValid;
    }
    return Time();
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

  JWSEKeyFetcher::JWSEKeyFetcher(char const * endpoint_url, UserConfig& userconfig):
       url_(endpoint_url?url_no_cred(endpoint_url):URL()), client_(make_config(userconfig), url_) {
    client_.RelativeURI(true);
  }

  bool JWSEKeyFetcher::Fetch(JWSEKeyHolderList& keys, Logger& logger) {
    HTTPClientInfo info;
    PayloadRaw request;
    PayloadRawInterface* response(NULL);
    MCC_Status status = client_.process("GET", &request, &info, &response);
    if(!status)
      return false;
    if(!response)
      return false;
    if(!(response->Content()))
      return false;
    AutoPointer<cJSON> content(cJSON_Parse(response->Content()), &cJSON_Delete);
    if(!content)
      return false;
    cJSON* keysObj = cJSON_GetObjectItem(content.Ptr(), "keys");
    if(!keysObj || (keysObj->type != cJSON_Array))
      return false;
    for(int idx = 0; idx < cJSON_GetArraySize(keysObj); ++idx) {
      cJSON* keyObj = cJSON_GetArrayItem(keysObj, idx);
      if(!keyObj || (keyObj->type != cJSON_Object))
        continue;
      cJSON* kidObj = cJSON_GetObjectItem(keyObj, "kid");
      std::string id;
      if(kidObj && (kidObj->type == cJSON_String))
        id.assign(kidObj->valuestring);
      Arc::AutoPointer<JWSEKeyHolder> key(new JWSEKeyHolder());
      if(!key) continue;
      if(!jwkParse(keyObj, *key.Ptr(), logger))
        continue;
      key->Id(id.c_str());
      keys.add(key);
    };
    return true;
  }

  bool JWSEKeyFetcher::Import(char const * contentStr, JWSEKeyHolderList& keys, Logger& logger) {
    if(!contentStr) return false;
    AutoPointer<cJSON> content(cJSON_Parse(contentStr), &cJSON_Delete);
    if(!content)
      return false;
    cJSON* keysObj = cJSON_GetObjectItem(content.Ptr(), "keys");
    if(!keysObj || (keysObj->type != cJSON_Array))
      return false;
    for(int idx = 0; idx < cJSON_GetArraySize(keysObj); ++idx) {
      cJSON* keyObj = cJSON_GetArrayItem(keysObj, idx);
      if(!keyObj || (keyObj->type != cJSON_Object))
        continue;
      cJSON* kidObj = cJSON_GetObjectItem(keyObj, "kid");
      std::string id;
      if(kidObj && (kidObj->type == cJSON_String))
        id.assign(kidObj->valuestring);
      Arc::AutoPointer<JWSEKeyHolder> key(new JWSEKeyHolder());
      if(!key) continue;
      if(!jwkParse(keyObj, *key.Ptr(), logger))
        continue;
      key->Id(id.c_str());
      keys.add(key);
    };
    return true;
  }

} // namespace Arc
