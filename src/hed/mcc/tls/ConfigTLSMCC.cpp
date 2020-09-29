#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/miscutils.h>
#include <openssl/err.h>

#include <arc/credential/Credential.h>

#include "PayloadTLSStream.h"

#include "ConfigTLSMCC.h"

namespace ArcMCCTLS {

using namespace Arc;

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
  client_authn_ = true;
  cert_file_ = (std::string)(cfg["CertificatePath"]);
  key_file_ = (std::string)(cfg["KeyPath"]);
  ca_file_ = (std::string)(cfg["CACertificatePath"]);
  ca_dir_ = (std::string)(cfg["CACertificatesDir"]);
  voms_dir_ = (std::string)(cfg["VOMSDir"]);
  globus_policy_ = (((std::string)(cfg["CACertificatesDir"].Attribute("PolicyGlobus"))) == "true");
  globus_gsi_ = (((std::string)(cfg["GSI"])) == "globus");
  globusio_gsi_ = (((std::string)(cfg["GSI"])) == "globusio");
  handshake_ = (cfg["Handshake"] == "SSLv3")?ssl3_handshake:tls_handshake;
  proxy_file_ = (std::string)(cfg["ProxyPath"]);
  credential_ = (std::string)(cfg["Credential"]);
  if(client) {
    // Client is using safest setup by default
    cipher_list_ = "TLSv1:SSLv3:!eNULL:!aNULL";
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
    // protocols must be converted into special format
    protocols_ = (std::string)(cfg["Protocols"]);
  } else {
    // Server allows client to choose. But requires authentication.
    cipher_list_ = "TLSv1:SSLv3:eNULL:!aNULL";
  }
  if(cfg["Encryption"] == "required") {
  } else if(cfg["Encryption"] == "preferred") {
    cipher_list_ = "TLSv1:SSLv3:eNULL:!aNULL";
  } else if(cfg["Encryption"] == "optional") {
    cipher_list_ = "eNULL:TLSv1:SSLv3:!aNULL";
  } else if(cfg["Encryption"] == "off") {
    cipher_list_ = "eNULL:!aNULL";
  }
  
  std::vector<std::string> gridSecDir (2);
  gridSecDir[0] = G_DIR_SEPARATOR_S + std::string("etc");
  gridSecDir[1] = "grid-security";
  std::string gridSecurityDir = Glib::build_path(G_DIR_SEPARATOR_S, gridSecDir);

  if(!client) {
    
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
  if(ca_dir_.empty() && ca_file_.empty()) ca_dir_= gridSecurityDir + G_DIR_SEPARATOR_S + "certificates";
  if(voms_dir_.empty()) voms_dir_= gridSecurityDir + G_DIR_SEPARATOR_S + "vomsdir";
  if(!proxy_file_.empty()) { key_file_=proxy_file_; cert_file_=proxy_file_; };
}

bool ConfigTLSMCC::Set(SSL_CTX* sslctx) {
  if((!ca_file_.empty()) || (!ca_dir_.empty())) {
    if(!SSL_CTX_load_verify_locations(sslctx, ca_file_.empty()?NULL:ca_file_.c_str(), ca_dir_.empty()?NULL:ca_dir_.c_str())) {
      failure_ = "Can not assign CA location - "+(ca_dir_.empty()?ca_file_:ca_dir_)+"\n";
      failure_ += HandleError();
      return false;
    };
  };
  if(!credential_.empty()) {
    // First try to use in-memory credential
    Credential cred(credential_, credential_, ca_dir_, ca_file_, Credential::NoPassword(), false);
    if (!cred.IsValid()) {
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
        res = SSL_CTX_add_extra_chain_cert(sslctx, cert);
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
  if(!cipher_list_.empty()) {
    if(!SSL_CTX_set_cipher_list(sslctx,cipher_list_.c_str())) {
      failure_ = "No ciphers found to satisfy requested encryption level. "
                 "Check if OpenSSL supports ciphers '"+cipher_list_+"'\n";
      failure_ += HandleError();
      return false;
    };
  };
#if (OPENSSL_VERSION_NUMBER >= 0x10002000L)
  if(!protocols_.empty()) {
    if(SSL_CTX_set_alpn_protos(sslctx, (unsigned char const *)protocols_.c_str(), (unsigned int)protocols_.length()) != 0) {
      // TODO: add warning message
    };
  };
#endif
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

