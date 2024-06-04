#ifndef __ARC_CREDENTIAL_H__
#define __ARC_CREDENTIAL_H__

#include <stdlib.h>
#include <stdexcept>
#include <iostream>
#include <string>
#include <openssl/asn1.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/pkcs12.h>
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/err.h>

#include <arc/Logger.h>
#include <arc/DateTime.h>
#include <arc/UserConfig.h>

#include <arc/credential/CertUtil.h>
#include <arc/credential/PasswordSource.h>

namespace Arc {

  /** \defgroup credential Credential handling classes and functions. */

  /// An exception class for the Credential class.
  /** This is an exception class that is used to handle runtime errors
   * discovered in the Credential class.
   * \ingroup credential
   * \headerfile Credential.h arc/credential/Credential.h
   */
class CredentialError : public std::runtime_error {
  public:
    // Constructor
    /** This is the constructor of the CredentialError class.
     * @param what  An explanation of the error.
     */
    CredentialError(const std::string& what="");
};

typedef enum {CRED_PEM, CRED_DER, CRED_PKCS, CRED_UNKNOWN} Credformat;
/// Signal algorithm
/**
 * \since Added in 4.0.0.
 **/
typedef enum { SIGN_DEFAULT = 0,
               SIGN_SHA1,
               SIGN_SHA224,
               SIGN_SHA256,
               SIGN_SHA384,
               SIGN_SHA512
             } Signalgorithm;

/**Logger to be used by all modules of credentials library*/
extern Logger CredentialLogger;

/// Class for handling X509 credentials.
/**Credential class covers the functionality about general processing about certificate/key
 * files, including:
 *  -# certificate/key parsing, information extracting (such as subject name,
 * issuer name, lifetime, etc.), chain verifying, extension processing about proxy certinfo,
 * extension processing about other general certificate extension (such as voms attributes,
 * it should be the extension-specific code itself to create, parse and verify the extension,
 * not the Credential class. For voms, it is some code about writing and parsing voms-implementing
 * Attribute Certificate/ RFC3281, the voms-attribute is then be looked as a binary part and
 * embedded into extension of X509 certificate/proxy certificate);
 *  -# certificate request, extension embedding and certificate signing, for both proxy certificate
 * and EEC (end entity certificate) certificate.
 *
 * The Credential class supports PEM, DER PKCS12 credentials.
 * \ingroup credential
 * \headerfile Credential.h arc/credential/Credential.h
 */
class Credential {
  public:
    /**Default constructor, only acts as a container for inquiring certificate request,
    *is meaningless for any other use.
    */
    Credential();

    /** Constructor with user-defined keylength. Needed for creation of EE certs, since some
    * applications will only support keys with a certain minimum length > 1024
    */
    Credential(int keybits);

    virtual ~Credential();

    /**Constructor, specific constructor for CA certificate
    *is meaningless for any other use.
    */
    Credential(const std::string& CAfile, const std::string& CAkey,
               const std::string& CAserial,
               const std::string& extfile, const std::string& extsect,
               const std::string& passphrase4key);

    /** Same as previuos constructor but allows password to be
    * supplied from different sources.
    * \since Added in 4.0.0.
    */
    Credential(const std::string& CAfile, const std::string& CAkey,
               const std::string& CAserial,
               const std::string& extfile, const std::string& extsect,
               PasswordSource& passphrase4key);

