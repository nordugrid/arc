/*
 * LocalCertHandler.cpp
 *
 *  Created on: Apr 8, 2009
 *      Author: tzangerl
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ConfusaCertHandler.h"

namespace Arc {

	Logger ConfusaCertHandler::logger(Logger::getRootLogger(), "ConfusaCertHandler");

	ConfusaCertHandler::ConfusaCertHandler(int keysize, const std::string dn) {
		SSL_load_error_strings();
		SSL_library_init();

		cred_  = new Credential(keysize);
		setDN(dn);
	}

	ConfusaCertHandler::~ConfusaCertHandler() {
		if (cred_) {
			delete cred_;
		}
	}



	// create a sha1 hash of the public key of the cert signing request for Confusa
	std::string ConfusaCertHandler::createAuthURL() {
		std::string result;

		// get the request
		X509_REQ *req = cred_->GetCertReq();

		if (!req) {
			logger.msg(ERROR, "The request is NULL!");
			return result;
		}

		// now extract the request's public key to a human readable string representation
		// with the help of BIO
		BIO *bp = BIO_new(BIO_s_mem());
		EVP_PKEY * req_pubkey = X509_REQ_get_pubkey(req);
		PEM_write_bio_PUBKEY(bp, req_pubkey);
		size_t length = BIO_ctrl_pending(bp);
		char *pub_key_c = new char[length];
		std::string pub_key;
		int num = BIO_read(bp, pub_key_c, length);

		// the BIO_ctrl_pending method is not reliable, hence cut the string to the right number of characters!
		if (num <= 0) {
			logger.msg(ERROR, "No characters were read from the BIO in public key extraction");
			return result;
		} else {
			pub_key = (const char*) pub_key_c;
			pub_key = pub_key.substr(0, num);
		}

		BIO_free(bp);

		 EVP_MD_CTX mdctx;
		 OpenSSL_add_all_digests();
		 const EVP_MD *md = EVP_get_digestbyname("SHA1");
		 if (md == NULL) {
			 logger.msg(ERROR, "Could not find any digest for the given name");
		 }

		 unsigned char md_value[EVP_MAX_MD_SIZE];
		 unsigned int md_len;
		 EVP_MD_CTX_init(&mdctx);
		 EVP_DigestInit_ex(&mdctx, md, NULL);
		 EVP_DigestUpdate(&mdctx, pub_key.c_str(), num);
		 EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
		 EVP_MD_CTX_cleanup(&mdctx);

		 if (md_value) {

			 // make the digest human readable, at the moment it is an unsigned char *
			 // TODO: not the most efficient method :(
			 // but then casting unsigned char* to string is always problematic
			 char buffer[50];
			 for(int i = 0; i < md_len; i++) {
				 int n = sprintf(buffer,"%02x", md_value[i]);
				 std::string bufferStr;
				 bufferStr = (const char*) buffer;
				 result += bufferStr.substr(0, n);
			 }

		 } else {
			 logger.msg(WARNING, "SHA1Sum appears to be empty!");
		 }

		 if (pub_key_c) {
			 delete pub_key_c;
		 }

		 return result;
	}

	bool ConfusaCertHandler::createCertRequest(std::string password, std::string storedir) {
		std::string dn = "/C=" + dn_c_ + "/O=" + dn_o_ + "/OU=" + dn_ou_ + "/CN=" + dn_cn_;

		req_location_ = storedir + "cert_request.pem";
		key_location_ = storedir + "key.pem";
		const char *request_filename = req_location_.c_str();
		const char *key_filename = key_location_.c_str();

		std::string cert;
		std::string key;

		if (!cred_->GenerateEECRequest(request_filename, key_filename, dn)) {
			logger.msg(ERROR, "Could not create a certificate request for subject %s", dn);
			return false;
		}

		std::string private_key;
		if (password == "") {
			cred_->OutputPrivatekey(private_key);
		} else {
			cred_->OutputPrivatekey(private_key, true, password);
		}

		std::fstream fop(key_location_.c_str(), std::ios::out);
		fop << private_key;
		fop.close();

		std::cout << "Cert is " << cert << " and key is " << key << std::endl;
		return true;

	}

	std::string ConfusaCertHandler::getRequestLocation() {
		return req_location_;
	}

	std::string ConfusaCertHandler::getKeyLocation() {
		return key_location_;
	}

	std::string ConfusaCertHandler::getCertRequestB64() {
		std::string non_enc_data = "";
		std::string sLine;

		std::ifstream is(req_location_.c_str());

		while (std::getline(is, sLine)) {
			non_enc_data += sLine + '\n';
		}

		non_enc_data += '\n';

		int enc_len = ((int) non_enc_data.size()*1.5)+1;
		char enc_data_char[enc_len];

		Base64::encode(enc_data_char, non_enc_data.c_str(), non_enc_data.size());
		std::string result = enc_data_char;
		return result;
	}

	void ConfusaCertHandler::setDN(const std::string dn) {
		std::string::size_type pos_c = dn.find("/C=");
		std::string::size_type pos_o = dn.find("/O=");
		std::string::size_type pos_ou = dn.find("/OU=");
		std::string::size_type pos_cn = dn.find("/CN=");

		std::string::size_type upper;
		if (pos_c != std::string::npos) {
			if ((upper = dn.find("/", pos_c+1)) == std::string::npos) {
				upper = dn.length()-1;
			}
			dn_c_ = dn.substr(pos_c+3,(upper-pos_c-3));
		}

		if (pos_o != std::string::npos) {
			if ((upper = dn.find("/", pos_o+1)) == std::string::npos) {
				upper = dn.length()-1;
			}
			dn_o_ = dn.substr(pos_o+3,(upper-pos_o-3));
		}

		// TODO: maybe fix that later
		dn_ou_ = dn_o_;

		if (pos_cn != std::string::npos) {
			if ((upper = dn.find("/", pos_cn+1)) == std::string::npos) {
				upper = dn.length();
			}
			dn_cn_ = dn.substr(pos_cn+4,(upper-pos_cn-4));
		}
	}

	std::string ConfusaCertHandler::getCN() {
		return dn_cn_;
	}

	std::string ConfusaCertHandler::getC() {
		return dn_c_;
	}

	std::string ConfusaCertHandler::getO() {
		return dn_o_;
	}

	std::string ConfusaCertHandler::getOU() {
		return dn_ou_;
	}

} // namespace ARC;
