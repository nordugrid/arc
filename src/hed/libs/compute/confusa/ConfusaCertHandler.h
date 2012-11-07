#ifndef __ARC_CONFUSACERTHANDLER_H__
#define __ARC_CONFUSACERTHANDLER_H__

#include <arc/credential/Credential.h>


namespace Arc {

/**
 * Wrapper around Credential handling the Confusa specifics.
 */
class ConfusaCertHandler {
public:
	/**
	 * Create a new ConfusaCertHandler for DN dn and given keysize
	 * Basically Confusa cert handler wraps around Credential
	 */
	ConfusaCertHandler(int keysize, const std::string dn);
	virtual ~ConfusaCertHandler();
	/**
	 * Get the certificate request managed by this confusa cert handler in base 64 encoding
	 */
	std::string getCertRequestB64();
	/*
	 * Confusa's Auth URL is an sha1 hash of the public key of the signing request
	 * This method creates such an Auth URL
	 */
	std::string createAuthURL();
	std::string getRequestLocation();
	std::string getKeyLocation();
	std::string getC();
	std::string getO();
	std::string getOU();
	std::string getCN();
	/**
	 * Create a new end entity certificate, with a private key encrypted with password password.
	 * Private key and certificate will be stored in directory storedir.
	 */
	bool createCertRequest(std::string password = "", std::string storedir ="./");
private:
	/**
	 * Splits the DN it receives up into country, organization, ou and cn parts
	 * Necessary, because Confusa stores certs in its database with the CN part only
	 */
	void setDN(const std::string dn);

	std::string req_location_;
	std::string key_location_;
	std::string dn_c_;
	std::string dn_o_;
	std::string dn_ou_;
	std::string dn_cn_;
	// wrapper class for the cert signing request
	Credential *cred_;
	static Logger logger;
};
};
#endif /* __ARC_CONFUSACERTHANDLER_H__ */