    /**Constructor, specific constructor for proxy certificate, only acts as a
    * container for constraining certificate signing and/or generating certificate
    * request (only keybits is useful for creating certificate request), is meaningless
    * for any other use.
    *
    * The proxyversion and policylang is for specifying the proxy certificate type and
    * the policy language inside proxy.
    * The definition of proxyversion and policy language is based on
    * http://dev.globus.org/wiki/Security/ProxyCertTypes#RFC_3820_Proxy_Certificates
    * The code is supposed to support proxy version:
    *  - GSI2 (legacy proxy)
    *  - GSI3 (Proxy draft)
    *  - RFC (RFC3820 proxy)
    *
    * and corresponding policy languages
    *  - GSI2 (GSI2, GSI2_LIMITED)
    *  - GSI3
    *  - RFC
    *    - IMPERSONATION_PROXY--1.3.6.1.5.5.7.21.1
    *    - INDEPENDENT_PROXY--1.3.6.1.5.5.7.21.2
    *    - LIMITED_PROXY--1.3.6.1.4.1.3536.1.1.1.9
    *    - RESTRICTED_PROXY--policy language undefined
    *
    * In openssl>=098, there are three types of policy languages:
    *  - id-ppl-inheritAll--1.3.6.1.5.5.7.21.1
    *  - id-ppl-independent--1.3.6.1.5.5.7.21.2
    *  - id-ppl-anyLanguage-1.3.6.1.5.5.7.21.0
    *
    * @param start start time of proxy certificate
    * @param lifetime lifetime of proxy certificate
    * @param keybits modulus size for RSA key generation,
    * it should be greater than 1024 if 'this' class is
    * used for generating X509 request; it should be '0' if
    * 'this' class is used for constraining certificate signing.
    * @param proxyversion proxy certificate version (see above for values)
    * @param policylang policy language of the proxy (see above for values)
    * @param policy path to file with policy content
    * @param pathlength path length constraint
    */
    Credential(Time start, Period lifetime = Period("PT12H"),
              int keybits = 2048, std::string proxyversion = "rfc",
              std::string policylang = "inheritAll", std::string policy = "",
              int pathlength = -1);

    /**Constructor, specific constructor for usual certificate, constructing from
    * credential files. only acts as a container for parsing the certificate and key
    * files, is meaningless for any other use. this constructor will parse the credential
    * information, and put them into "this" object
    * @param cert path to certificate file
    * @param key path to key file
    * @param cadir path to directory of CA certificates
    * @param cafile path to file with CA certificate
    * @param passphrase4key specifies the password for decrypting private key (if needed).
    *    If value is empty then password will be asked interactively. To avoid asking for
    *    password use value provided by NoPassword() method.
    * @param is_file specifies if the cert/key are from file, otherwise they
    *    are supposed to be from string. default is from file
    */
    Credential(const std::string& cert, const std::string& key,
               const std::string& cadir, const std::string& cafile, bool causedefault,
	       const std::string& passphrase4key = "", const bool is_file = true);

    /** Same as previuos constructor but allows password to be
    * supplied from different sources.
    * \since Added in 4.0.0.
    */
    Credential(const std::string& cert, const std::string& key,
               const std::string& cadir, const std::string& cafile, bool causedefault,
	       PasswordSource& passphrase4key, const bool is_file = true);

    /**Constructor, specific constructor for usual certificate, constructing from
    * information in UserConfig object. Only acts as a container for parsing the 
    * certificate and key files, is meaningless for any other use. this constructor 
    * will parse the credential information, and put them into "this" object.
    * @param usercfg UserConfig object from which certificate information is obtained
    * @param passphrase4key passphrase for private key
    */
    Credential(const UserConfig& usercfg, const std::string& passphrase4key = "");

    /** Same as previuos constructor but allows password to be
    * supplied from different sources.
    * \since Added in 4.0.0.
    */
    Credential(const UserConfig& usercfg, PasswordSource& passphrase4key);

    /**Initiate nid for proxy certificate extension*/
    static void InitProxyCertInfo(void);

    /** Returns true if credentials are valid.
      Credentials are read from locations specified in UserConfig object.
      This method is deprecated. User per-instance method IsValid() instead.
    */
    static bool IsCredentialsValid(const UserConfig& usercfg);

    /**General method for adding a new nid into openssl's global const*/
    static void AddCertExtObj(std::string& sn, std::string& oid);

    /// Set signing algorithm
    /**
     * \since Added in 4.0.0.
     **/
    void SetSigningAlgorithm(Signalgorithm signing_algorithm = SIGN_DEFAULT);

    /// Set key bits
    /**
     * \since Added in 4.0.0.
     **/
    void SetKeybits(int keybits = 0);

    static std::string NoPassword(void) { return std::string("\0",1); };

    static void GetLifetime(STACK_OF(X509) const * certchain, X509 const * cert, Time& start, Period& period);

  private:

    /** Credential object so far is not supposed to be copied */
    Credential(const Credential&);

    void InitCredential(const std::string& cert, const std::string& key,
		        const std::string& cadir, const std::string& cafile, bool causedefault,
			PasswordSource& passphrase4key, const bool is_file);
    void InitEmpty();

