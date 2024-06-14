#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/miscutils.h>
#include <openssl/err.h>
#include <openssl/dh.h> // For DH_* in newer OpenSSL

#include <arc/credential/Credential.h>

#include "PayloadTLSStream.h"

#include "ConfigTLSMCC.h"


// For early OpenSSL 1.0.0
#ifndef SSL_OP_NO_TLSv1_1
#define SSL_OP_NO_TLSv1_1 SSL_OP_NO_TLSv1
#endif

namespace ArcMCCTLS {

using namespace Arc;

Arc::Logger ConfigTLSMCC::logger(Arc::Logger::getRootLogger(), "MCC.TLS.Config");

static void config_VOMS_add(XMLNode cfg,std::vector<std::string>& vomscert_trust_dn) {
  XMLNode nd = cfg["VOMSCertTrustDNChain"];
  for(;(bool)nd;++nd) {
    XMLNode cnd = nd["VOMSCertTrustDN"];
    if((bool)cnd) {
      for(;(bool)cnd;++cnd) {
        vomscert_trust_dn.push_back((std::string)cnd);
      }
      vomscert_trust_dn.push_back("----NEXT CHAIN----");
    } else {
      XMLNode rnd = nd["VOMSCertTrustRegex"];
      if(rnd) {
        std::string rgx = (std::string)rnd;
        if(rgx[0] != '^') rgx.insert(0,"^");
        if(rgx[rgx.length()-1] != '$') rgx+="$";
        vomscert_trust_dn.push_back(rgx);
        vomscert_trust_dn.push_back("----NEXT CHAIN----");
      }
    }
  }
}

// This class is collection of configuration information

ConfigTLSMCC::ConfigTLSMCC(XMLNode cfg,bool client) {
  protocol_options_ = 0;
  curve_nid_ = NID_undef; // so far best seems to be NID_X25519, but let OpenSSL choose by default
  client_authn_ = true;
  system_ca_ = (((std::string)(cfg["SystemCA"])) == "true");
  allow_insecure_ = (((std::string)(cfg["AllowInsecure"])) == "true");
  if(!system_ca_) {
    ca_file_ = (std::string)(cfg["CACertificatePath"]);
    ca_dir_ = (std::string)(cfg["CACertificatesDir"]);
    globus_policy_ = (((std::string)(cfg["CACertificatesDir"].Attribute("PolicyGlobus"))) == "true");
  } else {
    globus_policy_ = false;
  }
  cert_file_ = (std::string)(cfg["CertificatePath"]);
  key_file_ = (std::string)(cfg["KeyPath"]);
  voms_dir_ = (std::string)(cfg["VOMSDir"]);
  globus_gsi_ = (((std::string)(cfg["GSI"])) == "globus");
  globusio_gsi_ = (((std::string)(cfg["GSI"])) == "globusio");
  handshake_ = (cfg["Handshake"] == "SSLv3")?ssl3_handshake:tls_handshake;
  proxy_file_ = (std::string)(cfg["ProxyPath"]);
  credential_ = (std::string)(cfg["Credential"]);
  cipher_list_ = (std::string)(cfg["Ciphers"]);
  cipher_suites_ = (std::string)(cfg["CipherSuites"]);
  server_ciphers_priority_ = (((std::string)(cfg["Ciphers"].Attribute("ServerPriority"))) == "true");
  dhparam_file_ = (std::string)(cfg["DHParamFile"]);
  if(cipher_list_.empty()) {
    // Safest setup by default
    if(client) {
      cipher_list_ = "HIGH:!eNULL:!aNULL";
      if(cfg["Encryption"] == "required") {
      } else if(cfg["Encryption"] == "preferred") {
        cipher_list_ = "HIGH:eNULL:!aNULL";
      } else if(cfg["Encryption"] == "optional") {
        cipher_list_ = "eNULL:HIGH:!aNULL";
      } else if(cfg["Encryption"] == "off") {
        cipher_list_ = "eNULL:!aNULL";
      }
    } else {
      // For server disable DES and 3DES (https://www.openssl.org/blog/blog/2016/08/24/sweet32/)
      // Do not use encryption in CBC mode (https://cve.mitre.org/cgi-bin/cvename.cgi?name=cve-2011-3389)
      // Specific ! in this list is just in case we compile with older OpenSSL. Otherwise HIGH should be enough.
      cipher_list_ = "HIGH:!3DES:!DES:!CBC:!RC4:!eNULL:!aNULL";
      if(cfg["Encryption"] == "required") {
      } else if(cfg["Encryption"] == "preferred") {
        cipher_list_ = "HIGH:!3DES:!DES:!CBC:!RC4:eNULL:!aNULL";
      } else if(cfg["Encryption"] == "optional") {
        cipher_list_ = "eNULL:HIGH:!3DES:!CBC:!DES:!RC4:!aNULL";
      } else if(cfg["Encryption"] == "off") {
        cipher_list_ = "eNULL:!aNULL";
      }
    }
  }
  if(client) {
    protocol_options_ = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3;
    hostname_ = (std::string)(cfg["Hostname"]);
    XMLNode protocol_node = cfg["Protocol"];
    while((bool)protocol_node) {
      std::string protocol = (std::string)protocol_node;
      if(!protocol.empty()) {
        if(protocol.length() > 255) protocol.resize(255);    
        protocols_.append(1,(char)protocol.length());
        protocols_.append(protocol);
      }
      ++protocol_node;
    }
  } else {
    protocol_options_ = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1; // default
    XMLNode protocol_node = cfg["Protocol"];
    if((bool)protocol_node) {
      // start from all disallowed (all we know about)
#ifdef SSL_OP_NO_TLSv1_3
      protocol_options_ = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2 | SSL_OP_NO_TLSv1_3;
#else
#ifdef SSL_OP_NO_TLSv1_2
      protocol_options_ = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1_2;
#else
      protocol_options_ = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1;
#endif
#endif
      while((bool)protocol_node) {
        std::string protocol = (std::string)protocol_node;
        std::list<std::string> tokens;
        Arc::tokenize(protocol, tokens);
        for(std::list<std::string>::iterator token = tokens.begin(); token != tokens.end(); ++token) {
          std::string str = Arc::trim(*token);
          if(str == "SSLv2") {
            protocol_options_ &= ~SSL_OP_NO_SSLv2;
          } else if(str == "SSLv3") {
            protocol_options_ &= ~SSL_OP_NO_SSLv3;
          } else if(str == "TLSv1.0") {
            protocol_options_ &= ~SSL_OP_NO_TLSv1;
          } else if(str == "TLSv1.1") {
            protocol_options_ &= ~SSL_OP_NO_TLSv1_1;
#ifdef SSL_OP_NO_TLSv1_2
          } else if(str == "TLSv1.2") {
            protocol_options_ &= ~SSL_OP_NO_TLSv1_2;
#ifdef SSL_OP_NO_TLSv1_3
          } else if(str == "TLSv1.3") {
            protocol_options_ &= ~SSL_OP_NO_TLSv1_3;
#endif
#endif
          };
        };
        ++protocol_node;
      };
    };

#if (OPENSSL_VERSION_NUMBER >= 0x10001000L)
    XMLNode curve_node = cfg["Curve"];
    if((bool)curve_node) {
      int nid = OBJ_sn2nid(((std::string)curve_node).c_str());
      if (nid != NID_undef) {
        curve_nid_ = nid;
      }
    }
#endif
    if(server_ciphers_priority_) {
      protocol_options_ |= SSL_OP_CIPHER_SERVER_PREFERENCE;
    }
  }

  std::vector<std::string> gridSecDir (2);
  gridSecDir[0] = G_DIR_SEPARATOR_S + std::string("etc");
  gridSecDir[1] = "grid-security";
  std::string gridSecurityDir = Glib::build_path(G_DIR_SEPARATOR_S, gridSecDir);

  if(!client) {
    // Default location of server certificate/key

    if(cert_file_.empty()) cert_file_= Glib::build_filename(gridSecurityDir, "hostcert.pem");
    if(key_file_.empty()) key_file_= Glib::build_filename(gridSecurityDir, "hostkey.pem");
    // Use VOMS trust DN of server certificates specified in configuration
    config_VOMS_add(cfg,vomscert_trust_dn_);
    // Look for those configured in separate files
    // TODO: should those file be reread on every connection
    XMLNode locnd = cfg["VOMSCertTrustDNChainsLocation"];
    for(;(bool)locnd;++locnd) {
      std::string filename = (std::string)locnd;
      std::ifstream file(filename.c_str());
      if (!file) {
        failure_ = "Can not read file "+filename+" with list of trusted VOMS DNs";
        continue;
      };
      XMLNode node;
      file >> node;
      config_VOMS_add(node,vomscert_trust_dn_);
    };
    std::string vproc = cfg["VOMSProcessing"];
    if(vproc == "relaxed") {
      voms_processing_ = relaxed_voms;
    } else if(vproc == "standard") {
      voms_processing_ = standard_voms;
    } else if(vproc == "strict") {
      voms_processing_ = strict_voms;
    } else if(vproc == "noerrors") {
      voms_processing_ = noerrors_voms;
    } else {
      voms_processing_ = standard_voms;
    }
    //If ClientAuthn is explicitly set to be "false" in configuration,
    //then client/authentication is not required, which means client 
    //side does not need to provide certificate and key in its configuration.
    //The default value of ClientAuthn is "true"
    if (((std::string)((cfg)["ClientAuthn"])) == "false") client_authn_ = false;
  } else {
    //If both CertificatePath and ProxyPath have not beed configured, 
    //client side can not provide certificate for server side. Then server 
    //side should not require client authentication
    if(cert_file_.empty() && proxy_file_.empty()) client_authn_ = false;
  };
  if(!system_ca_ && ca_dir_.empty() && ca_file_.empty()) ca_dir_= gridSecurityDir + G_DIR_SEPARATOR_S + "certificates";
  if(voms_dir_.empty()) voms_dir_= gridSecurityDir + G_DIR_SEPARATOR_S + "vomsdir";
  if(!proxy_file_.empty()) { key_file_=proxy_file_; cert_file_=proxy_file_; };
}

bool ConfigTLSMCC::Set(SSL_CTX* sslctx) {
  if((!ca_file_.empty()) || (!ca_dir_.empty())) {
    if(!ca_file_.empty())
      logger.msg(VERBOSE, "Using CA file: %s",ca_file_);
    if(!ca_dir_.empty())
      logger.msg(VERBOSE, "Using CA dir: %s",ca_dir_);
    if(!SSL_CTX_load_verify_locations(sslctx, ca_file_.empty()?NULL:ca_file_.c_str(), ca_dir_.empty()?NULL:ca_dir_.c_str())) {
      failure_ = "Can not assign CA location - "+(ca_dir_.empty()?ca_file_:ca_dir_)+"\n";
      failure_ += HandleError();
      return false;
    };
  } else {
    logger.msg(VERBOSE, "Using CA default location");
    if(!SSL_CTX_set_default_verify_paths(sslctx)) {
      failure_ = "Can not assign default CA location\n";
      failure_ += HandleError();
      return false;
    };
  }
  if(!credential_.empty()) {
    // First try to use in-memory credential
    Credential cred(credential_, credential_, ca_dir_, ca_file_, system_ca_, Credential::NoPassword(), false);
    if (!cred) {
      failure_ = "Failed to read in-memory credentials";
      return false;
    }
    EVP_PKEY* key = cred.GetPrivKey();
    if (SSL_CTX_use_PrivateKey(sslctx, key) != 1) {
      failure_ = "Can not load key from in-memory credentials\n";
      failure_ += HandleError();
      EVP_PKEY_free(key);
      return false;
    }
    EVP_PKEY_free(key);
    X509* cert = cred.GetCert();
    if (SSL_CTX_use_certificate(sslctx, cert) != 1) {
      failure_ = "Can not load certificate from in-memory credentials\n";
      failure_ += HandleError();
      X509_free(cert);
      return false;
    }
    X509_free(cert);
    // Add certificate chain
    STACK_OF(X509)* chain = cred.GetCertChain();
    int res = 1;
    if(chain) {
      for (int id = 0; id < sk_X509_num(chain) && res == 1; ++id) {
        X509* cert = sk_X509_value(chain,id);
        // Differently from SSL_CTX_use_certificate call to SSL_CTX_add_extra_chain_cert does not
        // increase reference counter (calls sk_x509_push inside). So must call it here.
        res = SSL_CTX_add_extra_chain_cert(sslctx, X509_dup(cert));
      }
      sk_X509_pop_free(chain, X509_free);
    }

    if (res != 1) {
      failure_ = "Can not construct certificate chain from in-memory credentials\n";
      failure_ += HandleError();
      return false;
    }
  }
  else {
    if(!cert_file_.empty()) {
      // Try to load proxy then PEM and then ASN1 certificate
      if((SSL_CTX_use_certificate_chain_file(sslctx,cert_file_.c_str()) != 1) &&
         (SSL_CTX_use_certificate_file(sslctx,cert_file_.c_str(),SSL_FILETYPE_PEM) != 1) &&
         (SSL_CTX_use_certificate_file(sslctx,cert_file_.c_str(),SSL_FILETYPE_ASN1) != 1)) {
        failure_ = "Can not load certificate file - "+cert_file_+"\n";
        failure_ += HandleError();
        return false;
      };
    };
    if(!key_file_.empty()) {
      if((SSL_CTX_use_PrivateKey_file(sslctx,key_file_.c_str(),SSL_FILETYPE_PEM) != 1) &&
         (SSL_CTX_use_PrivateKey_file(sslctx,key_file_.c_str(),SSL_FILETYPE_ASN1) != 1)) {
        failure_ = "Can not load key file - "+key_file_+"\n";
        failure_ += HandleError();
        return false;
      };
    };
    if((!key_file_.empty()) && (!cert_file_.empty())) {
      if(!(SSL_CTX_check_private_key(sslctx))) {
        failure_ = "Private key "+key_file_+" does not match certificate "+cert_file_+"\n";
        failure_ += HandleError();
        return false;
      };
    };
  };
  if(!dhparam_file_.empty()) {
    logger.msg(VERBOSE, "Using DH parameters from file: %s",dhparam_file_);
    FILE *dhf = fopen(dhparam_file_.c_str(), "r");
    if (!dhf) {
      logger.msg(ERROR, "Failed to open file with DH parameters for reading");
    } else {
      DH *dh = PEM_read_DHparams(dhf, NULL, NULL, NULL);
      fclose(dhf);
      if (dh == NULL) {
        logger.msg(ERROR, "Failed to read file with DH parameters");
      } else {
        if (SSL_CTX_set_tmp_dh(sslctx, dh) != 1) {
          logger.msg(ERROR, "Failed to apply DH parameters");
        } else {
          logger.msg(DEBUG, "DH parameters applied");
        };
        DH_free(dh);
      };
    };
  };
#if (OPENSSL_VERSION_NUMBER >= 0x10001000L)
  /*
  if(SSL_CTX_set1_curves_list(sslctx,"P-256:P-384:P-521")) {
    logger.msg(ERROR, "Failed to apply ECDH groups");
  } else {
    logger.msg(DEBUG, "ECDH groups applied");
  };
  */
  if(curve_nid_ != NID_undef) {
    logger.msg(VERBOSE, "Using curve with NID: %u",curve_nid_);
    EC_KEY* ecdh = EC_KEY_new_by_curve_name(curve_nid_);
    if (ecdh == NULL) {
      logger.msg(ERROR, "Failed to generate EC key");
    } else {
      if(SSL_CTX_set_tmp_ecdh(sslctx, ecdh) != 1) {
        logger.msg(ERROR, "Failed to apply ECDH parameters");
      } else {
        logger.msg(DEBUG, "ECDH parameters applied");
      };
      EC_KEY_free(ecdh);
    };
  };
#endif
  if(!cipher_list_.empty()) {
    logger.msg(VERBOSE, "Using cipher list: %s",cipher_list_);
    if(!SSL_CTX_set_cipher_list(sslctx,cipher_list_.c_str())) {
      failure_ = "No ciphers found to satisfy requested encryption level. "
                 "Check if OpenSSL supports ciphers '"+cipher_list_+"'\n";
      failure_ += HandleError();
      return false;
    };
  };
#if (OPENSSL_VERSION_NUMBER >= 0x10101000L)
  if(!cipher_suites_.empty()) {
    if(!SSL_CTX_set_ciphersuites(sslctx,cipher_suites_.c_str())) {
      failure_ = "No cipher suites found to satisfy requested encryption level. "
                 "Check if OpenSSL supports cipher suites '"+cipher_suites_+"'\n";
      failure_ += HandleError();
      return false;
    };
  }
#endif
#if (OPENSSL_VERSION_NUMBER >= 0x10002000L)
  if(!protocols_.empty()) {
    if(SSL_CTX_set_alpn_protos(sslctx, (unsigned char const *)protocols_.c_str(), (unsigned int)protocols_.length()) != 0) {
      // TODO: add warning message
    };
  };
#endif
  if(protocol_options_ != 0) {
    logger.msg(VERBOSE, "Using protocol options: 0x%x",protocol_options_);
    SSL_CTX_set_options(sslctx, protocol_options_);
  };
  return true;
}

std::string ConfigTLSMCC::HandleError(int code) {
  std::string errstr;
  unsigned long e = (code==SSL_ERROR_NONE)?ERR_get_error():code;
  while(e != SSL_ERROR_NONE) {
    if(e == SSL_ERROR_SYSCALL) {
      // Hiding system errors
      // int err = errno;
      // logger.msg(DEBUG, "SSL error: system call failed: %d, %s",err,StrError(err));
    } else {
      const char* lib = ERR_lib_error_string(e);
      const char* func = ERR_func_error_string(e);
      const char* reason = ERR_reason_error_string(e);
      const char* alert = SSL_alert_desc_string_long(e);
      // Ignore unknown errors
      if (reason || func || lib || std::string(alert) != "unknown") {
        if(!errstr.empty()) errstr += "\n";
        errstr += "SSL error";
        if (reason) errstr += ", \"" + std::string(reason) + "\"";
        if (func) errstr += ", in \"" + std::string(func) + "\" function";
        if (lib) errstr += ", at \"" + std::string(lib) + "\" library";
        if (alert) errstr += ", with \"" + std::string(alert) + "\" alert";
        //logger.msg(DEBUG, errstr);
      }
    };  
    e = ERR_get_error();
  }
  return errstr;
}

void ConfigTLSMCC::ClearError(void) {
  int e = ERR_get_error();
  while(e != 0) {
    e = ERR_get_error();
  }
}

} // namespace Arc