    /**load key from argument keybio, and put key information into argument pkey */
    //void loadKeyString(const std::string& key, EVP_PKEY* &pkey, const std::string& passphrase = "");
    void loadKeyString(const std::string& key, EVP_PKEY* &pkey, PasswordSource& passphrase);
    //void loadKeyFile(const std::string& keyfile, EVP_PKEY* &pkey, const std::string& passphrase = "");
    void loadKeyFile(const std::string& keyfile, EVP_PKEY* &pkey, PasswordSource& passphrase);
    //void loadKey(BIO* bio, EVP_PKEY* &pkey, const std::string& passphrase = "", const std::string& prompt_info = "", const bool is_file = true);

    /**load certificate from argument certbio, and put certificate information into
    * argument cert and certchain
    */
    void loadCertificateString(const std::string& cert, X509* &x509, STACK_OF(X509)** certchain);
    void loadCertificateFile(const std::string& certfile, X509* &x509, STACK_OF(X509)** certchain);
    //void loadCertificate(BIO* bio, X509* &x509, STACK_OF(X509)** certchain, const bool is_file=true);

    /**Verify whether the certificate is signed by trusted CAs
     *the verification is not needed for EEC, but needed for verifying a proxy certificate which
     *is generated by the others
     */
    bool Verify(void);

    /**Create a certificate extension based on the arguments
    * name and data. argument crit will be used by
    * X509_EXTENSION_create_by_OBJ which is called inside
    * CreateExtension method.
    */
    X509_EXTENSION* CreateExtension(const std::string& name, const std::string& data, bool crit = false);

    /** Set the start and end time for the proxy credential 'tosign'.
    * After setting, the start time of proxy will not be
    * before the later value from issuer's start time
    * and the "start" parameter, and the end time of proxy
    * will not be after the ealier value from issuer's end
    * time and the "start" parameter plus "lifetime" paremeter
    */
    static bool SetProxyPeriod(X509* tosign, X509* issuer, const Time& start, const Period& lifetime);

    /**Assistant method for signing the proxy request, the method will duplicate some information
    *(subject and extension) from signing certificate
    */
    bool SignRequestAssistant(Credential* proxy, EVP_PKEY* req_pubkey, X509** tosign);

  public:
    /**Log error information related with openssl*/
    void LogError(void) const;

    /************************************/
    /*****Get information from "this" object**/

    /**Get the verification result about certificate chain checking*/
    bool GetVerification(void) const {return verification_valid_; };

    /**Get the private key attached to this object*/
    EVP_PKEY* GetPrivKey(void) const;

    /**Get the public key attached to this object*/
    EVP_PKEY* GetPubKey(void) const;

    /**Get the certificate attached to this object*/
    X509* GetCert(void) const;

    /** Get the certificate request, if there is any */
    X509_REQ* GetCertReq(void) const;

    /**Get the certificate chain attached to this object*/
    STACK_OF(X509)* GetCertChain(void) const;

    /**Get the number of certificates in the certificate
     * chain attached to this object
     */
    int GetCertNumofChain(void) const;

    /**Get the certificate format, PEM PKCS12 or DER
    * BIO could be memory or file, they should be processed
    * differently.
    */
    Credformat getFormat_BIO(BIO * in, const bool is_file = true) const;
    Credformat getFormat_str(const std::string& source) const;
 
    /**Get the DN of the certificate attached to this object*/
    std::string GetDN(void) const;

    /**Get the Identity name of the certificate attached to this object,
     * the result will not include proxy CN
     */
    std::string GetIdentityName(void) const;

    /**Get type of the certificate attached to this object*/
    ArcCredential::certType GetType(void) const;

    /**Get issuer of the certificate attached to this object*/
    std::string GetIssuerName(void) const;

    /**Get CA of the certificate attached to this object, if the certificate 
     *is an EEC, GetCAName get the same value as GetIssuerName
     */
    std::string GetCAName(void) const;

    /**Get signing algorithm used to sign the certificate attached to this object
     * \since Added in 4.0.0. 
     **/
    Signalgorithm GetSigningAlgorithm(void) const;

    /**Get key size of the certificate attached to this object
     * \since Added in 4.0.0.
     **/
    int GetKeybits(void) const;

    /**Get the proxy policy attached to the "proxy certificate
     * information" extension of the proxy certificate
     */
    std::string GetProxyPolicy(void) const;

    /**Set the proxy policy attached to the "proxy certificate
     * information" extension of the proxy certificate
     */
    void SetProxyPolicy(const std::string& proxyversion, const std::string& policylang,
        const std::string& policy, int pathlength);

    /**Output the private key into string
     * @param content Filled with private key content
     * @param encryption whether encrypt the output private key or not
     * @param passphrase the passphrase to encrypt the output private key
     */
    bool OutputPrivatekey(std::string &content,  bool encryption = false, const std::string& passphrase ="");

    /**Output the private key into string
     * @param content Filled with private key content
     * @param encryption whether encrypt the output private key or not
     * @param passphrase the source for passphrase to encrypt the output private key
     * \since Added in 4.0.0.
     */
    bool OutputPrivatekey(std::string &content,  bool encryption, PasswordSource& passphrase);

    /**Output the public key into string*/
    bool OutputPublickey(std::string &content);

    /**Output the certificate into string
     * @param content Filled with certificate content
     * @param is_der false for PEM, true for DER
     */
    bool OutputCertificate(std::string &content, bool is_der=false);

    /**Output the certificate chain into string
     * @param content Filled with certificate chain content
     * @param is_der false for PEM, true for DER
     */
    bool OutputCertificateChain(std::string &content, bool is_der=false);

    /**Returns lifetime of certificate or proxy*/
    Period GetLifeTime(void) const;

    /**Returns validity start time of certificate or proxy*/
    Time GetStartTime() const;

    /**Returns validity end time of certificate or proxy*/
    Time GetEndTime() const;

    /**Set lifetime of certificate or proxy*/
    void SetLifeTime(const Period& period);

    /**Set start time of certificate or proxy*/
    void SetStartTime(const Time& start_time);

    /**Returns true if credentials are valid (if vaidation was performed)*/
    bool IsValid(void) const;

    /**Returns true if credentials were initialized*/
    operator bool() const;

    /**Returns true if credentials were not initialized*/
    bool operator!() const;

    /************************************/
    /*****Generate certificate request, add certificate extension, inquire certificate request,
    *and sign certificate request
    **/

    /**Add an extension to the extension part of the certificate
    *@param name the name of the extension, there OID related with the name
    *should be registered into openssl firstly
    *@param data the data which will be inserted into certificate extension
    *@param crit critical
    */
    bool AddExtension(const std::string& name, const std::string& data, bool crit = false, int type = -1);

    /**Add an extension to the extension part of the certificate
    * @param name the name of the extension, there OID related with the name
    * should be registered into openssl firstly
    * @param binary the data which will be inserted into certificate
    * extension part as a specific extension there should be specific
    * methods defined inside specific X509V3_EXT_METHOD structure
    * to parse the specific extension format.
    * For example, VOMS attribute certificate is a specific
    * extension to proxy certificate. There is specific X509V3_EXT_METHOD
    * defined in VOMSAttribute.h and VOMSAttribute.c for parsing attribute
    * certificate.
    * In openssl, the specific X509V3_EXT_METHOD can be got according to
    * the extension name/id, see X509V3_EXT_get_nid(ext_nid)
    */
    bool AddExtension(const std::string& name, char** binary);

    /**Get the specific extension (named by the parameter) in a certificate
    * this function is only supposed to be called after certificate and key
    * are loaded by the constructor for usual certificate 
    * @param name the name of the extension to get
    */
    std::string GetExtension(const std::string& name);

    /**Generate an EEC request, based on the keybits and signing
    * algorithm information inside this object
    * output the certificate request to output BIO
    *
    * The user will be asked for a private key password
    */
    bool GenerateEECRequest(BIO* reqbio, BIO* keybio, const std::string& dn = "");

    /**Generate an EEC request, output the certificate request to a string*/
    bool GenerateEECRequest(std::string &reqcontent, std::string &keycontent, const std::string& dn = "");

    /**Generate an EEC request, output the certificate request and the key to a file*/
    bool GenerateEECRequest(const char* request_filename, const char* key_filename, const std::string& dn = "");

    /**Generate a proxy request, base on the keybits and signing
    * algorithm information inside this object
    * output the certificate request to output BIO
    */
    bool GenerateRequest(BIO* bio, bool if_der = false);

    /**Generate a proxy request, output the certificate request to a string*/
    bool GenerateRequest(std::string &content, bool if_der = false);

    /**Generate a proxy request, output the certificate request to a file*/
    bool GenerateRequest(const char* filename, bool if_der = false);

    /**Inquire the certificate request from BIO, and put the request
    * information to X509_REQ inside this object,
    * and parse the certificate type from the PROXYCERTINFO
    * of request' extension
    * @param reqbio the BIO containing the certificate request
    * @param if_eec true if EEC request
    * @param if_der false for PEM; true for DER
    */
    bool InquireRequest(BIO* reqbio, bool if_eec = false, bool if_der = false);

    /**Inquire the certificate request from a string*/
    bool InquireRequest(std::string &content, bool if_eec = false, bool if_der = false);

    /**Inquire the certificate request from a file*/
    bool InquireRequest(const char* filename, bool if_eec = false, bool if_der = false);

    /**Sign request based on the information inside proxy, and
     * output the signed certificate to output BIO
     * @param proxy Credential object holding proxy information
     * @param outputbio BIO to hold the signed certificate
     * @param if_der false for PEM, true for DER
     */
    bool SignRequest(Credential* proxy, BIO* outputbio, bool if_der = false);

    /**Sign request and output the signed certificate to a string
     * @param proxy Credential object holding proxy information
     * @param content string to hold the signed certificate
     * @param if_der false for PEM, true for DER
     */
    bool SignRequest(Credential* proxy, std::string &content, bool if_der = false);

    /**Sign request and output the signed certificate to a file
     * @param proxy Credential object holding proxy information
     * @param filename path to file where certificate will be written
     * @param if_der false for PEM, true for DER
     */
    bool SignRequest(Credential* proxy, const char* filename, bool if_der = false);

    /**Self sign a certificate. This functionality is specific for creating a CA credential
    * by using this Credential class. 
    * @param dn  the DN for the subject
    * @param extfile  the configuration file which includes the extension information, typically the openssl.cnf file
    * @param extsect  the section/group name for the extension, e.g. in openssl.cnf, usr_cert and v3_ca
    * @param certfile the certificate file, which contains the signed certificate 
    */
    bool SelfSignEECRequest(const std::string& dn, const char* extfile, const std::string& extsect, const char* certfile);

    //The following three methods is about signing an EEC certificate by implementing the same
    //functionality as a normal CA
    /**Sign eec request, and output the signed certificate to output BIO*/
    bool SignEECRequest(Credential* eec, const std::string& dn, BIO* outputbio);

    /**Sign request and output the signed certificate to a string*/
    bool SignEECRequest(Credential* eec, const std::string& dn, std::string &content);

    /**Sign request and output the signed certificate to a file*/
    bool SignEECRequest(Credential* eec, const std::string& dn, const char* filename);

  private:
    // PKI files
    std::string cacertfile_;
    std::string cacertdir_;
    std::string certfile_;
    std::string keyfile_;

    //Verification result
    bool verification_valid_;
    std::string verification_proxy_policy_;

    //Certificate structures
    bool initialized_;
    X509 *           cert_;    //certificate
    ArcCredential::certType cert_type_;
    EVP_PKEY *       pkey_;    //private key
    STACK_OF(X509) * cert_chain_;  //certificates chain which is parsed
                                   //from the certificate, after
                                   //verification, the ca certificate
                                   //will be included
    PROXY_CERT_INFO_EXTENSION* proxy_cert_info_;
    Credformat       format;
    Time        start_;
    Period      lifetime_;

    //Certificate request
    X509_REQ* req_;
    RSA* rsa_key_;
    EVP_MD* signing_alg_;
    int keybits_;

    //Proxy policy
    std::string proxyversion_;
    std::string policy_;
    int proxyver_;
    int pathlength_;

    //Extensions for certificate, such as certificate policy, attributes, etc.
    STACK_OF(X509_EXTENSION)* extensions_;

    //CA functionality related information
    std::string CAserial_;
    std::string extfile_;
    std::string extsect_;

    static X509_NAME *parse_name(char *subject, long chtype, int multirdn);
};

}// namespace Arc

#endif /* __ARC_CREDENTIAL_H__ */

